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
#include <dink/delegate.hpp>
#include <dink/not_found.hpp>
#include <dink/provider.hpp>
#include <dink/request_traits.hpp>
#include <dink/scope.hpp>
#include <dink/type_list.hpp>
#include <utility>

namespace dink {

// ---------------------------------------------------------------------------------------------------------------------
// concepts
// ---------------------------------------------------------------------------------------------------------------------

template <typename policy_t>
concept is_container_policy = requires {
    typename policy_t::cache_t;
    typename policy_t::delegate_t;
    typename policy_t::default_provider_factory_t;
    typename policy_t::request_traits_t;
};

template <typename container_t>
concept is_container = requires(container_t& container) {
    { container.template resolve<meta::concept_probe_t, type_list_t<>>() } -> std::same_as<meta::concept_probe_t>;
};

// ---------------------------------------------------------------------------------------------------------------------
// container class
// ---------------------------------------------------------------------------------------------------------------------

template <is_container_policy policy_t, is_config config_t>
class container_t
{
public:
    using cache_t = policy_t::cache_t;
    using delegate_t = policy_t::delegate_t;
    using default_provider_factory_t = policy_t::default_provider_factory_t;
    using request_traits_t = policy_t::request_traits_t;

    // -----------------------------------------------------------------------------------------------------------------
    // constructors
    // -----------------------------------------------------------------------------------------------------------------

    //! constructs root container with given bindings
    template <is_binding... bindings_t>
    explicit container_t(bindings_t&&... bindings) noexcept
        : config_{resolve_bindings(std::forward<bindings_t>(bindings)...)}
    {}

    //! constructs nested container with given parent and bindings
    template <is_container parent_t, is_binding... bindings_t>
    explicit container_t(parent_t& parent, bindings_t&&... bindings) noexcept
        : config_{resolve_bindings(std::forward<bindings_t>(bindings)...)}, delegate_{parent}
    {}

    //! direct construction from components (used by deduction guides and testing)
    container_t(
        cache_t cache, config_t config, delegate_t delegate, default_provider_factory_t default_provider_factory,
        request_traits_t request_traits
    ) noexcept
        : cache_{std::move(cache)}, config_{std::move(config)}, delegate_{std::move(delegate)},
          default_provider_factory_{std::move(default_provider_factory)}, request_traits_{std::move(request_traits)}
    {}

    // -----------------------------------------------------------------------------------------------------------------
    // resolution
    // -----------------------------------------------------------------------------------------------------------------

    template <typename request_t, typename dependency_chain_t = type_list_t<>>
    auto resolve() -> as_returnable_t<request_t>
    {
        using resolved_t = resolved_t<request_t>;

        // check for local binding
        auto binding = config_.template find_binding<resolved_t>();
        static constexpr bool has_binding = !std::is_same_v<decltype(binding), not_found_t>;
        if constexpr (has_binding) return resolve_with_binding<request_t, dependency_chain_t>(binding);
        else return resolve_without_binding<request_t, dependency_chain_t>();
    }

private:
    // -----------------------------------------------------------------------------------------------------------------
    // resolution implementation
    // -----------------------------------------------------------------------------------------------------------------

    template <typename request_t, typename dependency_chain_t>
    auto resolve_with_binding(auto binding) -> as_returnable_t<request_t>
    {
        // check for accessor
        if constexpr (provider::is_accessor<typename std::remove_pointer_t<decltype(binding)>::provider_type>)
        {
            // accessor: bypass everything
            return request_traits_.template as_requested<request_t>(binding->provider.get());
        }
        else
        {
            // creator: use scope and cache

            // determine effective scope
            using bound_scope_t = typename std::remove_pointer_t<decltype(binding)>::scope_type;
            using effective_scope_t = effective_scope_t<bound_scope_t, request_t>;

            // singleton or transient?
            if constexpr (std::same_as<effective_scope_t, scope::singleton_t>)
            {
                // singleton

                // check cache
                if (auto cached = request_traits_.template find_in_cache<request_t>(cache_))
                {
                    return request_traits_.template as_requested<request_t>(cached);
                }

                // not in cache, create and cache atomically
                static_assert(!provider::is_accessor<typename std::remove_pointer_t<decltype(binding)>::provider_type>);
                return invoke_provider_singleton<request_t, dependency_chain_t>(binding->provider);
            }
            else
            {
                // transient

                // create without caching
                return invoke_provider_transient<request_t, dependency_chain_t>(binding->provider);
            }
        }
    }

