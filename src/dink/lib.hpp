/*!
    \file
    Copyright (C) 2025 Frank Secilia
*/

#pragma once

#include <dink/config.generated.hpp>
#include <cstdint>

namespace dink {

// clang-format off
#if defined __has_cpp_attribute
    #if __has_cpp_attribute(msvc::no_unique_address)
        #define dink_no_unique_address msvc::no_unique_address
    #elif __has_cpp_attribute(no_unique_address)
        #define dink_no_unique_address no_unique_address
    #endif
#endif
#if !defined dink_no_unique_address
    #define dink_no_unique_address
#endif
// clang-format on

using int_t = std::intptr_t;

} // namespace dink
