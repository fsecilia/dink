/*!
    \file
    Copyright (C) 2025 Frank Secilia
*/

#pragma once

#include <dink/lib.hpp>
#include <dink/not_found.hpp>
#include <type_traits>

namespace dink {
namespace delegation_strategy {

/*!
    delegation strategy for the root, root container
    
    This strategy has no parent. 
*/
struct root_t
{
    // signals there is no parent to which to delegate
    template <typename request_t, typename dependency_chain_t>
    auto delegate_to_parent() -> auto
    {
        return not_found;
    }
};

/*!
    scope for nested containers
    
    These scopes cache their instances in a hash table, mapping from type_index to shared_ptr<void>. 
*/
template <typename parent_t>
struct nested_t
{
    parent_t* parent;

    template <typename request_t, typename dependency_chain_t>
    auto delegate_to_parent() -> decltype(auto)
    {
        // delegate remaining resolution the parent
        return parent->template resolve<request_t, dependency_chain_t>();
    }

    explicit nested_t(parent_t& parent) : parent{&parent} {}
};

} // namespace delegation_strategy

template <typename>
struct is_delegation_strategy_f : std::false_type
{};

template <>
struct is_delegation_strategy_f<delegation_strategy::root_t> : std::true_type
{};

template <typename parent_t>
struct is_delegation_strategy_f<delegation_strategy::nested_t<parent_t>> : std::true_type
{};

template <typename value_t>
concept is_delegation_strategy = is_delegation_strategy_f<std::remove_cvref_t<value_t>>::value;

} // namespace dink
