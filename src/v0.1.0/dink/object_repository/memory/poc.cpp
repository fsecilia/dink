/*!
    \file
    Copyright (C) 2025 Frank Secilia
*/

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <memory>
#include <new>
#include <utility>
#include <variant>

#include <iostream>

namespace dink {
namespace {

//! standin for more complex, platform-specific implementation; returns 4k pages
struct page_size_t
{
    auto operator()() const noexcept -> std::size_t { return 4096; }
};

//! deletes a heap allocation using free
struct heap_allocation_deleter_t
{
    auto operator()(void* allocation) const noexcept { std::free(allocation); }
};

//! aligned heap allocator using aligned_alloc
template <typename allocation_deleter_t>
struct heap_allocator_t
{
    using allocation_t = std::unique_ptr<void, allocation_deleter_t>;

    auto allocate(std::size_t size) const -> allocation_t
    {
        auto result = allocation_t{std::malloc(size)};
        if (!result) throw std::bad_alloc{};
        return result;
    }

    auto allocate(std::size_t size, std::align_val_t align_val) const -> allocation_t
    {
        assert(!(size % static_cast<std::size_t>(align_val)));

        auto result = allocation_t{std::aligned_alloc(static_cast<std::size_t>(align_val), size)};
        if (!result) throw std::bad_alloc{};
        return result;
    }
};

namespace detail {

//! cpo for cast_allocation
template <typename dst_t, typename dst_deleter_t>
struct allocation_caster_t
{
    template <typename allocation_deleter_t, typename... dst_ctor_args_t>
    auto operator()(std::unique_ptr<void, allocation_deleter_t>&& allocation, dst_ctor_args_t&&... ctor_args) const
        -> std::unique_ptr<dst_t, dst_deleter_t>
    {
        if (!allocation) return nullptr;

        // perfect forward ctor args to placement new
        auto* node = new (allocation.get()) dst_t{std::forward<dst_ctor_args_t>(ctor_args)...};

        // transfer ownership, wrapping the allocation's original deleter
        auto result = std::unique_ptr<dst_t, dst_deleter_t>{node, dst_deleter_t{std::move(allocation.get_deleter())}};
        allocation.release();

        return result;
    }
};

} // namespace detail

//! constructs a specific type in an untyped allocation and transfers ownership
template <typename dst_t, typename dst_deleter_t>
inline constexpr auto cast_allocation = detail::allocation_caster_t<dst_t, dst_deleter_t>{};

//! deletes a list of nodes, destroying each and freeing its underlying allocation
template <typename node_t, typename allocation_deleter_t>
struct node_deleter_t
{
    auto operator()(node_t* head) const noexcept -> void
    {
        while (head)
        {
            auto next = head->next;

            // destroy instance
            head->~node_t();

            // free allocation
            allocation_deleter(head);

            head = next;
        }
    }

    [[no_unique_address]] allocation_deleter_t allocation_deleter;
};

//! append-only, node-based, intrusive list of owned allocations
template <typename node_t, typename node_deleter_t>
class allocation_list_t
{
public:
    using allocated_node_t = std::unique_ptr<node_t, node_deleter_t>;

    auto push(allocated_node_t&& node) noexcept -> void
    {
        node->next = head_.release();
        head_.reset(node.release());
    }

    auto top() const noexcept -> node_t const& { return *head_; }
    auto top() noexcept -> node_t& { return const_cast<node_t&>(static_cast<allocation_list_t const&>(*this).top()); }

    explicit allocation_list_t(allocated_node_t head = nullptr) noexcept : head_{std::move(head)} {}

private:
    allocated_node_t head_;
};

//! intrusive list node with an arena as a payload
template <typename arena_t>
struct arena_node_t
{
    arena_node_t* next;
    arena_t arena;
};

//! allocates from within a region of memory
class arena_t
{
public:
    using allocation_t = void*;

    struct pending_allocation_t
    {
        arena_t* allocator;
        allocation_t allocation;

        auto result() const noexcept -> allocation_t { return allocation; }
        auto commit() noexcept -> void { allocator->commit(allocation); }
    };

    auto max_allocation_size() const noexcept -> std::size_t { return max_allocation_size_; }

