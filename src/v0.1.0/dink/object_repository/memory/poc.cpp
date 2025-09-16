/*!
    \file
    Copyright (C) 2025 Frank Secilia
*/

#include <algorithm>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <new>
#include <optional>
#include <utility>

#include <iostream>

namespace dink {
namespace {

//! standin for more complex, platform-specific implementation; returns 4k pages
struct page_size_t
{
    auto operator()() const noexcept -> std::size_t { return 4096; }
};

//! aligned heap allocator that returns a unique_ptr with a stateless deleter
struct heap_allocator_t
{
    struct deleter_t
    {
        auto operator()(void* allocation) const noexcept { std::free(allocation); }
    };
    using allocation_t = std::unique_ptr<void, deleter_t>;

    auto allocate(std::size_t size, std::align_val_t align_val) const -> allocation_t
    {
        return allocation_t{std::aligned_alloc(size, static_cast<std::size_t>(align_val))};
    }
};

//! allocates from within a region of memory
template <typename storage_t>
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

    auto max_allocation_size() const noexcept -> std::size_t { return size_ / 8; }

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

    arena_t(storage_t storage, std::size_t size) : storage_{std::move(storage)}, size_{size} {}

    arena_t(arena_t const&) = delete;
    auto operator=(arena_t const&) -> arena_t& = delete;
    arena_t(arena_t&&) = default;
    auto operator=(arena_t&&) -> arena_t& = default;

private:
    storage_t storage_;
    std::size_t size_;

    using address_t = uintptr_t;
    address_t cur_{reinterpret_cast<address_t>(std::to_address(storage_))};
    address_t end_{cur_ + size_};
};

//! allocates arenas aligned to the os page size, in multiples of the os page size, using given allocator
template <typename arena_t, typename allocator_t, typename page_size_t>
class arena_factory_t
{
public:
    auto operator()() -> arena_t
    {
        return arena_t{allocator_.allocate(arena_size_, std::align_val_t{page_size_}), arena_size_};
    }

    arena_factory_t(allocator_t allocator, page_size_t page_size) noexcept
        : allocator_{std::move(allocator)}, page_size_{page_size()}
    {}

private:
    allocator_t allocator_;
    std::size_t page_size_;
    std::size_t arena_size_{page_size_ * 16};
};

//! configures types and ctor parameters for a pooled arena allocator
template <typename arena_p, typename arena_factory_p>
struct pooled_arena_allocator_config_t
{
    using arena_t = arena_p;
    using arena_factory_t = arena_factory_p;

    using arenas_t = std::vector<arena_t>;

    arena_factory_t arena_factory;
    arenas_t arenas = [this]() {
        auto result = arenas_t{};
        result.emplace_back(arena_factory());
        return result;
    }();
};

//! allocates from a pool of managed arenas
template <typename pooled_arena_allocator_config_t>
class pooled_arena_allocator_t
{
public:
    using arena_t = pooled_arena_allocator_config_t::arena_t;
    using arena_factory_t = pooled_arena_allocator_config_t::arena_factory_t;
    using arenas_t = pooled_arena_allocator_config_t::arenas_t;

    using allocation_t = arena_t::allocation_t;

    struct pending_allocation_t
    {
        pooled_arena_allocator_t* allocator;
        arena_t::pending_allocation_t arena_pending_allocation;
        std::optional<arena_t> new_arena;

        auto result() const noexcept -> allocation_t { return arena_pending_allocation.result(); }
        auto commit() noexcept -> void
        {
            allocator->commit(std::move(new_arena));
            arena_pending_allocation.commit();
        }
    };

    auto max_allocation_size() const noexcept -> std::size_t { return arenas_.back().max_allocation_size(); }

