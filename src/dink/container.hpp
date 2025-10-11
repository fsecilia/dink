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
#include <dink/scope.hpp>
#include <dink/type_list.hpp>
#include <utility>

namespace dink {

template <typename scope_t, typename config_t>
class container_t;

// --- Concept to identify a container (avoids ambiguity)
namespace detail {

template <typename>
struct is_container_f : std::false_type
{};

template <typename scope_t, typename config_t>
struct is_container_f<container_t<scope_t, config_t>> : std::true_type
{};

// --- Trait and concept for identifying a scope ---
template <typename>
struct is_scope_f : std::false_type
{};

template <>
struct is_scope_f<container::scope::global_t> : std::true_type
{};

template <typename T>
struct is_scope_f<container::scope::nested_t<T>> : std::true_type
{};

template <typename T> concept is_scope = is_scope_f<std::decay_t<T>>::value;

// --- Trait and concept for identifying a config ---
template <typename>
struct is_config_f : std::false_type
{};

template <typename... Bindings>
struct is_config_f<config_t<Bindings...>> : std::true_type
{};

template <typename T> concept is_config = is_config_f<std::decay_t<T>>::value;

//
template <typename>
struct is_reference_wrapper : std::false_type
{};

template <typename T>
struct is_reference_wrapper<std::reference_wrapper<T>> : std::true_type
{};

template <typename T>
inline constexpr bool is_reference_wrapper_v = is_reference_wrapper<T>::value;

} // namespace detail

template <typename T> concept is_container = detail::is_container_f<std::decay_t<T>>::value;

template <typename scope_t, typename config_t>
class container_t
{
    // Helper to get the deduced config type from a set of configs
    template <typename... bindings_t>
    using deduced_config_t =
        typename config_from_tuple_f<decltype(resolve_bindings(std::declval<bindings_t>()...))>::type;

public:
    // Direct constructor for testing, now properly constrained
    template <detail::is_scope ScopeP, detail::is_config ConfigP>
    explicit container_t(ScopeP&& scope, ConfigP&& config)
        : scope_{std::forward<ScopeP>(scope)}, config_{std::forward<ConfigP>(config)}
    {}

    // global constructor (empty)
    explicit container_t() : config_{} {}

    // global constructor (non-empty)
    template <typename first_binding_p, typename... rest_bindings_p>
    requires(
        !is_container<std::decay_t<first_binding_p>>
        && is_binding<std::decay_t<first_binding_p>>
        && (is_binding<std::decay_t<rest_bindings_p>> && ...)
    )
    explicit container_t(first_binding_p&& first, rest_bindings_p&&... rest)
        : config_{resolve_bindings(std::forward<first_binding_p>(first), std::forward<rest_bindings_p>(rest)...)}
    {}

    // nested constructor
    template <typename p_scope_t, typename p_config_t, typename... bindings_t>
    requires(is_binding<bindings_t> && ...)
    explicit container_t(container_t<p_scope_t, p_config_t>& parent, bindings_t&&... configs)
        : scope_{parent}, config_{resolve_bindings(std::forward<bindings_t>(configs)...)}
    {}

    template <typename request_t, typename dependency_chain_t = type_list_t<>>
    auto resolve() -> decltype(auto)
    {
        // Get the traits for the request
        using traits = request_traits_f<request_t>;
        using resolved_t = typename traits::value_type;

        // --- FIX: Skip cache lookup for transient requests ---
        // If the request itself forces a transient lifestyle (like unique_ptr or T&&),
        // we must create a new instance, so we bypass the cache check.
        if constexpr (traits::transitive_lifestyle != transitive_lifestyle_t::transient)
        {
            // 1. Check local cache (Runtime check)
            if constexpr (detail::is_shared_ptr_v<request_t> || detail::is_weak_ptr_v<request_t>)
            {
                if (auto cached = scope_.template find_shared_in_cache<resolved_t>(); cached)
                {
                    return as_requested<request_t>(std::move(cached));
                }
            }
            else
            {
                if (auto cached = scope_.template find_in_local_cache<resolved_t>(); cached)
                {
                    return as_requested<request_t>(std::move(cached));
                }
            }
        }

        // --- If we get here, the cache was skipped or missed ---

        // 2. Check local bindings (Compile-time check)
        auto binding = config_.template find_binding<resolved_t>();
        if constexpr (!std::is_same_v<decltype(binding), not_found_t>)
        {
            return create_from_binding<request_t, dependency_chain_t>(*binding);
        }

        // 3. Delegate to parent, or prepare to use the default provider.
        decltype(auto) delegate_result = scope_.template delegate<request_t, dependency_chain_t>();
        if constexpr (!std::is_same_v<decltype(delegate_result), not_found_t>)
        {
            return as_requested<request_t>(delegate_result);
        }
        else { return create_from_default_provider<request_t, dependency_chain_t>(); }
    }

private:
    // create instance from a binding
    template <typename request_t, typename dependency_chain_t, typename binding_t>
    auto create_from_binding(binding_t& binding) -> returned_t<request_t>
    {
        if constexpr (providers::is_accessor<typename binding_t::provider_type>)
        {
            // accessor - just get the instance
            return as_requested<request_t>(binding.provider.get());
        }
        else
        {
            // creator - check effective lifestyle
            using binding_lifestyle_t = typename binding_t::lifestyle_type;
            using effective_lifestyle_t = effective_lifestyle_t<binding_lifestyle_t, request_t>;
            return execute_provider<request_t, dependency_chain_t, effective_lifestyle_t>(binding.provider);
        }
    }