    auto reserve(std::size_t size, std::align_val_t align_val) noexcept -> pending_allocation_t
    {
        auto const alignment = static_cast<std::size_t>(align_val);
        size = std::max(size, std::size_t{1});

        auto const next = (cur_ + alignment - 1) & -alignment;

        auto const fits = next + size <= end_;
        if (!fits) return pending_allocation_t{this, nullptr};

        return pending_allocation_t{this, reinterpret_cast<allocation_t>(next)};
    }

    auto commit(allocation_t allocation) noexcept -> void { cur_ = reinterpret_cast<address_t>(allocation); }

    arena_t(void* begin, std::size_t size, std::size_t max_allocation_size)
        : cur_{reinterpret_cast<address_t>(begin)}, end_{cur_ + size}, max_allocation_size_{max_allocation_size}
    {}

private:
    using address_t = uintptr_t;
    address_t cur_;
    address_t end_;
    std::size_t max_allocation_size_;
};

template <typename page_size_t>
struct arena_sizing_params_t
{
    std::size_t page_size;
    std::size_t num_pages = 16;
    std::size_t max_allocation_size = page_size * num_pages / 8;
    arena_sizing_params_t(page_size_t page_size) noexcept : page_size{page_size()} {}
};

//! allocates arena nodes aligned to the os page size, in multiples of the os page size, using given allocator
template <typename node_t, typename node_deleter_t, typename allocator_t, typename arena_t, typename sizing_params_t>
class arena_node_factory_t
{
public:
    using allocated_node_t = std::unique_ptr<node_t, node_deleter_t>;

    auto operator()() -> allocated_node_t
    {
        // allocate aligned arena
        auto allocation = allocator_.allocate(arena_size_, std::align_val_t{page_size_});

        // lay out node as first allocation in arena
        auto const node_address = reinterpret_cast<std::byte*>(std::to_address(allocation));
        auto const remaining_arena_begin = node_address + sizeof(node_t);
        auto const remaining_arena_size = arena_size_ - sizeof(node_t);

        // construct node in allocation
        return cast_allocation<node_t, node_deleter_t>(
            std::move(allocation), nullptr,
            arena_t{remaining_arena_begin, remaining_arena_size, arena_max_allocation_size_}
        );
    }

    arena_node_factory_t(allocator_t allocator, sizing_params_t sizing_params) noexcept
        : allocator_{std::move(allocator)}, page_size_{sizing_params.page_size},
          arena_size_{sizing_params.page_size * sizing_params.num_pages},
          arena_max_allocation_size_{sizing_params.max_allocation_size}
    {}

private:
    allocator_t allocator_;
    std::size_t page_size_;
    std::size_t arena_size_;
    std::size_t arena_max_allocation_size_;
};

template <typename policy_t>
struct pooled_arena_allocator_ctor_params_t
{
    using allocated_node_t = policy_t::allocated_node_t;
    using allocation_list_t = policy_t::allocation_list_t;
    using node_deleter_t = policy_t::node_deleter_t;
    using node_factory_t = policy_t::node_factory_t;

    explicit pooled_arena_allocator_ctor_params_t(node_factory_t node_factory) noexcept
        : node_factory{std::move(node_factory)}, allocation_list{this->node_factory()}
    {}

    node_factory_t node_factory;
    allocation_list_t allocation_list;
};

//! allocates from a pool of managed arenas
template <typename policy_t>
class pooled_arena_allocator_t
{
public:
    using allocation_list_t = policy_t::allocation_list_t;
    using allocated_node_t = policy_t::allocated_node_t;
    using allocation_t = policy_t::allocation_t;
    using node_factory_t = policy_t::node_factory_t;

    using arena_t = policy_t::arena_t;
    using ctor_params_t = pooled_arena_allocator_ctor_params_t<policy_t>;

    struct pending_allocation_t
    {
        pooled_arena_allocator_t* allocator;
        typename arena_t::pending_allocation_t arena_pending_allocation;
        allocated_node_t new_node;

        auto result() const noexcept -> allocation_t { return arena_pending_allocation.result(); }
        auto commit() noexcept -> void
        {
            allocator->commit(std::move(new_node));
            arena_pending_allocation.commit();
        }
    };

    auto max_allocation_size() const noexcept -> std::size_t { return arena().max_allocation_size(); }