    auto reserve(std::size_t size, std::align_val_t align_val) -> pending_allocation_t
    {
        auto arena_pending_allocation = arena().reserve(size, align_val);
        if (arena_pending_allocation.allocation)
        {
            return pending_allocation_t{this, std::move(arena_pending_allocation), std::nullopt};
        }

        // grow vector capacity here so commit can be noexcept
        if (arenas_.size() == arenas_.capacity()) { arenas_.reserve(std::max(std::size_t{1}, arenas_.size() * 2)); }

        auto new_arena = arena_factory_();
        arena_pending_allocation = new_arena.reserve(size, align_val);
        return pending_allocation_t{this, std::move(arena_pending_allocation), std::move(new_arena)};
    }

    auto commit(std::optional<arena_t> new_arena) noexcept -> void
    {
        if (new_arena)
        {
            assert(arenas_.size() < arenas_.capacity());
            arenas_.emplace_back(std::move(*new_arena));
        }
    }

    explicit pooled_arena_allocator_t(pooled_arena_allocator_config_t pooled_arena_allocator_config)
        : arena_factory_{std::move(pooled_arena_allocator_config).arena_factory},
          arenas_{std::move(pooled_arena_allocator_config).arenas}
    {
        assert(!arenas_.empty());
    }

private:
    arena_factory_t arena_factory_;
    arenas_t arenas_;

    auto arena() noexcept -> arena_t& { return arenas_.back(); }
};

//! decorates allocator to track allocations internally
template <typename allocator_t, typename allocations_t = std::vector<typename allocator_t::allocation_t>>
class scoped_allocator_t
{
public:
    using allocation_t = void*;

    struct pending_allocation_t
    {
        scoped_allocator_t* allocator;
        allocator_t::allocation_t allocation;

        auto result() const noexcept -> allocation_t { return std::to_address(allocation); }
        auto commit() noexcept -> void { allocator->commit(std::move(allocation)); }
    };

    auto reserve(std::size_t size, std::align_val_t align_val) -> pending_allocation_t
    {
        // grow vector capacity here so commit can be noexcept
        if (allocations_.size() == allocations_.capacity())
        {
            allocations_.reserve(std::max(std::size_t{1}, allocations_.size() * 2));
        }

        return pending_allocation_t{this, allocator_.allocate(size, align_val)};
    }

    auto commit(allocator_t::allocation_t allocation) noexcept -> void
    {
        assert(allocations_.size() < allocations_.capacity());
        allocations_.emplace_back(std::move(allocation));
    }

    explicit scoped_allocator_t(allocator_t allocator, allocations_t allocations = {}) noexcept
        : allocator_{std::move(allocator)}, allocations_{std::move(allocations)}
    {}

    scoped_allocator_t(scoped_allocator_t const&) = delete;
    auto operator=(scoped_allocator_t const&) -> scoped_allocator_t& = delete;

    scoped_allocator_t(scoped_allocator_t&&) = default;
    auto operator=(scoped_allocator_t&&) -> scoped_allocator_t& = default;

private:
    allocator_t allocator_{};
    allocations_t allocations_{};
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
    using arena_t = dink::arena_t<heap_allocator_t::allocation_t>;
    using arena_factory_t = dink::arena_factory_t<arena_t, heap_allocator_t, page_size_t>;
    using pooled_arena_allocator_config_t = dink::pooled_arena_allocator_config_t<arena_t, arena_factory_t>;
    using pooled_arena_allocator_t = dink::pooled_arena_allocator_t<pooled_arena_allocator_config_t>;
    using scoped_allocator_t = dink::scoped_allocator_t<heap_allocator_t>;

    using small_object_allocator_t = pooled_arena_allocator_t;
    using large_object_allocator_t = scoped_allocator_t;
    using thresholding_allocator_t = dink::thresholding_allocator_t<small_object_allocator_t, large_object_allocator_t>;

    auto allocator = thresholding_allocator_t{
        small_object_allocator_t{pooled_arena_allocator_t{
            pooled_arena_allocator_config_t{arena_factory_t{heap_allocator_t{}, page_size_t{}}}
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
