/*!
    \file
    Copyright (C) 2025 Frank Secilia
*/

#include <algorithm>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <new>
#include <utility>

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

    auto allocate(std::size_t size, std::align_val_t align_val) const -> allocation_t
    {
        assert(!(size % static_cast<std::size_t>(align_val)));

        auto result = allocation_t{std::aligned_alloc(static_cast<std::size_t>(align_val), size)};
        if (!result) throw std::bad_alloc{};
        return result;
    }
};

//! deletes a list of nodes
template <typename node_t, typename node_deleter_t>
struct node_list_deleter_t
{
    auto operator()(node_t* head) const noexcept -> void
    {
        while (head)
        {
            auto next = head->next;
            node_deleter(head);
            head = next;
        }
    }

    [[no_unique_address]] node_deleter_t node_deleter;
};

//! append-only, node-based, intrusive list of owned allocations
template <typename node_t, typename node_deleter_t, typename node_list_deleter_t>
class allocation_list_t
{
public:
    using allocated_node_t = std::unique_ptr<node_t, node_deleter_t>;
    using allocated_node_list_t = std::unique_ptr<node_t, node_list_deleter_t>;

    auto push(allocated_node_t&& node) noexcept -> void
    {
        node->next = head_.release();
        head_.reset(node.release());
    }

    auto top() const noexcept -> node_t& { return *head_; }

    explicit allocation_list_t(allocated_node_list_t head = nullptr) noexcept : head_{std::move(head)} {}

private:
    allocated_node_list_t head_;
};

//! intrusive list node with an arena as a payload
template <typename arena_p>
struct arena_node_t
{
    using arena_t = arena_p;

    arena_node_t* next;
    arena_t arena;
};

//! deletes a single node
template <typename node_t, typename allocation_deleter_t>
struct arena_node_deleter_t
{
    auto operator()(node_t* node) const noexcept -> void
    {
        if (!node) return;
        auto const allocation = node->arena.allocation();
        node->~node_t();
        allocation_deleter(allocation);
    }

    [[no_unique_address]] allocation_deleter_t allocation_deleter;
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

    auto allocation() const noexcept -> void* { return allocation_; }
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
        : allocation_{begin}, cur_{reinterpret_cast<address_t>(begin)}, end_{cur_ + size},
          max_allocation_size_{max_allocation_size}
    {}

private:
    void* allocation_;

    using address_t = uintptr_t;
    address_t cur_;
    address_t end_;
    std::size_t max_allocation_size_;
};

//! allocates arena nodes aligned to the os page size, in multiples of the os page size, using given allocator
template <typename node_p, typename node_deleter_p, typename allocator_t, typename page_size_t>
class arena_node_factory_t
{
public:
    using node_t = node_p;
    using node_deleter_t = node_deleter_p;
    using allocated_node_t = std::unique_ptr<node_t, node_deleter_t>;

    auto operator()() -> allocated_node_t
    {
        auto const total_allocation_size = arena_size_ + page_size_;
        auto allocation = allocator_.allocate(total_allocation_size, std::align_val_t{page_size_});

        auto const allocation_begin = std::to_address(allocation);
        auto const node_location = static_cast<std::byte*>(allocation_begin) + arena_size_;

        auto node = new (node_location)
            node_t{nullptr, typename node_t::arena_t{allocation_begin, arena_size_, arena_max_allocation_size_}};
        auto result = allocated_node_t{node, node_deleter_t{std::move(allocation.get_deleter())}};

        allocation.release();
        return result;
    }

    arena_node_factory_t(allocator_t allocator, page_size_t page_size) noexcept
        : allocator_{std::move(allocator)}, page_size_{page_size()}
    {}

private:
    allocator_t allocator_;
    std::size_t page_size_;
    std::size_t arena_size_{page_size_ * 16};
    std::size_t arena_max_allocation_size_{arena_size_ / 8};
};

//! configures dependent types for a pooled arena allocator
template <typename node_factory_p>
struct pooled_arena_allocator_policy_t
{
    using node_factory_t = node_factory_p;

