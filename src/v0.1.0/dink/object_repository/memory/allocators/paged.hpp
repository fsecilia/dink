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

/*!
    defines the memory sizing and layout for pages

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

//! intrusive list node with a page as payload
template <typename page_t>
struct node_t
{
    node_t* next;
    page_t page;
};

//! allocates page nodes aligned to the os page size, in multiples of the that page size, using given allocator
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

} // namespace dink::paged
