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
struct tuple_contains_f;

template <typename element_t, typename... elements_t>
struct tuple_contains_f<std::tuple<elements_t...>, element_t> : std::disjunction<std::is_same<element_t, elements_t>...>
{};

template <typename tuple_t, typename element_t>
inline constexpr auto const tuple_contains_v = tuple_contains_f<tuple_t, element_t>::value;

template <typename tuple_t, typename element_t>
struct tuple_append_f;

template <typename element_t, typename... elements_t>
struct tuple_append_f<std::tuple<elements_t...>, element_t>
{
    using type = std::tuple<elements_t..., element_t>;
};

template <typename tuple_t, typename element_t>
using tuple_append_t = tuple_append_f<tuple_t, element_t>::type;

template <typename accumulator_tuple_t, typename remaining_tuple_t>
struct tuple_unique_f;

template <typename accumulator_tuple_t>
struct tuple_unique_f<accumulator_tuple_t, std::tuple<>>
{
    using type = accumulator_tuple_t;
};

template <typename accumulator_tuple_t, typename next_t, typename... remaining_t>
struct tuple_unique_f<accumulator_tuple_t, std::tuple<next_t, remaining_t...>>
{
    using type = tuple_unique_f<
        std::conditional_t<
            tuple_contains_v<accumulator_tuple_t, next_t>, accumulator_tuple_t,
            tuple_append_t<accumulator_tuple_t, next_t>>,
        std::tuple<remaining_t...>>::type;
};

template <typename tuple_t>
using tuple_unique_t = tuple_unique_f<std::tuple<>, tuple_t>::type;

} // namespace dink::detail::tuple
