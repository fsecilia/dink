/*!
    \file
    Copyright (C) 2025 Frank Secilia
*/

#pragma once

#include <dink/lib.hpp>
#include <functional>
#include <memory>

namespace dink {

namespace detail {

//! metafunction to remove all ref, cv, and pointer qualifiers, including smart pointers and reference wrappers
template <typename type_t>
struct canonical_f;

//! base case: type is unmodified
template <typename type_t>
struct canonical_f
{
    using type = type_t;
};

//! removes lvalue reference
template <typename type_t>
struct canonical_f<type_t&> : canonical_f<type_t>
{};

//! removes rvalue reference
template <typename type_t>
struct canonical_f<type_t&&> : canonical_f<type_t>
{};

//! removes const
template <typename type_t>
struct canonical_f<type_t const> : canonical_f<type_t>
{};

//! removes volatile
template <typename type_t>
struct canonical_f<type_t volatile> : canonical_f<type_t>
{};

//! removes pointer
template <typename type_t>
struct canonical_f<type_t*> : canonical_f<type_t>
{};

//! pointers to function are unmodified; pointer is not removed
template <typename return_t, typename... args_t>
struct canonical_f<return_t (*)(args_t...)>
{
    using type = return_t (*)(args_t...);
};

//! decays function to function pointer
template <typename return_t, typename... args_t>
struct canonical_f<return_t(args_t...)> : canonical_f<return_t (*)(args_t...)>
{};

//! removes unsized array
template <typename type_t>
struct canonical_f<type_t[]> : canonical_f<type_t>
{};

//! removes const unsized array; this specialization is necessary to break a tie between const and unsized array
template <typename type_t>
struct canonical_f<type_t const[]> : canonical_f<type_t>
{};

//! removes sized array
template <typename type_t, std::size_t size>
struct canonical_f<type_t[size]> : canonical_f<type_t>
{};

//! removes const sized array; this specialization is necessary to break a tie between const and sized array
template <typename type_t, std::size_t size>
struct canonical_f<type_t const[size]> : canonical_f<type_t>
{};

//! removes reference_wrapper
template <typename type_t>
struct canonical_f<std::reference_wrapper<type_t>> : canonical_f<type_t>
{};

//! removes unique_ptr
template <typename type_t, typename deleter_t>
struct canonical_f<std::unique_ptr<type_t, deleter_t>> : canonical_f<type_t>
{};

//! removes shared_ptr
template <typename type_t>
struct canonical_f<std::shared_ptr<type_t>> : canonical_f<type_t>
{};

//! removes weak_ptr
template <typename type_t>
struct canonical_f<std::weak_ptr<type_t>> : canonical_f<type_t>
{};

} // namespace detail

/*!
    gets core, canonical type

    This trait removes references, cv-qualifiers, pointers, smart pointers, and reference wrappers to distill a type
    down to its fundamental form.
*/
template <typename type_t>
using canonical_t = typename detail::canonical_f<type_t>::type;

} // namespace dink
