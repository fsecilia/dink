/*!
    \file
    Copyright (C) 2025 Frank Secilia
*/

#pragma once

#include <dink/lib.hpp>
#include <dink/bindings.hpp>
#include <dink/config.hpp>
#include <dink/lifestyle.hpp>
#include <dink/not_found.hpp>
#include <dink/providers.hpp>
#include <dink/request_traits.hpp>
#include <dink/resolver.hpp>
#include <dink/scope.hpp>
#include <dink/type_list.hpp>
#include <utility>

namespace dink {

template <typename scope_t, typename config_t, typename resolver_t>
class container_t;

// --- Concept to identify a container (avoids ambiguity)
namespace detail {

template <typename>
struct is_container_f : std::false_type
{};

template <typename scope_t, typename config_t, typename resolver_t>
struct is_container_f<container_t<scope_t, config_t, resolver_t>> : std::true_type
{};

} // namespace detail

template <typename T> concept is_container = detail::is_container_f<std::decay_t<T>>::value;

template <typename scope_t, typename config_t, typename resolver_t>
class container_t : public scope_t
{
    // Helper to get the deduced config type from a set of configs
    template <typename... bindings_t>
    using deduced_config_t =
        typename config_from_tuple_f<decltype(resolve_bindings(std::declval<bindings_t>()...))>::type;

public:
    container_t(scope_t scope, config_t config, resolver_t resolver) noexcept
        : scope_t{std::move(scope)}, config_{std::move(config)}, resolver_{std::move(resolver)}
    {}

    // global constructor
    template <typename... bindings_t>
    requires((!is_container<bindings_t> && ...) && (is_binding<bindings_t> && ...))
    explicit container_t(bindings_t&&... configs) : config_{resolve_bindings(std::forward<bindings_t>(configs)...)}
    {}

    // nested constructor
    template <typename p_scope_t, typename p_config_t, typename p_creator_t, typename... bindings_t>
    requires(is_binding<bindings_t> && ...)
    explicit container_t(container_t<p_scope_t, p_config_t, p_creator_t>& parent, bindings_t&&... configs)
        : scope_t{parent}, config_{resolve_bindings(std::forward<bindings_t>(configs)...)}
    {}

    template <typename request_t, typename dependency_chain_t = type_list_t<>>
    auto resolve() -> returned_t<request_t>
    {
        // run search for instance or bindings up the chain, or use the default provider with this container.
        return try_resolve<request_t, dependency_chain_t>([this]() -> decltype(auto) {
            return resolver_.template create_from_default_provider<request_t, dependency_chain_t>(*this);
        });
    }

    template <typename request_t, typename dependency_chain_t, typename factory_t>
    auto try_resolve(factory_t&& factory) -> returned_t<request_t>
    {
        using resolved_t = resolved_t<request_t>;

        // check local cache for for singleton-lifestyled requests
        if constexpr (request_traits_f<resolved_t>::transitive_lifestyle == transitive_lifestyle_t::singleton)
        {
            if (auto cached = scope_t::template find_in_local_cache<resolved_t>())
            {
                return as_requested<request_t>(std::move(cached));
            }
        }

        // check local bindings and create if found
        auto binding = config_.template find_binding<resolved_t>();
        if constexpr (!std::same_as<decltype(binding), not_found_t>)
        {
            return resolver_.template create_from_binding<request_t, dependency_chain_t>(*binding, *this);
        }

        return delegate_to_parent<request_t, dependency_chain_t>(std::forward<factory_t>(factory));
    }

    template <typename request_t, typename dependency_chain_t, typename factory_t>
    auto delegate_to_parent(factory_t&& factory) -> returned_t<request_t>
    {
        // delegate to parent if exists
        static constexpr auto is_global = std::same_as<scope_t, container::scope::global_t>;
        if constexpr (!is_global)
        {
            return scope_t::parent->template try_resolve<request_t, dependency_chain_t>(
                std::forward<factory_t>(factory)
            );
        }

        // Not found anywhere - invoke the factory from the original caller
        return factory();
    }

private:
    config_t config_{};
    resolver_t resolver_{};
};

/*
    deduction guides
    
    clang kept matching the global deduction guide for a nested container, so there are 2 clang-20.1-specific workarounds:
        - the global deduction guides must be split into empty and nonempty so we can apply a constrainted to the first_
          parameter
        - the nonempty version must use enable_if_t to remove itself from consideration
    
    When clang fixes this, the empty/nonempty split can be removed, as can the enable_if_t.
*/

//! deduction guide for nonempty global containers
template <
    typename first_binding_p, typename... rest_bindings_p,
    std::enable_if_t<
        !is_container<std::decay_t<first_binding_p>>
            && is_binding<std::decay_t<first_binding_p>>
            && (is_binding<std::decay_t<rest_bindings_p>> && ...),
        int>
    = 0>
container_t(first_binding_p&&, rest_bindings_p&&...) -> container_t<
    container::scope::global_t,
    typename config_from_tuple_f<
        decltype(resolve_bindings(std::declval<first_binding_p>(), std::declval<rest_bindings_p>()...))>::type,
    resolver_t>;

//! deduction guide for empty global containers
container_t() -> container_t<container::scope::global_t, config_t<>, resolver_t>;

//! deduction guide for nested containers
template <typename p_scope_t, typename p_config_t, typename p_creator_t, typename... bindings_t>
requires(is_binding<bindings_t> && ...)
container_t(container_t<p_scope_t, p_config_t, p_creator_t>& parent, bindings_t&&...) -> container_t<
    container::scope::nested_t<std::decay_t<decltype(parent)>>,
    typename config_from_tuple_f<decltype(resolve_bindings(std::declval<bindings_t>()...))>::type, resolver_t>;

// type aliases

template <typename... bindings_t>
using global_container_t = container_t<
    container::scope::global_t,
    typename config_from_tuple_f<decltype(resolve_bindings(std::declval<bindings_t>()...))>::type, resolver_t>;

template <typename parent_t, typename... bindings_t>
using child_container_t = container_t<
    container::scope::nested_t<parent_t>,
    typename config_from_tuple_f<decltype(resolve_bindings(std::declval<bindings_t>()...))>::type, dink::resolver_t>;

} // namespace dink
