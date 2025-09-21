/*!
    \file
    Copyright (C) 2025 Frank Secilia
*/

#pragma once

#include <dink/lib.hpp>
#include <dink/object_repository/memory/alignment.hpp>
#include <dink/object_repository/memory/allocators/allocator.hpp>
#include <cassert>
#include <cstddef>
#include <utility>
#include <vector>

namespace dink {

//! decorates an allocator to manage lifetimes internally
template <allocator allocator_t, typename allocations_t = std::vector<typename allocator_t::allocation_t>>
struct scoped_allocator_t
{
    using allocation_t = void*;

    /*!
        allocates and tracks pointers internally; underlying throws std::bad_alloc if allocation fails

        \pre align_val is nonzero power of two
        \pre size is multiple of align_val
    */
    auto allocate(std::size_t size, std::align_val_t align_val) -> allocation_t
    {
        assert(is_valid_aligned_request(size, align_val));
        return allocations_.emplace_back(allocator_.allocate(size, align_val)).get();
    }

    //! rolls back most recent allocation, if one exists
    auto roll_back() noexcept -> void
    {
        if (!allocations_.empty()) allocations_.pop_back();
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

} // namespace dink