    // create instance from default provider
    template <typename request_t, typename dependency_chain_t>
    auto create_from_default_provider() -> returned_t<request_t>
    {
        using resolved_t = resolved_t<request_t>;

        providers::creator_t<resolved_t> default_provider;
        using effective_lifestyle_t = effective_lifestyle_t<lifestyle::transient_t, request_t>;
        return execute_provider<request_t, dependency_chain_t, effective_lifestyle_t>(default_provider);
    }

    template <typename request_t, typename dependency_chain_t, typename lifestyle_t, typename provider_t>
    auto execute_provider(provider_t& provider) -> returned_t<request_t>
    {
        using provided_t = typename provider_t::provided_t;
        using resolved_t = resolved_t<request_t>; // The underlying T

        if constexpr (std::same_as<lifestyle_t, lifestyle::singleton_t>)
        {
            // --- SINGLETON LIFESTYLE LOGIC ---

            // This is the core fix: Handle unique_ptr as a special case for singletons.
            // The user wants a unique copy of the canonical singleton instance.
            if constexpr (detail::is_unique_ptr_v<request_t>) // Assume is_unique_ptr_v exists
            {
                // 1. Resolve the singleton instance. This will create and cache it on the first call,
                //    and return a reference to the cached instance on subsequent calls.
                auto& singleton_instance
                    = scope_.template resolve_singleton<provided_t, dependency_chain_t>(provider, *this);

                // 2. Create a copy of the singleton and return it in a unique_ptr.
                return std::make_unique<resolved_t>(singleton_instance);
            }
            else if constexpr (detail::is_shared_ptr_v<request_t> || detail::is_weak_ptr_v<request_t>)
            {
                // Use the scope's dedicated shared_ptr handling, which correctly
                // creates a non-owning or cached shared_ptr to the singleton.
                return as_requested<request_t>(
                    scope_.template resolve_shared_ptr<provided_t, dependency_chain_t>(provider, *this)
                );
            }
            else
            {
                // For other requests (T&, T*), resolve the singleton and return a reference to it.
                return as_requested<request_t>(
                    scope_.template resolve_singleton<provided_t, dependency_chain_t>(provider, *this)
                );
            }
        }
        else // --- TRANSIENT LIFESTYLE LOGIC ---
        {
            // Create a new instance every time, without caching.
            return as_requested<request_t>(provider.template create<dependency_chain_t>(*this));
        }
    }

    scope_t scope_{};
    config_t config_{};
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
        decltype(resolve_bindings(std::declval<first_binding_p>(), std::declval<rest_bindings_p>()...))>::type>;

//! deduction guide for empty global containers
container_t() -> container_t<container::scope::global_t, config_t<>>;

//! deduction guide for nested containers
template <typename p_scope_t, typename p_config_t, typename... bindings_t>
requires(is_binding<bindings_t> && ...)
container_t(container_t<p_scope_t, p_config_t>& parent, bindings_t&&...) -> container_t<
    container::scope::nested_t<std::decay_t<decltype(parent)>>,
    typename config_from_tuple_f<decltype(resolve_bindings(std::declval<bindings_t>()...))>::type>;

// type aliases

template <typename... bindings_t>
using global_container_t = container_t<
    container::scope::global_t,
    typename config_from_tuple_f<decltype(resolve_bindings(std::declval<bindings_t>()...))>::type>;

template <typename parent_t, typename... bindings_t>
using child_container_t = container_t<
    container::scope::nested_t<parent_t>,
    typename config_from_tuple_f<decltype(resolve_bindings(std::declval<bindings_t>()...))>::type>;

} // namespace dink
