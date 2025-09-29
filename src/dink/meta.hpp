/*!
    \file
    Copyright (C) 2025 Frank Secilia
    
    \brief metaprogramming utilities
*/

#pragma once

#include <dink/lib.hpp>

namespace dink::meta {

//! constexpr false, but dependent on a template parameter to delay evaluation
template <typename>
inline constexpr auto dependent_false_v = false;

} // namespace dink::meta