    template <typename request_t, typename dependency_chain_t>
    auto resolve_without_binding() -> as_returnable_t<request_t>
    {
        // determine effective scope
        using effective_scope_t = effective_scope_t<scope::default_t, request_t>;

        // singleton or transient?
        if constexpr (std::same_as<effective_scope_t, scope::singleton_t>)
        {
            // singleton

            // check cache
            if (auto cached = request_traits_.template find_in_cache<request_t>(cache_))
            {
                return request_traits_.template as_requested<request_t>(cached);
            }
        }

        // delegate?
        if constexpr (decltype(auto) delegate_result = delegate_.template delegate<request_t, dependency_chain_t>();
                      !std::is_same_v<decltype(delegate_result), not_found_t>)
        {
            // delegate
            return request_traits_.template as_requested<request_t>(delegate_result);
        }

        // use default provider
        return resolve_with_default_provider<request_t, dependency_chain_t>();
    }

    //! resolves using default provider (auto-wired constructor)
    template <typename request_t, typename dependency_chain_t>
    auto resolve_with_default_provider() -> as_returnable_t<request_t>
    {
        auto default_provider = default_provider_factory_.template create<request_t>();
        using default_provider_t = decltype(default_provider);

        // default providers have default scope
        using default_effective_scope_t = effective_scope_t<typename default_provider_t::default_scope_t, request_t>;
        return invoke_provider<request_t, dependency_chain_t, default_effective_scope_t>(default_provider);
    }

    // -----------------------------------------------------------------------------------------------------------------
    // provider invocation
    // -----------------------------------------------------------------------------------------------------------------

    //! invokes provider respecting the effective scope
    template <typename request_t, typename dependency_chain_t, typename scope_t, typename provider_t>
    auto invoke_provider(provider_t& provider) -> as_returnable_t<request_t>
    {
        if constexpr (std::same_as<scope_t, scope::singleton_t>)
        {
            return invoke_provider_singleton<request_t, dependency_chain_t>(provider);
        }
        else { return invoke_provider_transient<request_t, dependency_chain_t>(provider); }
    }

    //! invokes provider and caches result
    template <typename request_t, typename dependency_chain_t, typename provider_t>
    auto invoke_provider_singleton(provider_t& provider) -> as_returnable_t<request_t>
    {        
        // create and cache atomically via resolve_from_cache
        return request_traits_.template as_requested<request_t>(
            request_traits_.template resolve_from_cache<request_t, typename provider_t::provided_t>(cache_, [&]() {
                return provider.template create<dependency_chain_t>(*this);
            })
        );
    }

    //! invokes provider without caching
    template <typename request_t, typename dependency_chain_t, typename provider_t>
    auto invoke_provider_transient(provider_t& provider) -> as_returnable_t<request_t>
    {
        return request_traits_.template as_requested<request_t>(provider.template create<dependency_chain_t>(*this));
    }

    cache_t cache_;
    config_t config_;
    [[no_unique_address]] delegate_t delegate_;
    [[no_unique_address]] default_provider_factory_t default_provider_factory_;
    [[no_unique_address]] request_traits_t request_traits_;
};

// ---------------------------------------------------------------------------------------------------------------------
// named policies
// ---------------------------------------------------------------------------------------------------------------------

//! common policy
struct container_policy_t
{
    using default_provider_factory_t = provider::default_factory_t;
    using request_traits_t = request_traits_t;
};

//! policy for root containers (no parent delegation)
struct root_container_policy_t : container_policy_t
{
    using cache_t = cache::type_indexed_t<>;
    using delegate_t = delegate::none_t;
};

//! policy for nested containers (delegates to parent)
template <typename parent_container_t>
struct nested_container_policy_t : container_policy_t
{
    using cache_t = cache::hash_table_t;
    using delegate_t = delegate::to_parent_t<parent_container_t>;
};

// ---------------------------------------------------------------------------------------------------------------------
// deduction guides
// ---------------------------------------------------------------------------------------------------------------------

//! deduction guide for root containers
template <is_binding... bindings_t>
container_t(bindings_t&&...) -> container_t<
    root_container_policy_t, detail::config_from_tuple_t<decltype(resolve_bindings(std::declval<bindings_t>()...))>>;

//! deduction guide for nested containers
template <is_container parent_container_t, is_binding... bindings_t>
container_t(parent_container_t& parent_container, bindings_t&&...) -> container_t<
    nested_container_policy_t<parent_container_t>,
    detail::config_from_tuple_t<decltype(resolve_bindings(std::declval<bindings_t>()...))>>;

// ---------------------------------------------------------------------------------------------------------------------
// type aliases
// ---------------------------------------------------------------------------------------------------------------------

//! root container with given bindings
template <is_binding... bindings_t>
using root_container_t = container_t<
    root_container_policy_t, detail::config_from_tuple_t<decltype(resolve_bindings(std::declval<bindings_t>()...))>>;

//! nested container with given parent and bindings
template <is_container parent_container_t, typename... bindings_t>
using nested_container_t = container_t<
    nested_container_policy_t<parent_container_t>,
    detail::config_from_tuple_t<decltype(resolve_bindings(std::declval<bindings_t>()...))>>;

} // namespace dink
