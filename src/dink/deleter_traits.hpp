/*!
    \file
    Copyright (C) 2025 Frank Secilia
*/

#pragma once

#include <dink/lib.hpp>

namespace dink {

/*!
    customization point to rebind deleters to a new element type
*/
template <typename deleter_t, typename new_element_t>
struct rebind_deleter_f;

/*!
    primary template for rebinding a deleter to a new element type

    The unspecialized template makes no attempt at rebinding a deleter. It just tries to use the new one as-is.

    \tparam deleter_t original deleter
    \tparam new_element_t new element type to be deleteed
*/
template <typename deleter_t, typename new_element_t>
struct rebind_deleter_f
{
    using type = deleter_t;
};

/*
    specialization for rebindable, parameterized deleters

    Deleters are an arbitrary type and can have an arbitrary parameterization. In the general case, we simply cannot
    directly convert from a deleter that works on one type another type. However, we can follow a convention similar to
    std::default_delete. As long as the first parameter of the deleter matches the original type, we can rebind the
    deleter on the new type.
*/
template <
    typename old_element_t, template <typename, typename...> class deleter_t, typename... deleter_params_t,
    typename new_element_t>
struct rebind_deleter_f<deleter_t<old_element_t, deleter_params_t...>, new_element_t>
{
    using type = deleter_t<new_element_t, deleter_params_t...>;
};

//! rebinds deleters to a new element type
template <typename deleter_t, typename new_element_t>
using rebind_deleter_t = typename rebind_deleter_f<deleter_t, new_element_t>::type;

} // namespace dink
