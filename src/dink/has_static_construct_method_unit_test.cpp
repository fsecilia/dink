/*!
    \file
    Copyright (C) 2025 Frank Secilia
*/

#include "has_static_construct_method.hpp"

namespace dink {

// unique, but otherwise arbitrary arg type
template <int_t index>
struct formal_arg_t
{};

struct no_static_construct_method_t
{};

struct zero_args_t
{
    static auto construct() -> zero_args_t;
};

struct one_arg_value_t
{
    static auto construct(formal_arg_t<0>) -> one_arg_value_t;
};

struct one_arg_lref_t
{
    static auto construct(formal_arg_t<0>&) -> one_arg_lref_t;
};

struct one_arg_rref_t
{
    static auto construct(formal_arg_t<0>&&) -> one_arg_rref_t;
};
struct one_arg_cref_t
{
    static auto construct(formal_arg_t<0> const&) -> one_arg_cref_t;
};

struct multiple_arg_t
{
    static auto construct(formal_arg_t<0>, formal_arg_t<1>&, formal_arg_t<1>&&, formal_arg_t<2> const&)
        -> multiple_arg_t;
};

struct max_num_args_t
{
    static auto construct(
        formal_arg_t<0>, formal_arg_t<1>, formal_arg_t<2>, formal_arg_t<3>, formal_arg_t<4>, formal_arg_t<5>,
        formal_arg_t<6>, formal_arg_t<7>, formal_arg_t<8>, formal_arg_t<9>
    ) -> max_num_args_t;
};

struct too_many_args_t
{
    static auto construct(
        formal_arg_t<0>, formal_arg_t<1>, formal_arg_t<2>, formal_arg_t<3>, formal_arg_t<4>, formal_arg_t<5>,
        formal_arg_t<6>, formal_arg_t<7>, formal_arg_t<8>, formal_arg_t<9>, formal_arg_t<10>
    ) -> max_num_args_t;
};

struct template_one_arg_value_t
{
    template <typename arg_t>
    static auto construct(arg_t) -> template_one_arg_value_t;
};

struct template_one_arg_lref_t
{
    template <typename arg_t>
    static auto construct(arg_t&) -> template_one_arg_lref_t;
};

struct template_one_arg_rref_t
{
    template <typename arg_t>
    static auto construct(arg_t&&) -> template_one_arg_rref_t;
};

struct template_one_arg_cref_t
{
    template <typename arg_t>
    static auto construct(arg_t const&) -> template_one_arg_cref_t;
};

struct template_multiple_template_arg_t
{
    template <typename arg0_t, typename arg1_t, typename arg2_t, typename arg3_t>
    static auto construct(arg0_t, arg1_t&, arg2_t&&, arg3_t const&) -> template_multiple_template_arg_t;
};

struct template_multiple_mixed_arg_t
{
    template <typename arg0_t, typename arg1_t, typename arg2_t, typename arg3_t>
    static auto construct(
        arg0_t, arg1_t&, arg2_t&&, arg3_t const&, formal_arg_t<4>, formal_arg_t<5>&, formal_arg_t<6>&&,
        formal_arg_t<7> const&
    ) -> template_multiple_mixed_arg_t;
};

struct mismatched_return_type_t
{
    static auto construct() -> int_t { return 0; }
};

struct nonstatic_t
{
    auto construct() -> nonstatic_t;
};

struct only_in_base_t : zero_args_t
{};

struct in_base_and_derived_same_args_t : zero_args_t
{
    static auto construct() -> in_base_and_derived_same_args_t;
};

struct in_base_and_derived_more_args_t : zero_args_t
{
    static auto construct(formal_arg_t<0>) -> in_base_and_derived_more_args_t;
};

// clang-format off
static_assert(!has_static_construct_method<no_static_construct_method_t>);
static_assert( has_static_construct_method<zero_args_t>);
static_assert( has_static_construct_method<one_arg_value_t>);
static_assert( has_static_construct_method<one_arg_lref_t>);
static_assert( has_static_construct_method<one_arg_rref_t>);
static_assert( has_static_construct_method<one_arg_cref_t>);
static_assert( has_static_construct_method<multiple_arg_t>);
static_assert( has_static_construct_method<max_num_args_t>);
static_assert(!has_static_construct_method<too_many_args_t>);
static_assert( has_static_construct_method<template_one_arg_value_t>);
static_assert( has_static_construct_method<template_one_arg_lref_t>);
static_assert( has_static_construct_method<template_one_arg_rref_t>);
static_assert( has_static_construct_method<template_one_arg_cref_t>);
static_assert( has_static_construct_method<template_multiple_template_arg_t>);
static_assert( has_static_construct_method<template_multiple_mixed_arg_t>);
static_assert(!has_static_construct_method<mismatched_return_type_t>);
static_assert(!has_static_construct_method<nonstatic_t>);
static_assert(!has_static_construct_method<only_in_base_t>);
static_assert( has_static_construct_method<in_base_and_derived_same_args_t>);
static_assert( has_static_construct_method<in_base_and_derived_more_args_t>);
// clang-format on

} // namespace dink
