/*!
    \file
    Copyright (C) 2025 Frank Secilia
*/

#pragma once

#include <dink/lib.hpp>
#include <concepts>
#include <new>

namespace dink {

/*!
    provides allocate(), which returns an instance of member allocation_t and throws bad_alloc on failure
    
    This concept says nothing about the type of the allocation, nor does it require a deallocate method. Typically,
    either the allocation type is a smart pointer, or the allocator itself manages its own allocations internally.
*/
template <typename allocator_t>
concept allocator = requires(allocator_t allocator, std::size_t size, std::align_val_t align_val) {
    { allocator.allocate(size, align_val) } -> std::same_as<typename allocator_t::allocation_t>;
};

} // namespace dink
