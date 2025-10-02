/*!
    \file
    Copyright (C) 2025 Frank Secilia
*/

#pragma once

#include <dink/lib.hpp>
#include <dink/deleter_traits.hpp>
#include <dink/unqualified.hpp>
#include <memory>

namespace dink {

namespace detail {

//! metafunction to determine type we actually store in the instance cache for a given type
template <typename type_t>
struct canonical_f;

//! base case: type is the unqualified type
template <typename type_t>
struct canonical_f
{
    using type = unqualified_t<type_t>;
};

//! lvalue refs are stored as values
template <typename type_t>
struct canonical_f<type_t&> : canonical_f<type_t>
{};

//! rvalue refs are stored as values
template <typename type_t>
struct canonical_f<type_t&&> : canonical_f<type_t>
{};

//! const is stored as value
template <typename type_t>
struct canonical_f<type_t const> : canonical_f<type_t>
{};

//! volatile is stored as value
template <typename type_t>
struct canonical_f<type_t volatile> : canonical_f<type_t>
{};

//! unique_ptr becomes unique_ptr to unqualified using deleter rebound to unqualified
template <typename type_t, typename deleter_t>
struct canonical_f<std::unique_ptr<type_t, deleter_t>>
{
    using type = std::unique_ptr<unqualified_t<type_t>, rebind_deleter_t<deleter_t, unqualified_t<type_t>>>;
};

//! shared_ptr becomes shared_ptr to unqualified
template <typename type_t>
struct canonical_f<std::shared_ptr<type_t>>
{
    using type = std::shared_ptr<unqualified_t<type_t>>;
};

//! weak_ptr becomes shared_ptr to unqualified
template <typename type_t>
struct canonical_f<std::weak_ptr<type_t>> : canonical_f<std::shared_ptr<type_t>>
{};

} // namespace detail

/*!
    determines concrete type to be stored in instance cache
    
    This trait uses unqualified_t to find the core type and then re-wraps it in the appropriate smart pointer if
    necessary:
        - unique_ptr<type_t> -> unique_ptr<unqualified_ttype_t>>
        - shared_ptr<type_t> -> shared_ptr<unqualified_t<type_t>>
        - weak_ptr<type_t>   -> shared_ptr<unqualified_t<type_t>>
        - type_t             -> unqualified_t<type_t>
*/
template <typename type_t>
using canonical_t = typename detail::canonical_f<type_t>::type;

} // namespace dink
