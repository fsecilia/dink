/*!
    \file
    Copyright (C) 2025 Frank Secilia
    
    \brief metaprogramming utilities
*/

#pragma once

#include <dink/lib.hpp>

namespace dink::meta {

/*!
    constexpr false, but dependent on a template parameter to delay evaluation

    This is useful to trigger a static assert when the condition is already known to be false. The static assert
    message will contain the contents of the passed typename.
*/
template <typename>
inline constexpr auto dependent_false_v = false;

/*!
    constexpr bool, but dependent on a template parameter to delay evaluation

    This is useful to trigger a static assert conditionally. The static assert message will contain the contents of the
    passed typename.
*/
template <bool condition, typename>
inline constexpr auto dependent_bool_v = condition;

//! consumes an index to produce a type; useful for repeating type_t n times from an index sequence of length n
template <typename type_t, std::size_t index>
using indexed_type_t = type_t;

} // namespace dink::meta
