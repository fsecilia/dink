/*!
    \file
    Copyright (C) 2025 Frank Secilia
*/

#pragma once

#include <dink/lib.hpp>
#include <bit>
#include <cassert>
#include <new>

namespace dink {

//! checks if align_val is a nonzero power of two
[[nodiscard]] constexpr auto is_valid_alignment(std::align_val_t align_val) noexcept -> bool
{
    return std::has_single_bit(static_cast<std::size_t>(align_val));
}

/*!
    checks if size is a multiple of align_val

    \pre align_val is a nonzero power of two
*/
[[nodiscard]] constexpr auto is_multiple_of_alignment(std::size_t size, std::align_val_t align_val) noexcept -> bool
{
    assert(is_valid_alignment(align_val));
    return (size & (static_cast<std::size_t>(align_val) - 1)) == 0;
}

/*!
    checks if a size and alignment pair form a valid request for an aligned allocation

    The strictest alignment requirement comes from std::aligned_alloc, which requires alignment to be a nonzero power
    of two, and size to be a multiple of alignment.

    \pre align_val is a nonzero power of two
*/
[[nodiscard]] constexpr auto is_properly_aligned(std::size_t size, std::align_val_t align_val) noexcept -> bool
{
    return is_valid_alignment(align_val) && is_multiple_of_alignment(size, align_val);
}

} // namespace dink
