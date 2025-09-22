/*!
    \file
    Copyright (C) 2025 Frank Secilia
*/

#pragma once

#include <dink/lib.hpp>
#include <bit>
#include <cassert>
#include <cstddef>
#include <new>

namespace dink {

//! checks if align_val is a nonzero power of two
constexpr auto is_valid_alignment(std::align_val_t align_val) noexcept -> bool
{
    return std::has_single_bit(static_cast<std::size_t>(align_val));
}

/*!
    checks if offset is aligned to align_val

    \pre align_val is a nonzero power of two
*/
constexpr auto is_aligned(uintptr_t offset, std::align_val_t align_val) noexcept -> bool
{
    assert(is_valid_alignment(align_val));
    auto const alignment = static_cast<uintptr_t>(align_val);
    return !(offset & (alignment - 1));
}

/*!
    checks if address is aligned to align_val

    \pre align_val is a nonzero power of two
*/
auto is_aligned(auto* address, std::align_val_t align_val) noexcept -> bool
{
    return is_aligned(reinterpret_cast<uintptr_t>(address), align_val);
}

/*!
    aligns offset to align_val

    \pre align_val is a nonzero power of two
*/
[[nodiscard]] constexpr auto align(uintptr_t offset, std::align_val_t align_val) noexcept -> uintptr_t
{
    assert(is_valid_alignment(align_val));
    auto const alignment = static_cast<uintptr_t>(align_val);
    return (offset + alignment - 1) & -alignment;
}

/*!
    aligns address to align_val

    \pre align_val is a nonzero power of two
*/
[[nodiscard]] auto align(auto* address, std::align_val_t align_val) noexcept -> decltype(address)
{
    return reinterpret_cast<decltype(address)>(align(reinterpret_cast<uintptr_t>(address), align_val));
}

} // namespace dink
