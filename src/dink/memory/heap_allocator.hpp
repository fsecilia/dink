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

//! supports allocate and deallocate
template <typename heap_allocator_t>
concept heap_allocator = requires(
    heap_allocator_t heap_allocator, size_t size, std::align_val_t alignment, void* allocation
) {
    { heap_allocator.allocate(size, alignment) } -> std::same_as<void*>;
    { heap_allocator.deallocate(allocation) };
};

//! allocates from heap using new and delete
class heap_allocator_t
{
public:
    auto allocate(std::size_t size, std::align_val_t alignment) const -> void*
    {
        return new (alignment) std::byte[size];
    }

    auto deallocate(void* allocation) noexcept -> void { delete[] static_cast<std::byte*>(allocation); }
};

} // namespace dink