    using node_t = node_factory_t::node_t;
    using node_deleter_t = node_factory_t::node_deleter_t;
    using node_list_deleter_t = node_list_deleter_t<node_t, node_deleter_t>;

    using allocation_list_t = allocation_list_t<node_t, node_deleter_t, node_list_deleter_t>;
    using allocated_node_t = allocation_list_t::allocated_node_t;
    using allocated_node_list_t = allocation_list_t::allocated_node_list_t;

    using arena_t = node_t::arena_t;
};

/*!
    nontrivial ctor params for a pooled arena allocator

    pooled_arena_allocator_t takes both a node factory and the initial node. This forms an awkward dependency between
    the constructor parameters itself. This type handles that dependency between parameters in an injectable way.
*/
template <typename pooled_arena_allocator_policy_t>
struct pooled_arena_allocator_ctor_params_t
{
    pooled_arena_allocator_policy_t::node_factory_t node_factory;
    pooled_arena_allocator_policy_t::allocated_node_t initial_node{node_factory()};
};

//! allocates from a pool of managed arenas
template <typename pooled_arena_allocator_policy_t>
class pooled_arena_allocator_t
{
public:
    using arena_t = pooled_arena_allocator_policy_t::arena_t;
    using allocation_list_t = pooled_arena_allocator_policy_t::allocation_list_t;
    using allocated_node_t = pooled_arena_allocator_policy_t::allocated_node_t;
    using allocated_node_list_t = pooled_arena_allocator_policy_t::allocated_node_list_t;
    using factory_t = pooled_arena_allocator_policy_t::node_factory_t;
    using node_list_deleter_t = pooled_arena_allocator_policy_t::node_list_deleter_t;

    using allocation_t = arena_t::allocation_t;

    struct pending_allocation_t
    {
        pooled_arena_allocator_t* allocator;
        arena_t::pending_allocation_t arena_pending_allocation;
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
        if (arena_pending_allocation.allocation)
        {
            return pending_allocation_t{this, std::move(arena_pending_allocation), nullptr};
        }

        auto new_node = node_factory_();
        arena_pending_allocation = new_node->arena.reserve(size, align_val);
        return pending_allocation_t{this, std::move(arena_pending_allocation), std::move(new_node)};
    }

    auto commit(allocated_node_t&& new_arena) noexcept -> void
    {
        if (new_arena) allocation_list_.push(std::move(new_arena));
    }

    using ctor_params_t = pooled_arena_allocator_ctor_params_t<pooled_arena_allocator_policy_t>;
    explicit pooled_arena_allocator_t(ctor_params_t params)
        : node_factory_{std::move(params.node_factory)},
          allocation_list_{allocated_node_list_t{
              params.initial_node.release(), node_list_deleter_t{std::move(params.initial_node.get_deleter())}
          }}
    {
    }

private:
    factory_t node_factory_;
    allocation_list_t allocation_list_;

    auto arena() const noexcept -> arena_t& { return allocation_list_.top().arena; }
};

struct scoped_node_t
{
    scoped_node_t* next;
    void* allocation;
};

template <typename node_t, typename allocation_deleter_t>
struct scoped_node_deleter_t
{
    auto operator()(node_t* node) const noexcept -> void
    {
        if (!node) return;
        allocation_deleter(node->allocation);
    }

    [[no_unique_address]] allocation_deleter_t allocation_deleter;
};

//! decorates allocator to track allocations internally
template <typename node_t, typename allocator_t, typename allocation_list_t>
class scoped_allocator_t
{
public:
    using allocation_t = void*;

    using allocated_node_t = allocation_list_t::allocated_node_t;

    struct pending_allocation_t
    {
        scoped_allocator_t* allocator;
        allocated_node_t new_node;

        auto result() const noexcept -> allocation_t { return std::to_address(new_node); }
        auto commit() noexcept -> void { allocator->commit(std::move(new_node)); }
    };

