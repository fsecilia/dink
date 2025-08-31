/*!
    \file
    Copyright (C) 2025 Frank Secilia
*/

#pragma once

#include <dink/lib.hpp>
#include <concepts>
#include <cstddef>
#include <memory>
#include <new>

namespace dink {

//! callable that takes size and alignment, then returns unique_ptr<byte[]>
template <typename array_allocator_t>
concept array_allocator = requires(array_allocator_t array_allocator, size_t size, std::align_val_t alignment) {
    { array_allocator(size, alignment) } -> std::same_as<std::unique_ptr<std::byte[]>>;
};

//! allocates arrays using new
class array_allocator_t
{
public:
    using dynamic_array_t = std::unique_ptr<std::byte[]>;
    auto operator()(std::size_t size, std::align_val_t alignment) const -> dynamic_array_t
    {
        return dynamic_array_t{new (alignment) std::byte[size]};
    }
};

} // namespace dink
