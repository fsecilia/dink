/*!
    \file
    Copyright (C) 2025 Frank Secilia
*/

#include "alignment.hpp"
#include <dink/test.hpp>

namespace dink {
namespace {

namespace is_valid_alignment_tests {

// valid alignments
static_assert(is_valid_alignment(std::align_val_t{1}));
static_assert(is_valid_alignment(std::align_val_t{2}));
static_assert(is_valid_alignment(std::align_val_t{4}));
static_assert(is_valid_alignment(std::align_val_t{8}));
static_assert(is_valid_alignment(std::align_val_t{16}));
static_assert(is_valid_alignment(std::align_val_t{64}));
static_assert(is_valid_alignment(std::align_val_t{1024}));

// invalid alignments
static_assert(!is_valid_alignment(std::align_val_t{0}));
static_assert(!is_valid_alignment(std::align_val_t{3}));
static_assert(!is_valid_alignment(std::align_val_t{5}));
static_assert(!is_valid_alignment(std::align_val_t{6}));
static_assert(!is_valid_alignment(std::align_val_t{7}));
static_assert(!is_valid_alignment(std::align_val_t{9}));
static_assert(!is_valid_alignment(std::align_val_t{15}));
static_assert(!is_valid_alignment(std::align_val_t{17}));
static_assert(!is_valid_alignment(std::align_val_t{63}));
static_assert(!is_valid_alignment(std::align_val_t{65}));
static_assert(!is_valid_alignment(std::align_val_t{1023}));
static_assert(!is_valid_alignment(std::align_val_t{1025}));

} // namespace is_valid_alignment_tests

namespace is_multiple_of_alignment_tests {

// size is a multiple
static_assert(is_multiple_of_alignment(2, std::align_val_t{2}));
static_assert(is_multiple_of_alignment(4, std::align_val_t{2}));
static_assert(is_multiple_of_alignment(6, std::align_val_t{2}));
static_assert(is_multiple_of_alignment(8, std::align_val_t{2}));
static_assert(is_multiple_of_alignment(4, std::align_val_t{4}));
static_assert(is_multiple_of_alignment(8, std::align_val_t{4}));
static_assert(is_multiple_of_alignment(12, std::align_val_t{4}));
static_assert(is_multiple_of_alignment(16, std::align_val_t{4}));
static_assert(is_multiple_of_alignment(1024, std::align_val_t{1024}));
static_assert(is_multiple_of_alignment(2048, std::align_val_t{1024}));

// size is not a multiple
static_assert(!is_multiple_of_alignment(1, std::align_val_t{2}));
static_assert(!is_multiple_of_alignment(3, std::align_val_t{2}));
static_assert(!is_multiple_of_alignment(5, std::align_val_t{2}));
static_assert(!is_multiple_of_alignment(7, std::align_val_t{2}));
static_assert(!is_multiple_of_alignment(9, std::align_val_t{2}));
static_assert(!is_multiple_of_alignment(1, std::align_val_t{4}));
static_assert(!is_multiple_of_alignment(2, std::align_val_t{4}));
static_assert(!is_multiple_of_alignment(3, std::align_val_t{4}));
static_assert(!is_multiple_of_alignment(5, std::align_val_t{4}));
static_assert(!is_multiple_of_alignment(6, std::align_val_t{4}));
static_assert(!is_multiple_of_alignment(7, std::align_val_t{4}));
static_assert(!is_multiple_of_alignment(9, std::align_val_t{4}));
static_assert(!is_multiple_of_alignment(1, std::align_val_t{1024}));
static_assert(!is_multiple_of_alignment(512, std::align_val_t{1024}));
static_assert(!is_multiple_of_alignment(1023, std::align_val_t{1024}));
static_assert(!is_multiple_of_alignment(1025, std::align_val_t{1024}));
static_assert(!is_multiple_of_alignment(2047, std::align_val_t{1024}));
static_assert(!is_multiple_of_alignment(2049, std::align_val_t{1024}));

// size is 0
static_assert(is_multiple_of_alignment(0, std::align_val_t{1}));
static_assert(is_multiple_of_alignment(0, std::align_val_t{2}));
static_assert(is_multiple_of_alignment(0, std::align_val_t{4}));
static_assert(is_multiple_of_alignment(0, std::align_val_t{1024}));

// alignment is 1
static_assert(is_multiple_of_alignment(0, std::align_val_t{1}));
static_assert(is_multiple_of_alignment(1, std::align_val_t{1}));
static_assert(is_multiple_of_alignment(2, std::align_val_t{1}));
static_assert(is_multiple_of_alignment(3, std::align_val_t{1}));
static_assert(is_multiple_of_alignment(4, std::align_val_t{1}));
static_assert(is_multiple_of_alignment(5, std::align_val_t{1}));
static_assert(is_multiple_of_alignment(1023, std::align_val_t{1}));
static_assert(is_multiple_of_alignment(1024, std::align_val_t{1}));
static_assert(is_multiple_of_alignment(1025, std::align_val_t{1}));

} // namespace is_multiple_of_alignment_tests

namespace is_valid_aligned_request_tests {

// valid request, is power of two and is multiple
static_assert(is_valid_aligned_request(0, static_cast<std::align_val_t>(1)));
static_assert(is_valid_aligned_request(7, static_cast<std::align_val_t>(1)));
static_assert(is_valid_aligned_request(8, static_cast<std::align_val_t>(8)));
static_assert(is_valid_aligned_request(32, static_cast<std::align_val_t>(16)));

// invalid alignment, is not power of two but is multiple
static_assert(!is_valid_aligned_request(6, static_cast<std::align_val_t>(3)));
static_assert(!is_valid_aligned_request(30, static_cast<std::align_val_t>(15)));

// invalid alignment, is power of two but is not multiple
static_assert(!is_valid_aligned_request(9, static_cast<std::align_val_t>(8)));
static_assert(!is_valid_aligned_request(63, static_cast<std::align_val_t>(32)));

// invalid alignment, is not power of two and is not multiple
static_assert(!is_valid_aligned_request(10, static_cast<std::align_val_t>(6)));
static_assert(!is_valid_aligned_request(20, static_cast<std::align_val_t>(7)));

} // namespace is_valid_aligned_request_tests

namespace is_aligned_tests {

// aligned values
static_assert(is_aligned(2, std::align_val_t{2}));
static_assert(is_aligned(4, std::align_val_t{2}));
static_assert(is_aligned(16, std::align_val_t{16}));
static_assert(is_aligned(32, std::align_val_t{16}));

// unaligned values
static_assert(!is_aligned(1, std::align_val_t{2}));
static_assert(!is_aligned(3, std::align_val_t{2}));
static_assert(!is_aligned(1, std::align_val_t{16}));
static_assert(!is_aligned(8, std::align_val_t{16}));
static_assert(!is_aligned(15, std::align_val_t{16}));
static_assert(!is_aligned(17, std::align_val_t{16}));
static_assert(!is_aligned(31, std::align_val_t{16}));
static_assert(!is_aligned(33, std::align_val_t{16}));

// 0 is valid for all alignments
static_assert(is_aligned(0, std::align_val_t{1}));
static_assert(is_aligned(0, std::align_val_t{2}));
static_assert(is_aligned(0, std::align_val_t{16}));

// all values are valid when alignment is 1
static_assert(is_aligned(1, std::align_val_t{1}));
static_assert(is_aligned(2, std::align_val_t{1}));
static_assert(is_aligned(3, std::align_val_t{1}));

} // namespace is_aligned_tests

// the address version is implemented in terms of the offset version, so this just tests that it converts correctly
struct is_aligned_test_t : Test
{
    // guarantee offset of data is aligned
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

// offset is not aligned
static_assert(align(1, std::align_val_t{2}) == 2);
static_assert(align(3, std::align_val_t{2}) == 4);
static_assert(align(1, std::align_val_t{16}) == 16);
static_assert(align(15, std::align_val_t{16}) == 16);
static_assert(align(17, std::align_val_t{16}) == 32);
static_assert(align(31, std::align_val_t{16}) == 32);

// offset is aligned
static_assert(align(0, std::align_val_t{2}) == 0);
static_assert(align(2, std::align_val_t{2}) == 2);
static_assert(align(4, std::align_val_t{2}) == 4);
static_assert(align(0, std::align_val_t{16}) == 0);
static_assert(align(16, std::align_val_t{16}) == 16);
static_assert(align(32, std::align_val_t{16}) == 32);

// offset is 0
static_assert(align(0, std::align_val_t{2}) == 0);
static_assert(align(0, std::align_val_t{16}) == 0);

// alignment is 1
static_assert(align(0, std::align_val_t{1}) == 0);
static_assert(align(1, std::align_val_t{1}) == 1);
static_assert(align(2, std::align_val_t{1}) == 2);
static_assert(align(3, std::align_val_t{1}) == 3);

// offset at max of range
static_assert(
    align(std::numeric_limits<uintptr_t>::max(), std::align_val_t{1}) == std::numeric_limits<uintptr_t>::max()
);
static_assert(align(std::numeric_limits<uintptr_t>::max(), std::align_val_t{2}) == 0);
static_assert(align(std::numeric_limits<uintptr_t>::max(), std::align_val_t{16}) == 0);

} // namespace align_tests

// the address version is implemented in terms of the offset version, so this just tests that it converts correctly
struct align_test_t : Test
{
    // guarantee offset of data is aligned
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