    auto reserve(std::size_t size, std::align_val_t align_val) -> pending_allocation_t
    {
        auto arena_pending_allocation = arena().reserve(size, align_val);
        if (arena_pending_allocation.result())
        {
            return pending_allocation_t{this, std::move(arena_pending_allocation), nullptr};
        }

        // arena is full; create another and allocate from that
        auto new_node = node_factory_();
        arena_pending_allocation = new_node->arena.reserve(size, align_val);
        return pending_allocation_t{this, std::move(arena_pending_allocation), std::move(new_node)};
    }

    auto commit(allocated_node_t&& new_arena) noexcept -> void
    {
        if (new_arena) allocation_list_.push(std::move(new_arena));
    }

    explicit pooled_arena_allocator_t(ctor_params_t params) noexcept
        : node_factory_{std::move(params).node_factory}, allocation_list_{std::move(params).allocation_list}
    {}

private:
    node_factory_t node_factory_;
    allocation_list_t allocation_list_;

    auto arena() const noexcept -> arena_t const& { return allocation_list_.top().arena; }
    auto arena() noexcept -> arena_t&
    {
        return const_cast<arena_t&>(static_cast<pooled_arena_allocator_t const&>(*this).arena());
    }
};

struct scoped_node_t
{
    scoped_node_t* next;
    void* allocation;
};

//! allocates scoped nodes by prepending them to a manually aligned buffer
template <typename node_t, typename node_deleter_t, typename allocator_t>
class scoped_node_factory_t
{
public:
    using allocated_node_t = std::unique_ptr<node_t, node_deleter_t>;

    auto operator()(std::size_t size, std::align_val_t align_val) -> allocated_node_t
    {
        // allocate padded buffer
        auto const alignment = static_cast<std::size_t>(align_val);
        auto allocation = allocator_.allocate(size + sizeof(node_t) + alignment - 1);

        // lay out node and then aligned allocation
        auto const node_address = reinterpret_cast<std::byte*>(std::to_address(allocation));
        auto const aligned_allocation = reinterpret_cast<void*>(
            (reinterpret_cast<uintptr_t>(node_address) + sizeof(node_t) + alignment - 1) & -alignment
        );

        // construct node in allocation
        return cast_allocation<node_t, node_deleter_t>(std::move(allocation), nullptr, aligned_allocation);
    }

    explicit scoped_node_factory_t(allocator_t allocator) noexcept : allocator_{std::move(allocator)} {}

private:
    allocator_t allocator_;
};

//! tracks allocations internally, freeing them on destruction
template <typename policy_t>
class scoped_allocator_t
{
public:
    using allocation_list_t = policy_t::allocation_list_t;
    using allocated_node_t = policy_t::allocated_node_t;
    using allocation_t = policy_t::allocation_t;
    using node_factory_t = policy_t::node_factory_t;

    struct pending_allocation_t
    {
        scoped_allocator_t* allocator;
        allocated_node_t new_node;

        auto result() const noexcept -> allocation_t { return new_node->allocation; }
        auto commit() noexcept -> void { allocator->commit(std::move(new_node)); }
    };

    auto reserve(std::size_t size, std::align_val_t align_val) -> pending_allocation_t
    {
        return pending_allocation_t{this, node_factory_(size, align_val)};
    }

    auto commit(allocated_node_t new_node) noexcept -> void { allocation_list_.push(std::move(new_node)); }

    explicit scoped_allocator_t(
        node_factory_t node_factory, allocation_list_t allocation_list = allocation_list_t{}
    ) noexcept
        : node_factory_{std::move(node_factory)}, allocation_list_{std::move(allocation_list)}
    {}

    scoped_allocator_t(scoped_allocator_t const&) = delete;
    auto operator=(scoped_allocator_t const&) -> scoped_allocator_t& = delete;
    scoped_allocator_t(scoped_allocator_t&&) = default;
    auto operator=(scoped_allocator_t&&) -> scoped_allocator_t& = default;

private:
    node_factory_t node_factory_;
    allocation_list_t allocation_list_{};
};

//! dispatches to one of two allocators based on requested allocation size
template <typename small_object_allocator_t, typename large_object_allocator_t>
class thresholding_allocator_t
{
public:
    using allocation_t = void*;

    struct pending_allocation_t
    {
        std::variant<
            typename small_object_allocator_t::pending_allocation_t,
            typename large_object_allocator_t::pending_allocation_t>
            pending_allocation;

        auto result() const noexcept -> allocation_t
        {
            return std::visit(
                [](auto& pending_allocation) noexcept { return pending_allocation.result(); }, pending_allocation
            );
        }

