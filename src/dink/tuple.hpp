/*!
    \file
    Copyright (C) 2025 Frank Secilia
*/

#pragma once

#include <dink/lib.hpp>

namespace dink::tuple {

namespace detail {

// constexpr false, but dependent on a template parameter to delay evaluation
template <typename>
inline constexpr auto const dependent_false_v = false;

} // namespace detail

} // namespace dink::tuple
