/*!
    \file
    Copyright (C) 2025 Frank Secilia
*/

#pragma once

#include <dink/lib.hpp>
#include <concepts>
#include <type_traits>

namespace dink {
namespace detail {

//! matches any arg, without the baggage of dink::arg_t
struct arg_t
{
    template <typename deduced_t>
    constexpr operator deduced_t();

    template <typename deduced_t>
    constexpr operator deduced_t&() const;
};

//! matches a specific signature
template <typename resolved_t, typename... args_t>
concept has_static_construct_method_for_args = requires(args_t... args) {
    { std::remove_cvref_t<resolved_t>::construct(args...) } -> std::convertible_to<resolved_t>;
};

//! matches signatures using type recursion
template <typename resolved_t, typename... args_t>
struct has_static_construct_method_t;

//! match is successful when has_static_construct_method_for_args matches
template <typename resolved_t, typename... args_t>
requires(has_static_construct_method_for_args<resolved_t, args_t...>)
struct has_static_construct_method_t<resolved_t, args_t...>
{
    static constexpr auto result = true;
};

//! unsuccessful matches append one arg
template <typename resolved_t, typename... args_t>
requires(!has_static_construct_method_for_args<resolved_t, args_t...> && sizeof...(args_t) <= dink_max_deduced_params)
struct has_static_construct_method_t<resolved_t, args_t...>
    : has_static_construct_method_t<resolved_t, args_t..., arg_t>
{};

//! exceeding dink_max_deduced_params fails
template <typename resolved_t, typename... args_t>
requires(sizeof...(args_t) > dink_max_deduced_params)
struct has_static_construct_method_t<resolved_t, args_t...>
{
    static constexpr auto result = false;
};

} // namespace detail

//! true if type has a static construct() method, regardless of signature
template <typename type_t> concept has_static_construct_method = detail::has_static_construct_method_t<type_t>::result;

//! inverse of has_static_construct_method
template <typename type_t> concept missing_static_construct_method = !has_static_construct_method<type_t>;

} // namespace dink
