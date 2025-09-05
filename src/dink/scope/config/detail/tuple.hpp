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

template <typename accumulated_elements_tuple_t, typename accumulated_keys_tuple_t, typename remaining_elements_tuple_t>
struct unique_by_key_f;

template <typename accumulated_elements_tuple_t, typename accumulated_keys_tuple_t>
struct unique_by_key_f<accumulated_elements_tuple_t, accumulated_keys_tuple_t, std::tuple<>>
{
    using type = accumulated_elements_tuple_t;
};

template <
    typename accumulated_elements_tuple_t, typename accumulated_keys_tuple_t, typename next_element_t,
    typename... remaining_elements_t>
struct unique_by_key_f<
    accumulated_elements_tuple_t, accumulated_keys_tuple_t, std::tuple<next_element_t, remaining_elements_t...>>
{
private:
    using key_type_t = next_element_t::key_t;
    static constexpr bool is_duplicate = detail::tuple::contains_v<accumulated_keys_tuple_t, key_type_t>;

    using next_accumulated_elements_t = std::conditional_t<
        is_duplicate, accumulated_elements_tuple_t,
        detail::tuple::append_t<accumulated_elements_tuple_t, next_element_t>>;

    using next_accumulated_keys_t = std::conditional_t<
        is_duplicate, accumulated_keys_tuple_t, detail::tuple::append_t<accumulated_keys_tuple_t, key_type_t>>;

public:
    using type = unique_by_key_f<
        next_accumulated_elements_t, next_accumulated_keys_t, std::tuple<remaining_elements_t...>>::type;
};

//! produces new tuple containing only first instance of each element containing same key_t; unique by projection of key
template <typename tuple_t>
using unique_by_key_t = unique_by_key_f<std::tuple<>, std::tuple<>, tuple_t>::type;

} // namespace dink::detail::tuple