    auto reserve(std::size_t size, std::align_val_t align_val) -> pending_allocation_t
    {
        auto const alignment = static_cast<std::size_t>(align_val);
        auto const total_allocation_size
            = ((size + sizeof(node_t)) + (alignment - 1) + (alignof(node_t) - 1)) & -alignment;
        auto allocation = allocator_.allocate(total_allocation_size, align_val);

        auto const allocation_begin = std::to_address(allocation);
        auto const allocation_end = reinterpret_cast<std::uintptr_t>(allocation_begin) + size;
        auto const node_location = reinterpret_cast<void*>((allocation_end + alignof(node_t) - 1) & -alignof(node_t));

        auto allocated_node = allocated_node_t{
            new (node_location) node_t{nullptr, allocation_begin},
            typename allocated_node_t::deleter_type{std::move(allocation.get_deleter())}
        };
        allocation.release();
        return pending_allocation_t{this, std::move(allocated_node)};
    }

    auto commit(allocated_node_t new_node) noexcept -> void { allocation_list_.push(std::move(new_node)); }

    explicit scoped_allocator_t(allocator_t allocator, allocation_list_t allocation_list = allocation_list_t{}) noexcept
        : allocator_{std::move(allocator)}, allocation_list_{std::move(allocation_list)}
    {}

    scoped_allocator_t(scoped_allocator_t const&) = delete;
    auto operator=(scoped_allocator_t const&) -> scoped_allocator_t& = delete;

    scoped_allocator_t(scoped_allocator_t&&) = default;
    auto operator=(scoped_allocator_t&&) -> scoped_allocator_t& = default;

private:
    allocator_t allocator_{};
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
        thresholding_allocator_t* allocator;

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
        auto const worst_case_alignment_total_allocation_size = size + static_cast<std::size_t>(align_val) - 1;
        if (worst_case_alignment_total_allocation_size <= small_object_allocator_.max_allocation_size())
        {
            return pending_allocation_t{this, small_object_allocator_.reserve(size, align_val)};
        }

        return pending_allocation_t{this, large_object_allocator_.reserve(size, align_val)};
    }

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
    using heap_allocator_t = dink::heap_allocator_t<heap_allocation_deleter_t>;
    using arena_node_t = dink::arena_node_t<dink::arena_t>;
    using arena_node_factory_t = dink::arena_node_factory_t<
        arena_node_t, dink::arena_node_deleter_t<arena_node_t, heap_allocation_deleter_t>, heap_allocator_t,
        page_size_t>;
    using pooled_arena_allocator_policy_t = dink::pooled_arena_allocator_policy_t<arena_node_factory_t>;
    using pooled_arena_allocator_t = dink::pooled_arena_allocator_t<pooled_arena_allocator_policy_t>;
    using pooled_arena_allocator_ctor_params_t = pooled_arena_allocator_t ::ctor_params_t;
    using scoped_node_deleter_t = scoped_node_deleter_t<scoped_node_t, heap_allocation_deleter_t>;
    using scoped_allocator_t = dink::scoped_allocator_t<
        scoped_node_t, heap_allocator_t,
        allocation_list_t<
            scoped_node_t, scoped_node_deleter_t, node_list_deleter_t<scoped_node_t, scoped_node_deleter_t>>>;

    using small_object_allocator_t = pooled_arena_allocator_t;
    using large_object_allocator_t = scoped_allocator_t;
    using thresholding_allocator_t = dink::thresholding_allocator_t<small_object_allocator_t, large_object_allocator_t>;

    auto allocator = thresholding_allocator_t{
        small_object_allocator_t{pooled_arena_allocator_t{
            pooled_arena_allocator_ctor_params_t{arena_node_factory_t{heap_allocator_t{}, page_size_t{}}}
        }},
        large_object_allocator_t{heap_allocator_t()}
    };

    // successful case via small allocator
    std::cout << "testing allocations\n";
    try
    {
        std::cout << "reserve small allocation: ";
        auto pending_small_allocation = allocator.reserve(10, std::align_val_t{4});
        std::cout << "succeeded\n";

        std::cout << "commit small allocation: ";
        pending_small_allocation.commit();
        std::cout << "succeeded\n";

        std::cout << "reserve large allocation: ";
        auto pending_large_allocation = allocator.reserve(4096 * 32, std::align_val_t{4});
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
