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

// ---------------------------------------------------------------------------------------------------------------------

//! appends element to tuple if not already present
template <typename tuple_t, typename element_t>
using append_unique_t = std::conditional_t<contains_v<tuple_t, element_t>, tuple_t, append_t<tuple_t, element_t>>;

// ---------------------------------------------------------------------------------------------------------------------

template <typename tuple_t, typename element_t, std::size_t index>
struct index_of_f;

template <typename element_t, typename current_element_t, typename... remaining_elements_t, std::size_t index>
struct index_of_f<std::tuple<current_element_t, remaining_elements_t...>, element_t, index>
{
    static inline constexpr auto const value
        = index_of_f<std::tuple<remaining_elements_t...>, element_t, index + 1>::value;
};

template <typename element_t, typename... remaining_elements_t, std::size_t index>
struct index_of_f<std::tuple<element_t, remaining_elements_t...>, element_t, index>
{
    static inline constexpr auto const value = index;
};

template <typename element_t, std::size_t index>
struct index_of_f<std::tuple<>, element_t, index>
{
    static_assert(detail::dependent_false_v<element_t>, "tuple element not found");
};

//! index of element type in tuple_t if present; static asserts if not
template <typename tuple_t, typename element_t>
inline constexpr auto const index_of_v = index_of_f<tuple_t, element_t, 0>::value;

} // namespace dink::tuple