        auto commit() noexcept -> void
        {
            std::visit([](auto& pending_allocation) noexcept { pending_allocation.commit(); }, pending_allocation);
        }
    };

    auto reserve(std::size_t size, std::align_val_t align_val) -> pending_allocation_t
    {
        auto const total_allocation_size = size + static_cast<std::size_t>(align_val) - 1;
        if (total_allocation_size <= small_object_allocator_.max_allocation_size())
        {
            return pending_allocation_t{small_object_allocator_.reserve(size, align_val)};
        }

        return pending_allocation_t{large_object_allocator_.reserve(size, align_val)};
    }

    auto threshold() const noexcept -> std::size_t { return small_object_allocator_.max_allocation_size(); }

    thresholding_allocator_t(
        small_object_allocator_t small_object_allocator, large_object_allocator_t large_object_allocator
    ) noexcept
        : small_object_allocator_{std::move(small_object_allocator)},
          large_object_allocator_{std::move(large_object_allocator)}
    {}

private:
    small_object_allocator_t small_object_allocator_;
    large_object_allocator_t large_object_allocator_;
};

TEST(thresholding_allocator_test, example)
{
    // composition root

    // common allocator (heap)
    using heap_deleter_t = dink::heap_allocation_deleter_t;
    using heap_allocator_t = dink::heap_allocator_t<heap_deleter_t>;

    // small object allocator (pooled arena)
    struct small_object_allocator_policy_t
    {
        using arena_t = dink::arena_t;
        using sizing_params_t = arena_sizing_params_t<page_size_t>;

        using node_t = arena_node_t<arena_t>;
        using node_deleter_t = dink::node_deleter_t<node_t, heap_deleter_t>;

        using allocation_list_t = dink::allocation_list_t<node_t, node_deleter_t>;
        using allocated_node_t = allocation_list_t::allocated_node_t;
        using allocation_t = arena_t::allocation_t;
        using node_factory_t = arena_node_factory_t<node_t, node_deleter_t, heap_allocator_t, arena_t, sizing_params_t>;
    };
    using small_object_node_factory_t = small_object_allocator_policy_t::node_factory_t;
    using small_object_ctor_params_t = dink::pooled_arena_allocator_ctor_params_t<small_object_allocator_policy_t>;

    using small_object_allocator_t = dink::pooled_arena_allocator_t<small_object_allocator_policy_t>;

    // large object allocator (scoped)
    struct large_object_allocator_policy_t
    {
        using node_t = dink::scoped_node_t;
        using node_deleter_t = dink::node_deleter_t<node_t, heap_deleter_t>;

        using allocation_list_t = allocation_list_t<node_t, node_deleter_t>;
        using allocated_node_t = allocation_list_t::allocated_node_t;
        using allocation_t = void*;
        using node_factory_t = dink::scoped_node_factory_t<node_t, node_deleter_t, heap_allocator_t>;
    };
    using large_object_node_factory_t = large_object_allocator_policy_t::node_factory_t;

    using large_object_allocator_t = scoped_allocator_t<large_object_allocator_policy_t>;

    // final composed allocator type
    using thresholding_allocator_t = dink::thresholding_allocator_t<small_object_allocator_t, large_object_allocator_t>;

    // instantiate and inject dependencies
    auto allocator = thresholding_allocator_t{
        small_object_allocator_t{
            small_object_ctor_params_t{small_object_node_factory_t{heap_allocator_t{}, page_size_t{}}}
        },
        large_object_allocator_t{large_object_node_factory_t{heap_allocator_t{}}}
    };

    // usage
    std::cout << "testing allocations\n";
    try
    {
        std::cout << "reserve small allocation: ";
        auto pending_small_allocation = allocator.reserve(allocator.threshold() - 1, std::align_val_t{4});
        std::cout << "succeeded\n";

        std::cout << "commit small allocation: ";
        pending_small_allocation.commit();
        std::cout << "succeeded\n";

        std::cout << "reserve large allocation: ";
        auto pending_large_allocation
            = allocator.reserve(allocator.threshold() * 2, std::align_val_t{allocator.threshold()});
        std::cout << "succeeded\n";

        std::cout << "commit large allocation: ";
        pending_large_allocation.commit();
        std::cout << "succeeded\n";
    }
    catch (std::exception const& e)
    {
        std::cerr << "failed: " << e.what() << '\n';
    }
}

} // namespace
} // namespace dink
