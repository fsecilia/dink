/*!
    \file
    Copyright (C) 2025 Frank Secilia
*/

#pragma once

#include <dink/lib.hpp>
#include <dink/memory/heap_allocator.hpp>
#include <concepts>
#include <cstddef>
#include <new>
#include <vector>

namespace dink {

//! supports allocate, but deallocate is not required
template <typename heap_allocator_t>
concept append_only_heap_allocator = requires(
    heap_allocator_t heap_allocator, size_t size, std::align_val_t alignment
) {
    { heap_allocator.allocate(size, alignment) } -> std::same_as<void*>;
    { heap_allocator.roll_back() };
};

//! heap allocator that supports allocate, but manages lifetimes internally, so results should not be deallocated
template <heap_allocator heap_allocator_t, typename allocations_container_t = std::vector<void*>>
class append_only_heap_allocator_t
{
public:
    //! allocates and tracks pointers internally; underlying throws std::bad_alloc if allocation fails
    auto allocate(std::size_t size, std::align_val_t alignment) -> void*
    {
        auto allocation = heap_allocator_.allocate(size, alignment);
        try
        {
            return allocations_.emplace_back(allocation);
        }
        catch (...)
        {
            heap_allocator_.deallocate(allocation);
            throw;
        }
    }

    //! rolls back last allocation, if any
    auto roll_back() noexcept -> void
    {
        if (allocations_.empty()) return;

        heap_allocator_.deallocate(allocations_.back());
        allocations_.pop_back();
    }

    explicit append_only_heap_allocator_t(
        heap_allocator_t heap_allocator, allocations_container_t allocations = {}
    ) noexcept
        : heap_allocator_{std::move(heap_allocator)}, allocations_{std::move(allocations)}
    {}

    ~append_only_heap_allocator_t()
    {
        for (auto allocation : allocations_) heap_allocator_.deallocate(allocation);
    }

    append_only_heap_allocator_t(append_only_heap_allocator_t const&) = delete;
    auto operator=(append_only_heap_allocator_t const&) -> append_only_heap_allocator_t& = delete;

    append_only_heap_allocator_t(append_only_heap_allocator_t&&) = default;
    auto operator=(append_only_heap_allocator_t&&) -> append_only_heap_allocator_t& = default;

private:
    heap_allocator_t heap_allocator_{};
    allocations_container_t allocations_{};
};

} // namespace dink
