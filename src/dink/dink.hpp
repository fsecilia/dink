/*!
    \file
    Copyright (C) 2025 Frank Secilia
*/

#pragma once

namespace dink {

// determine how to express no_unique_address
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

} // namespace dink
