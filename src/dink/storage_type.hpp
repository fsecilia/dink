/*!
    \file
    Copyright (C) 2025 Frank Secilia
*/

#pragma once

#include <dink/lib.hpp>
#include <dink/unqualified.hpp>
#include <memory>

namespace dink {

namespace detail {

//! metafunction to determine type we actually store in the instance cache for a given type
template <typename type_t>
struct storage_type_f;

//! base case: type is the unqualified type
template <typename type_t>
struct storage_type_f
{
    using type = unqualified_t<type_t>;
};

//! lvalue refs are stored as values
template <typename type_t>
struct storage_type_f<type_t&> : storage_type_f<type_t>
{};

//! rvalue refs are stored as values
template <typename type_t>
struct storage_type_f<type_t&&> : storage_type_f<type_t>
{};

//! const is stored as value
template <typename type_t>
struct storage_type_f<type_t const> : storage_type_f<type_t>
{};

//! volatile is stored as value
template <typename type_t>
struct storage_type_f<type_t volatile> : storage_type_f<type_t>
{};

/*!
    unique_ptr becomes unique_ptr to unqualified using original deleter
    
    This specialization requires the original deleter can delete both type_t and unqualified_t<type_t>.
*/
template <typename type_t, typename deleter_t>
struct storage_type_f<std::unique_ptr<type_t, deleter_t>>
{
    using type = std::unique_ptr<unqualified_t<type_t>, deleter_t>;
};

/*!
    unique_ptr becomes unique_ptr to unqualified using deleter rebound to unqualified 
    
    Deleters are an arbitrary type and can have an arbitrary parameterization. In the general case, we simply cannot
    directly convert from a deleter that works on the original type to one that works on the unqualified type. However,
    we can follow a convention similar to std::default_delete. As long as the first parameter of the deleter matches
    the original type, we can rebind the deleter on the unqualified type.
*/
template <typename type_t, template <typename, typename...> class deleter_t, typename... deleter_params_t>
struct storage_type_f<std::unique_ptr<type_t, deleter_t<type_t, deleter_params_t...>>>
{
    using type = std::unique_ptr<unqualified_t<type_t>, deleter_t<unqualified_t<type_t>, deleter_params_t...>>;
};

//! shared_ptr becomes shared_ptr to unqualified
template <typename type_t>
struct storage_type_f<std::shared_ptr<type_t>>
{
    using type = std::shared_ptr<unqualified_t<type_t>>;
};

//! weak_ptr becomes shared_ptr to unqualified
template <typename type_t>
struct storage_type_f<std::weak_ptr<type_t>> : storage_type_f<std::shared_ptr<type_t>>
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
using storage_type_t = typename detail::storage_type_f<type_t>::type;

} // namespace dink
