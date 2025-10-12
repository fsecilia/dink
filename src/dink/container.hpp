/*!
    \file
    Copyright (C) 2025 Frank Secilia
*/

#pragma once

#include <dink/lib.hpp>
#include <dink/bindings.hpp>
#include <dink/cache/hash_table.hpp>
#include <dink/cache/type_indexed.hpp>
#include <dink/config.hpp>
#include <dink/delegation_policy.hpp>
#include <dink/not_found.hpp>
#include <dink/provider.hpp>
#include <dink/request_traits.hpp>
#include <dink/scope.hpp>
#include <dink/type_list.hpp>
#include <utility>

namespace dink {

// ---------------------------------------------------------------------------------------------------------------------
// Policy concepts
// ---------------------------------------------------------------------------------------------------------------------

template <typename policy_t>
concept is_container_policy = requires {
    typename policy_t::delegation_policy_t;
    typename policy_t::cache_t;
    typename policy_t::default_provider_factory_t;
};

// ---------------------------------------------------------------------------------------------------------------------
// Container detection
// ---------------------------------------------------------------------------------------------------------------------

template <is_container_policy policy_t, is_config config_t>
class container_t;

template <typename>
struct is_container_f : std::false_type
{};

template <is_container_policy policy_t, is_config config_t>
struct is_container_f<container_t<policy_t, config_t>> : std::true_type
{};

template <typename value_t> concept is_container = is_container_f<std::remove_cvref_t<value_t>>::value;

// ---------------------------------------------------------------------------------------------------------------------
// Container implementation
// ---------------------------------------------------------------------------------------------------------------------

template <is_container_policy policy_t, is_config config_t>
class container_t
{
public:
    using cache_t = policy_t::cache_t;
    using delegation_policy_t = policy_t::delegation_policy_t;
    using default_provider_factory_t = policy_t::default_provider_factory_t;

    // -----------------------------------------------------------------------------------------------------------------
    // Constructors
    // -----------------------------------------------------------------------------------------------------------------

    //! Constructs root container with given bindings
    template <is_binding... bindings_t>
    explicit container_t(bindings_t&&... bindings)
        : cache_{}, delegation_policy_{}, config_{resolve_bindings(std::forward<bindings_t>(bindings)...)},
          default_provider_factory_{}
    {}

    //! Constructs nested container with parent and given bindings
    template <is_container parent_t, is_binding... bindings_t>
    explicit container_t(parent_t& parent, bindings_t&&... bindings)
        : cache_{}, delegation_policy_{parent}, config_{resolve_bindings(std::forward<bindings_t>(bindings)...)},
          default_provider_factory_{}
    {}

    //! Direct construction from components (used by deduction guides)
    container_t(
        cache_t cache, delegation_policy_t delegation_policy, config_t config,
        default_provider_factory_t default_provider_factory
    ) noexcept
        : cache_{std::move(cache)}, delegation_policy_{std::move(delegation_policy)}, config_{std::move(config)},
          default_provider_factory_{std::move(default_provider_factory)}
    {}

    // -----------------------------------------------------------------------------------------------------------------
    // Resolution
    // -----------------------------------------------------------------------------------------------------------------

    /*!
        Resolves a dependency of the requested type

        Resolution follows this priority:
        1. Check cache (for singleton requests)
        2. Check local bindings
        3. Delegate to parent container (for nested containers)
        4. Use default provider (auto-wired constructor)

        \tparam request_t The type being requested (may include qualifiers like & or smart pointers)
        \tparam dependency_chain_t Type list tracking dependencies to detect circular dependencies
        \return Instance of the requested type
    */
    template <typename request_t, typename dependency_chain_t = type_list_t<>>
    auto resolve() -> as_returnable_t<request_t>
    {
        using request_traits_t = request_traits_f<request_t>;
        using resolved_t = typename request_traits_t::value_type;
        using bound_scope_t = typename config_t::template bound_scope_t<resolved_t>;
        using effective_scope_t = effective_scope_t<bound_scope_t, request_t>;

        // Step 1: Check cache for singleton requests
        if constexpr (std::same_as<effective_scope_t, scope::singleton_t>)
        {
            if (auto cached = request_traits_t::find_in_cache(cache_)) return as_requested<request_t>(cached);
        }

        // Step 2: Check local bindings
        if constexpr (auto local_binding = config_.template find_binding<resolved_t>();
                      !std::is_same_v<decltype(local_binding), not_found_t>)
        {
            return resolve_from_binding<request_t, dependency_chain_t, effective_scope_t>(*local_binding);
        }

        // Step 3: Try delegating to parent
        if constexpr (decltype(auto) delegate_result
                      = delegation_policy_.template delegate<request_t, dependency_chain_t>();
                      !std::is_same_v<decltype(delegate_result), not_found_t>)
        {
            return as_requested<request_t>(delegate_result);
        }

        // Step 4: Use default provider (auto-wire constructor)
        return resolve_with_default_provider<request_t, dependency_chain_t>();
    }

private:
    // -----------------------------------------------------------------------------------------------------------------
    // Resolution helpers
    // -----------------------------------------------------------------------------------------------------------------

