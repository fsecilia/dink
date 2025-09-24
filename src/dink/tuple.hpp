/*!
    \file
    Copyright (C) 2025 Frank Secilia
*/

#pragma once

#include <dink/lib.hpp>
#include <tuple>
#include <type_traits>

namespace dink::tuple {

namespace detail {

// constexpr false, but dependent on a template parameter to delay evaluation
template <typename>
inline constexpr auto const dependent_false_v = false;

} // namespace detail

// ---------------------------------------------------------------------------------------------------------------------

template <typename tuple_t, typename element_t>
struct contains_f;

template <typename element_t, typename... elements_t>
struct contains_f<std::tuple<elements_t...>, element_t> : std::disjunction<std::is_same<element_t, elements_t>...>
{};

//! true if tuple_t has an element of type element_t
template <typename tuple_t, typename element_t>
inline constexpr auto const contains_v = contains_f<tuple_t, element_t>::value;

// ---------------------------------------------------------------------------------------------------------------------

template <typename tuple_t, typename element_t>
struct append_f;

template <typename element_t, typename... elements_t>
struct append_f<std::tuple<elements_t...>, element_t>
{
    using type = std::tuple<elements_t..., element_t>;
};

//! appends element to tuple
template <typename tuple_t, typename element_t>
using append_t = append_f<tuple_t, element_t>::type;

} // namespace dink::tuple
