/*!
    \file
    Copyright (C) 2025 Frank Secilia
*/

#pragma once

#include <dink/lib.hpp>
#include <dink/scope/config/detail/tuple.hpp>
#include <tuple>

namespace dink {

template <typename tuple_t>
struct keys_f;

template <typename... elements_t>
struct keys_f<std::tuple<elements_t...>>
{
    using type = std::tuple<typename elements_t::key_t...>;
};

//! projects to new tuple containing only key_t member of each element
template <typename tuple_t>
using keys_t = typename keys_f<tuple_t>::type;

} // namespace dink