    //! Resolves from an explicit binding
    template <typename request_t, typename dependency_chain_t, typename effective_scope_t, typename binding_t>
    auto resolve_from_binding(binding_t& binding) -> as_returnable_t<request_t>
    {
        if constexpr (provider::is_accessor<typename binding_t::provider_type>)
        {
            // Accessor providers (references/prototypes) bypass caching
            return as_requested<request_t>(binding.provider.get());
        }
        else
        {
            // Creator providers respect effective scope
            return invoke_provider<request_t, dependency_chain_t, effective_scope_t>(binding.provider);
        }
    }

    //! Resolves using default provider (auto-wired constructor)
    template <typename request_t, typename dependency_chain_t>
    auto resolve_with_default_provider() -> as_returnable_t<request_t>
    {
        auto default_provider = default_provider_factory_.template create<request_t>();
        using default_provider_t = decltype(default_provider);

        // Default providers have transient_t as their bound scope, so recalculate effective scope.
        using default_effective_scope_t = effective_scope_t<typename default_provider_t::default_scope_t, request_t>;
        return invoke_provider<request_t, dependency_chain_t, default_effective_scope_t>(default_provider);
    }

    // -----------------------------------------------------------------------------------------------------------------
    // Provider invocation
    // -----------------------------------------------------------------------------------------------------------------

    //! Invokes provider respecting the effective scope
    template <typename request_t, typename dependency_chain_t, typename scope_t, typename provider_t>
    auto invoke_provider(provider_t& provider) -> as_returnable_t<request_t>
    {
        if constexpr (std::same_as<scope_t, scope::singleton_t>)
        {
            return invoke_provider_singleton<request_t, dependency_chain_t>(provider);
        }
        else { return invoke_provider_transient<request_t, dependency_chain_t>(provider); }
    }

    //! Invokes provider and caches result as singleton
    template <typename request_t, typename dependency_chain_t, typename provider_t>
    auto invoke_provider_singleton(provider_t& provider) -> as_returnable_t<request_t>
    {
        return as_requested<request_t>(
            request_traits_f<request_t>::template resolve_from_cache<typename provider_t::provided_t>(cache_, [&]() {
                return provider.template create<dependency_chain_t>(*this);
            })
        );
    }

    //! invokes provider to create transient instance with no caching
    template <typename request_t, typename dependency_chain_t, typename provider_t>
    auto invoke_provider_transient(provider_t& provider) -> as_returnable_t<request_t>
    {
        return as_requested<request_t>(provider.template create<dependency_chain_t>(*this));
    }

    // -----------------------------------------------------------------------------------------------------------------
    // Member data
    // -----------------------------------------------------------------------------------------------------------------

    cache_t cache_;
    delegation_policy_t delegation_policy_;
    config_t config_;
    [[no_unique_address]] default_provider_factory_t default_provider_factory_;
};

// ---------------------------------------------------------------------------------------------------------------------
// Named policies
// ---------------------------------------------------------------------------------------------------------------------

//! Policy for root containers (no parent delegation)
struct root_container_policy_t
{
    using delegation_policy_t = delegation_policy::root_t;
    using cache_t = cache::type_indexed_t<>;
    using default_provider_factory_t = provider::default_factory_t;
};

//! Policy for nested containers (delegates to parent)
template <typename parent_t>
struct nested_container_policy_t
{
    using delegation_policy_t = delegation_policy::nested_t<parent_t>;
    using cache_t = cache::hash_table_t;
    using default_provider_factory_t = provider::default_factory_t;
};

// ---------------------------------------------------------------------------------------------------------------------
// Deduction guides
// ---------------------------------------------------------------------------------------------------------------------

//! Deduction guide for root containers
template <is_binding... bindings_t>
container_t(bindings_t&&...) -> container_t<
    root_container_policy_t,
    typename config_from_tuple_f<decltype(resolve_bindings(std::declval<bindings_t>()...))>::type>;

//! Deduction guide for nested containers
template <is_container parent_t, is_binding... bindings_t>
container_t(parent_t& parent, bindings_t&&...) -> container_t<
    nested_container_policy_t<parent_t>,
    typename config_from_tuple_f<decltype(resolve_bindings(std::declval<bindings_t>()...))>::type>;

// ---------------------------------------------------------------------------------------------------------------------
// Type aliases
// ---------------------------------------------------------------------------------------------------------------------

//! Root container with given bindings
template <is_binding... bindings_t>
using root_container_t = container_t<
    root_container_policy_t,
    typename config_from_tuple_f<decltype(resolve_bindings(std::declval<bindings_t>()...))>::type>;

//! Nested container with given parent and bindings
template <is_container parent_t, typename... bindings_t>
using nested_container_t = container_t<
    nested_container_policy_t<parent_t>,
    typename config_from_tuple_f<decltype(resolve_bindings(std::declval<bindings_t>()...))>::type>;

} // namespace dink
