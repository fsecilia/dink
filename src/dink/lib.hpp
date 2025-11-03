// \file
// Copyright (c) 2025 Frank Secilia
// SPDX-License-Identifier: MIT
//
// \brief Common header included by every file in the library.

#pragma once

#include <dink/config.gen.hpp>
#include <cstdint>

//!
namespace dink {

// ----------------------------------------------------------------------------
// dink_no_unique_address
// ----------------------------------------------------------------------------

// clang-format off

#if !defined dink_no_unique_address
  #if __has_cpp_attribute(no_unique_address) >= 201803L
    // __has_cpp_attribute works on all supported compilers if the version is
    // 201803L or newer.
    #define dink_no_unique_address no_unique_address
  #elif !defined __clang__ && defined _MSC_VER && _MSC_VER >= 1929
    // MSVC first defined their own name for it in v1929, but there is no
    // support from __has_cpp_attribute.
    #define dink_no_unique_address msvc::no_unique_address
  #else
    // It is unsupported otherwise.
    #define dink_no_unique_address
  #endif
#endif

// clang-format on

// ----------------------------------------------------------------------------
// processor word size ints
//
// Default integer types are the same size as the processor word.
// ----------------------------------------------------------------------------

using int_t = std::intptr_t;
using uint_t = std::uintptr_t;

}  // namespace dink
