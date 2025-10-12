/*!
    \file
    Copyright (C) 2025 Frank Secilia
*/

#pragma once

#include <dink/lib.hpp>
#include <dink/not_found.hpp>
#include <type_traits>

namespace dink::delegation_policy {

/*!
    root container delegation policy
    
    The root container has no parent, so no delegation occurs.
*/
struct root_t
{
    // signals there is no parent to which to delegate
    template <typename request_t, typename dependency_chain_t>
    auto delegate() -> auto
    {
        return not_found;
    }
};

/*!
    nested container delegation policy
    
    Nested containers delegate to their parent container.
*/
template <typename parent_t>
struct nested_t
{
    parent_t* parent;

    // delegates resolution to parent
    template <typename request_t, typename dependency_chain_t>
    auto delegate() -> decltype(auto)
    {
        return parent->template resolve<request_t, dependency_chain_t>();
    }

    explicit nested_t(parent_t& parent) : parent{&parent} {}
};

template <typename value_t>
struct is_delegation_policy_f : std::false_type
{};

template <>
struct is_delegation_policy_f<delegation_policy::root_t> : std::true_type
{};

template <typename parent_t>
struct is_delegation_policy_f<delegation_policy::nested_t<parent_t>> : std::true_type
{};

template <typename value_t> concept is_delegation_policy = is_delegation_policy_f<std::remove_cvref_t<value_t>>::value;

} // namespace dink::delegation_policy
