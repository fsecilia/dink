/*!
    \file
    Copyright (C) 2025 Frank Secilia
    
    \brief metaprogramming utilities
*/

#pragma once

#include <dink/lib.hpp>

namespace dink::meta {

// constexpr false, but dependent on a template parameter to delay evaluation
template <typename>
inline constexpr auto const dependent_false_v = false;

// consumes an index to produce a type; useful for repeating type_t n times from an index sequence of length n
template <typename type_t, std::size_t index>
using indexed_type_t = type_t;

} // namespace dink::meta
