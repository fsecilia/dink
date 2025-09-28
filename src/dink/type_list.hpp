/*!
    \file
    Copyright (C) 2025 Frank Secilia
*/

#pragma once

#include <dink/lib.hpp>
#include <dink/meta.hpp>
#include <type_traits>

namespace dink {

//! lightweight, compile-time tuple
template <typename... types_t>
struct type_list_t;

// ---------------------------------------------------------------------------------------------------------------------

namespace type_list {

template <typename type_list_t, typename element_t>
struct contains_f;

template <typename element_t, typename... elements_t>
struct contains_f<type_list_t<elements_t...>, element_t> : std::disjunction<std::is_same<element_t, elements_t>...>
{};

//! true if type list has an element of type element_t
template <typename type_list_t, typename element_t>
inline constexpr auto const contains_v = contains_f<type_list_t, element_t>::value;

// ---------------------------------------------------------------------------------------------------------------------

template <typename type_list_t, typename element_t>
struct append_f;

template <typename element_t, typename... elements_t>
struct append_f<type_list_t<elements_t...>, element_t>
{
    using type = type_list_t<elements_t..., element_t>;
};

//! appends element to type list
template <typename type_list_t, typename element_t>
using append_t = append_f<type_list_t, element_t>::type;

// ---------------------------------------------------------------------------------------------------------------------

//! conditionally appends element to type list if not already present
template <typename type_list_t, typename element_t>
using append_unique_t
    = std::conditional_t<contains_v<type_list_t, element_t>, type_list_t, append_t<type_list_t, element_t>>;

// ---------------------------------------------------------------------------------------------------------------------

template <typename type_list_t, typename element_t, std::size_t index>
struct index_of_f;

template <typename element_t, typename current_element_t, typename... remaining_elements_t, std::size_t index>
struct index_of_f<type_list_t<current_element_t, remaining_elements_t...>, element_t, index>
{
    static inline constexpr auto const value
        = index_of_f<type_list_t<remaining_elements_t...>, element_t, index + 1>::value;
};

template <typename element_t, typename... remaining_elements_t, std::size_t index>
struct index_of_f<type_list_t<element_t, remaining_elements_t...>, element_t, index>
{
    static inline constexpr auto const value = index;
};

template <typename element_t, std::size_t index>
struct index_of_f<type_list_t<>, element_t, index>
{
    static_assert(meta::dependent_false_v<element_t>, "type_list_t element not found");
};

//! index of element type in type_list_t if present; static asserts if not
template <typename type_list_t, typename element_t>
inline constexpr auto const index_of_v = index_of_f<type_list_t, element_t, 0>::value;

// ---------------------------------------------------------------------------------------------------------------------

template <typename accumulated_type_list_t, typename remaining_type_list_t>
struct unique_f;

template <typename accumulated_type_list_t, typename next_element_t, typename... remaining_elements_t>
struct unique_f<accumulated_type_list_t, type_list_t<next_element_t, remaining_elements_t...>>
    : unique_f<
          std::conditional_t<
              contains_v<accumulated_type_list_t, next_element_t>, accumulated_type_list_t,
              append_t<accumulated_type_list_t, next_element_t>>,
          type_list_t<remaining_elements_t...>>
{};

template <typename accumulated_type_list_t>
struct unique_f<accumulated_type_list_t, type_list_t<>>
{
    using type = accumulated_type_list_t;
};

//! produces new type_list_t containing only first instance of each element; order is preserved
template <typename src_t>
using unique_t = unique_f<type_list_t<>, src_t>::type;

} // namespace type_list
} // namespace dink
