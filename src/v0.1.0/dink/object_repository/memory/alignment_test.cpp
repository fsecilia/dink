/*!
    \file
    Copyright (C) 2025 Frank Secilia
*/

#include "alignment.hpp"
#include <dink/test.hpp>
#include <limits>

namespace dink {
namespace {

constexpr auto const align_val = std::align_val_t{16};

namespace is_valid_alignment_tests {

// 0 is a boundary case
static_assert(!is_valid_alignment(std::align_val_t{0}));

// smallest valid power-of-two alignment
static_assert(is_valid_alignment(std::align_val_t{1}));

// common, small power-of-two
static_assert(is_valid_alignment(std::align_val_t{2}));

// larger, more typical power-of-two
static_assert(is_valid_alignment(std::align_val_t{64}));

// largest possible power-of-two for size_t
static_assert(is_valid_alignment(std::align_val_t{std::size_t{1} << (std::numeric_limits<std::size_t>::digits - 1)}));

// values immediately adjacent to a power of two
static_assert(!is_valid_alignment(std::align_val_t{3}));
static_assert(!is_valid_alignment(std::align_val_t{15}));
static_assert(!is_valid_alignment(std::align_val_t{17}));
static_assert(!is_valid_alignment(std::align_val_t{63}));
static_assert(!is_valid_alignment(std::align_val_t{65}));

} // namespace is_valid_alignment_tests

namespace is_aligned_tests {

// 0 is always aligned
static_assert(is_aligned(0, align_val));

// all inputs are aligned to 1
static_assert(is_aligned(0, std::align_val_t{1}));
static_assert(is_aligned(1, std::align_val_t{1}));
static_assert(is_aligned(2, std::align_val_t{1}));

// an offset equal to the alignment is aligned
static_assert(is_aligned(16, align_val));
static_assert(is_aligned(32, align_val));

// boundaries immediately adjacent to an aligned value
static_assert(!is_aligned(15, align_val));
static_assert(!is_aligned(17, align_val));

// smallest unaligned value
static_assert(!is_aligned(1, align_val));

} // namespace is_aligned_tests

// the address version is implemented in terms of the offset version, so this just tests that it converts correctly
struct is_aligned_test_t : Test
{
    static inline constexpr auto const alignment = 2;
    alignas(alignment) char data[2];
};

TEST_F(is_aligned_test_t, aligned)
{
    ASSERT_TRUE(is_aligned(&data[0], std::align_val_t{alignment}));
}

TEST_F(is_aligned_test_t, unaligned)
{
    ASSERT_FALSE(is_aligned(&data[1], std::align_val_t{alignment}));
}

namespace align_tests {

// 0 is always aligned
static_assert(align(0, align_val) == 0);

// all inputs are aligned to 1
static_assert(align(0, std::align_val_t{1}) == 0);
static_assert(align(2, std::align_val_t{1}) == 2);

// offset that is already aligned.
static_assert(align(16, align_val) == 16);

// boundaries immediately adjacent to an aligned region
static_assert(align(1, align_val) == 16);
static_assert(align(15, align_val) == 16);

// offset at the max of the range to check for overflow.
static_assert(
    align(std::numeric_limits<uintptr_t>::max(), std::align_val_t{1}) == std::numeric_limits<uintptr_t>::max()
);
static_assert(align(std::numeric_limits<uintptr_t>::max(), align_val) == 0);

} // namespace align_tests

// the address version is implemented in terms of the offset version, so this just tests that it converts correctly
struct align_test_t : Test
{
    static inline constexpr auto const alignment = 2;
    alignas(alignment) char data[2];
};

TEST_F(align_test_t, aligned)
{
    ASSERT_EQ(&data[0], align(&data[0], std::align_val_t{alignment}));
}

TEST_F(align_test_t, unaligned)
{
    ASSERT_EQ(&data[0] + alignment, align(&data[1], std::align_val_t{alignment}));
}

} // namespace
} // namespace dink
