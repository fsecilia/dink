/*!
    \file
    Copyright (C) 2025 Frank Secilia
*/

#pragma once

#include <dink/lib.hpp>
#include <dink/memory/append_only_heap_allocator.hpp>
#include <dink/memory/paged_sub_allocator.hpp>

namespace dink {

template <typename large_object_allocator_t>
concept large_object_allocator = append_only_heap_allocator<large_object_allocator_t>;

template <typename large_object_allocator_t>
concept small_object_allocator = paged_sub_allocator<large_object_allocator_t>;

//! append-only arena allocator with small object optimization
template <large_object_allocator large_object_allocator_t, small_object_allocator small_object_allocator_t>
class arena_allocator_t
{
public:
    /*!
        threshold to choose when to fall back to the large object allocator
        
        Allocations with effective sizes greater than this are serviced by the large object allocator. The rest use the
        faster small object allocator.
    */
    std::size_t const small_object_threshold = small_object_allocator_.max_allocation_size();

    /*!
        /pre alignment is nonzero power of two
    */
    auto allocate(std::size_t size, std::align_val_t alignment) -> void*
    {
        // enforce alignment precondition
        auto const alignment_value = static_cast<std::size_t>(alignment);
        assert(std::has_single_bit(alignment_value));

        // calculate the largest allocation size given the worst possible alignment
        auto const worst_case_alignment_padding = alignment_value - 1;
        auto const effective_allocation_size = size + worst_case_alignment_padding;

        // delegate based on effective allocation size
        if (effective_allocation_size > small_object_threshold)
        {
            return large_object_allocator_.allocate(size, alignment);
        }
        return small_object_allocator_.allocate(size, alignment);
    }

    arena_allocator_t(
        large_object_allocator_t large_object_allocator, small_object_allocator_t small_object_allocator
    ) noexcept
        : large_object_allocator_{std::move(large_object_allocator)},
          small_object_allocator_{std::move(small_object_allocator)}
    {}

private:
    large_object_allocator_t large_object_allocator_{};
    small_object_allocator_t small_object_allocator_{};
};

} // namespace dink
