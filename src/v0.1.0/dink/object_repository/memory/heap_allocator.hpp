/*!
    \file
    Copyright (C) 2025 Frank Secilia
*/

#pragma once

#include <dink/lib.hpp>
#include <dink/object_repository/memory/alignment.hpp>
#include <bit>
#include <cassert>
#include <memory>

namespace dink {

//! aligned heap allocator, returns unique_ptrs
struct heap_allocator_t
{
    //! deletes from heap using std::free
    struct deleter_t
    {
        auto operator()(void* allocation) const noexcept { std::free(allocation); }
    };

    //! unique_ptr with custom, stateless deleter
    using allocation_t = std::unique_ptr<void, deleter_t>;

    /*!
        allocates from heap using std::aligned_alloc

        \pre align_val is nonzero power of two
        \pre size is multiple of align_val
        \returns unique_ptr with stateless deleter
        \throws std::bad_alloc if allocation fails
    */
    auto allocate(std::size_t size, std::align_val_t align_val) const -> allocation_t
    {
        assert(is_properly_aligned(size, align_val));
        return allocation_t{std::aligned_alloc(size, static_cast<std::size_t>(align_val))};
    }
};

} // namespace dink
