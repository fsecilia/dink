/*!
    \file
    Copyright (C) 2025 Frank Secilia
*/

#pragma once

#include <dink/lib.hpp>
#include <dink/not_found.hpp>
#include <type_traits>

namespace dink {

//! lightweight, compile-time tuple
template <typename... types_t>
struct type_list_t;

// ---------------------------------------------------------------------------------------------------------------------

namespace type_list {

template <typename type_list_t, typename element_t>
struct append_f;

template <typename element_t, typename... elements_t>
struct append_f<type_list_t<elements_t...>, element_t> {
    using type = type_list_t<elements_t..., element_t>;
};

//! appends element to type list
template <typename type_list_t, typename element_t>
using append_t = append_f<type_list_t, element_t>::type;

// ---------------------------------------------------------------------------------------------------------------------

template <typename type_list_t, typename element_t>
struct contains_f;

template <typename element_t, typename... elements_t>
struct contains_f<type_list_t<elements_t...>, element_t> : std::disjunction<std::is_same<element_t, elements_t>...> {};

//! true if type list has an element of type element_t
template <typename type_list_t, typename element_t>
inline constexpr auto contains_v = contains_f<type_list_t, element_t>::value;


}  // namespace type_list
}  // namespace dink
