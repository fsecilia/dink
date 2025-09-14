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

namespace is_properly_aligned_tests {

// valid alignment, is multiple
static_assert(is_properly_aligned(0, static_cast<std::align_val_t>(1)));
static_assert(is_properly_aligned(7, static_cast<std::align_val_t>(1)));
static_assert(is_properly_aligned(8, static_cast<std::align_val_t>(8)));
static_assert(is_properly_aligned(32, static_cast<std::align_val_t>(16)));

// invalid alignment, is multiple
static_assert(!is_properly_aligned(6, static_cast<std::align_val_t>(3)));
static_assert(!is_properly_aligned(30, static_cast<std::align_val_t>(15)));

// valid alignment, is not multiple
static_assert(!is_properly_aligned(9, static_cast<std::align_val_t>(8)));
static_assert(!is_properly_aligned(63, static_cast<std::align_val_t>(32)));

// invalid alignment, is not multiple
static_assert(!is_properly_aligned(10, static_cast<std::align_val_t>(6)));
static_assert(!is_properly_aligned(20, static_cast<std::align_val_t>(7)));

} // namespace is_properly_aligned_tests

} // namespace
} // namespace dink
