/*!
    \file
    Copyright (C) 2025 Frank Secilia
*/

#pragma once

#include <dink/lib.hpp>
#include <dink/not_found.hpp>
#include <type_traits>

namespace dink::delegate {

/*!
    root container delegation policy
    
    The root container has no parent, so no delegation occurs.
*/
struct none_t
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
template <typename parent_container_t>
struct to_parent_t
{
    parent_container_t* parent_container;

    // delegates resolution to parent
    template <typename request_t, typename dependency_chain_t>
    auto delegate() -> decltype(auto)
    {
        return parent_container->template resolve<request_t, dependency_chain_t>();
    }

    explicit to_parent_t(parent_container_t& parent) : parent_container{&parent} {}
};

template <typename value_t>
struct is_delegate_f : std::false_type
{};

template <>
struct is_delegate_f<delegate::none_t> : std::true_type
{};

template <typename parent_t>
struct is_delegate_f<delegate::to_parent_t<parent_t>> : std::true_type
{};

template <typename value_t> concept is_delegate = is_delegate_f<std::remove_cvref_t<value_t>>::value;

} // namespace dink::delegate
