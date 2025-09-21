/*!
    \file
    Copyright (C) 2025 Frank Secilia
*/

#include <dink/lib.hpp>
#include <dink/object_repository/memory/alignment.hpp>
#include <dink/object_repository/memory/construct_in_allocation.hpp>
#include <algorithm>
#include <cassert>
#include <memory>
#include <new>
#include <ratio>

namespace dink::paged {

//! intrusive list node with a page as payload
template <typename page_t>
struct node_t
{
    node_t* next;
    page_t page;
};

/*!
    allocates page nodes aligned to the os page size, in multiples of the that page size, using given allocator

    This factory is responsible for acquiring a memory page from the OS and constructing a `page_node_t` within it. The
    node's metadata (e.g., the `next` pointer) is placed at the very start of the page, and the `page_t` member, which
    manages the rest of the memory, immediately follows. This "in-band" metadata strategy ensures that an entire OS
    page is not wasted on bookkeeping, because placing this data out-of-band would require allocating an additional
    aligned OS page.
*/
template <typename policy_t>
class node_factory_t
{
public:
    using node_allocator_t = policy_t::node_allocator_t;
    using node_deleter_t = policy_t::node_deleter_t;
    using node_t = policy_t::node_t;
    using page_t = policy_t::page_t;

    using allocated_node_t = std::unique_ptr<node_t, node_deleter_t>;

    [[nodiscard]] auto operator()(std::size_t page_size, std::align_val_t page_alignment) -> allocated_node_t
    {
        // allocate aligned page
        auto allocation = node_allocator_.allocate(page_size, page_alignment);

        // lay out node as first allocation in page
        auto const node_address = reinterpret_cast<std::byte*>(std::to_address(allocation));
        auto const remaining_page_begin = node_address + sizeof(node_t);
        auto const remaining_page_size = page_size - sizeof(node_t);

        // construct node in allocation
        return construct_in_allocation<node_t, node_deleter_t>(
            std::move(allocation), nullptr, page_t{remaining_page_begin, remaining_page_size}
        );
    }

    node_factory_t(node_allocator_t node_allocator) noexcept : node_allocator_{std::move(node_allocator)} {}

private:
    node_allocator_t node_allocator_;
};

/*!
    defines the memory sizing and layout for pages

    The maximum allocation size is chosen to balance amortization vs internal fragmentation. It is a large integer
    fraction of the total page size.

    \tparam os_page_size_provider_t callable that returns the operating system's physical memory page size
*/
template <typename os_page_size_provider_t>
struct page_size_config_t
{
    //! number of physical os pages in one logical page
    static inline auto constexpr const os_pages_per_logical_page = std::size_t{16};

    //! ratio applied to logical page size to derive the maximum allocation size
    using max_allocation_size_scale = std::ratio<1, 8>;

    //! size, in bytes, of a physical memory page from the os
    std::size_t os_page_size;

    //! total size, in bytes, of one logical page
    std::size_t page_size = os_page_size * os_pages_per_logical_page;

    //! threshold for the largest single allocation allowed from a page
    std::size_t max_allocation_size = (page_size / max_allocation_size_scale::den) * max_allocation_size_scale::num;

    page_size_config_t(os_page_size_provider_t os_page_size_provider) noexcept : os_page_size{os_page_size_provider()}
    {}
};

/*!
    command to commit paged allocation after reserving it

    This type is the \ref reservation for the paged allocator.
*/
template <typename policy_t>
class reservation_t
{
public:
    using allocated_node_t = policy_t::allocated_node_t;
    using allocator_t = policy_t::allocator_t;
    using page_reservation_t = policy_t::page_reservation_t;

    auto allocation() const noexcept -> void* { return page_reservation_.allocation(); }
    auto commit() noexcept -> void
    {
        allocator_->commit(std::move(new_node_));
        page_reservation_.commit();
    }

    reservation_t(allocator_t& allocator, page_reservation_t page_reservation, allocated_node_t new_node) noexcept
        : allocator_{&allocator}, page_reservation_{std::move(page_reservation)}, new_node_{std::move(new_node)}
    {}

private:
    allocator_t* allocator_;
    page_reservation_t page_reservation_;
    allocated_node_t new_node_;
};

/*!
    allocates from a pool of managed pages

    This type uses a \ref reservation.

    \warning memory layout trade-off
    To avoid allocating an entire OS page (e.g., 4KiB) for a few tracking pointers, this allocator places its internal
    bookkeeping node at the beginning of the requested page itself. This means the total available range is reduced
    slightly, and the pointer returned by an allocation from a new page will be slightly offset from the page's aligned
    base address.

    \warning This is a deliberate design decision. We could instead allocate an additional page and increase the
    requested size, but between reducing the requested range by about 16 bytes vs increasing it by just under 4kB, this
    choice seems more reasonable for this type's intended purpose as a small object allocator.
*/
template <typename policy_t>
class allocator_t
{
public:
    using allocated_node_t = policy_t::allocated_node_t;
    using allocation_list_t = policy_t::allocation_list_t;
    using node_factory_t = policy_t::node_factory_t;
    using node_t = policy_t::node_t;
    using page_size_config_t = policy_t::page_size_config_t;
    using reservation_t = policy_t::reservation_t;

    //! maximum allocation size to balance amortization vs internal fragmentation
    auto max_allocation_size() const noexcept -> std::size_t { return page_size_config_.max_allocation_size; }

    /*!
        \pre align_val is a nonzero power of two
        \pre size + align_val - 1 does not exceed max_allocation_size
        \pre max_allocation_size is not larger than the page size
    */
    [[nodiscard]] auto reserve(std::size_t size, std::align_val_t align_val) -> reservation_t
    {
        assert(is_valid_alignment(align_val));
        assert(size + static_cast<std::size_t>(align_val) - 1 <= max_allocation_size());

        // try to allocate from current page
        auto page_reservation = page().reserve(size, align_val);
        if (page_reservation.allocation()) { return reservation_t{this, std::move(page_reservation), nullptr}; }

        /*
            page is full; create another node containing a new page, allocate from that, then include that new page in
            the reservation.

            Allocation from a new page cannot fail as long as max_allocation_size() is not larger than the page size,
            which is a precondition.
        */
        auto new_node = create_node();
        page_reservation = new_node->page.reserve(size, align_val);
        return reservation_t{this, std::move(page_reservation), std::move(new_node)};
    }

    auto commit(allocated_node_t&& new_page) noexcept -> void
    {
        // if the allocation came from a new page, make that the new list tail
        if (new_page) allocation_list_.push(std::move(new_page));
    }

    explicit allocator_t(node_factory_t create_node, page_size_config_t page_size_config) noexcept
        : create_node_{std::move(create_node)}, page_size_config_{std::move(page_size_config)},
          allocation_list_{this->create_node()}
    {}

private:
    using page_t = policy_t::page_t;

    node_factory_t create_node_;
    page_size_config_t page_size_config_;
    allocation_list_t allocation_list_;

    auto page() const noexcept -> page_t const& { return allocation_list_.back().page; }
    auto page() noexcept -> page_t& { return const_cast<page_t&>(static_cast<allocator_t const&>(*this).page()); }

    auto create_node() -> allocated_node_t
    {
        return create_node_(page_size_config_.page_size, std::align_val_t{page_size_config_.os_page_size});
    }
};

} // namespace dink::paged
