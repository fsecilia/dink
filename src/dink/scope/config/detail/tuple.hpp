/*!
    \file
    Copyright (C) 2025 Frank Secilia
*/

#pragma once

#include <dink/lib.hpp>
#include <tuple>
#include <type_traits>

namespace dink::detail::tuple {

template <typename tuple_t, typename element_t>
struct contains_f;

template <typename element_t, typename... elements_t>
struct contains_f<std::tuple<elements_t...>, element_t> : std::disjunction<std::is_same<element_t, elements_t>...>
{};

//! true if tuple_t has an element of type element_t
template <typename tuple_t, typename element_t>
inline constexpr auto const contains_v = contains_f<tuple_t, element_t>::value;

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

template <typename accumulator_tuple_t, typename remaining_tuple_t>
struct unique_f;

template <typename accumulator_tuple_t>
struct unique_f<accumulator_tuple_t, std::tuple<>>
{
    using type = accumulator_tuple_t;
};

template <typename accumulator_tuple_t, typename next_element_t, typename... remaining_elements_t>
struct unique_f<accumulator_tuple_t, std::tuple<next_element_t, remaining_elements_t...>>
{
    using type = unique_f<
        std::conditional_t<
            contains_v<accumulator_tuple_t, next_element_t>, accumulator_tuple_t,
            append_t<accumulator_tuple_t, next_element_t>>,
        std::tuple<remaining_elements_t...>>::type;
};

//! removes repeated elements
template <typename tuple_t>
using unique_t = unique_f<std::tuple<>, tuple_t>::type;

} // namespace dink::detail::tuple
