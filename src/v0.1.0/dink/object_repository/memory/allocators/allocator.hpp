/*!
    \file
    Copyright (C) 2025 Frank Secilia
*/

#pragma once

#include <dink/lib.hpp>
#include <concepts>
#include <cstddef>
#include <new>

namespace dink {

/*!
    provides allocate(), which returns an instance of member allocation_t and throws bad_alloc on failure
    
    This concept says nothing about the type of the allocation, nor does it require a deallocate method. Typically,
    either the allocation type is a smart pointer, or the allocator itself manages its own allocations internally.
*/
template <typename allocator_t>
concept allocator = requires(allocator_t allocator, std::size_t size, std::align_val_t align_val) {
    { allocator.allocate(size, align_val) } -> std::convertible_to<typename allocator_t::allocation_t>;
};

/*!
    provides reserve(), which returns a command to optionally commit() the allocation

    These allocators use the \ref pending_allocation pattern.
*/
template <typename allocator_t>
concept reservable_allocator = requires(allocator_t allocator, std::size_t size, std::align_val_t align_val) {
    { allocator.reserve(size, align_val) } -> std::destructible;

    requires requires(decltype(allocator.reserve(size, align_val)) reservation) {
        { reservation.allocation() } -> std::convertible_to<typename allocator_t::allocation_t>;
        { reservation.commit() } -> std::same_as<void>;
    };
};
} // namespace dink
