/*!
    \file
    Copyright (C) 2025 Frank Secilia
*/

#pragma once

#include <dink/lib.hpp>
#include <dink/bindings.hpp>
#include <dink/cache/hash_table.hpp>
#include <dink/cache/type_indexed.hpp>
#include <dink/cache_traits.hpp>
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
    typename policy_t::cache_traits_t;
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
class container_t {
public:
    using cache_t                    = policy_t::cache_t;
    using cache_traits_t             = policy_t::cache_traits_t;
    using delegate_t                 = policy_t::delegate_t;
    using default_provider_factory_t = policy_t::default_provider_factory_t;
    using request_traits_t           = policy_t::request_traits_t;

    // -----------------------------------------------------------------------------------------------------------------
    // constructors
    // -----------------------------------------------------------------------------------------------------------------

    //! constructs root container with given bindings
    template <is_binding... bindings_t>
    explicit container_t(bindings_t&&... bindings) noexcept
        : config_{resolve_bindings(std::forward<bindings_t>(bindings)...)} {}

    //! constructs nested container with given parent and bindings
    template <is_container parent_t, is_binding... bindings_t>
    explicit container_t(parent_t& parent, bindings_t&&... bindings) noexcept
        : config_{resolve_bindings(std::forward<bindings_t>(bindings)...)}, delegate_{parent} {}

    //! direct construction from components (used by deduction guides and testing)
    container_t(cache_t cache, cache_traits_t cache_traits, config_t config, delegate_t delegate,
                default_provider_factory_t default_provider_factory, request_traits_t request_traits) noexcept
        : cache_{std::move(cache)},
          cache_traits_{std::move(cache_traits)},
          config_{std::move(config)},
          delegate_{std::move(delegate)},
          default_provider_factory_{std::move(default_provider_factory)},
          request_traits_{std::move(request_traits)} {}

    // -----------------------------------------------------------------------------------------------------------------
    // resolution
    // -----------------------------------------------------------------------------------------------------------------

    template <typename request_t, typename dependency_chain_t = type_list_t<>>
    auto resolve() -> as_returnable_t<request_t> {
        using resolved_t = resolved_t<request_t>;

        auto                  binding       = config_.template find_binding<resolved_t>();
        static constexpr auto binding_found = !std::is_same_v<decltype(binding), not_found_t>;
        if constexpr (binding_found) {
            // check cache

            // dispatch

            return dispatch<request_t, dependency_chain_t>(binding, binding->provider);
        } else {
            // check cache

            // try delegating to parent
            if constexpr (decltype(auto) delegate_result = delegate_.template delegate<request_t, dependency_chain_t>();
                          !std::is_same_v<decltype(delegate_result), not_found_t>) {
                return request_traits_.template as_requested<request_t>(delegate_result);
            }

            auto default_provider = default_provider_factory_.template create<request_t>();
            return dispatch<request_t, dependency_chain_t>(binding, default_provider);
        }
    }

    template <typename request_t, typename dependency_chain_t, typename binding_t, typename provider_t>
    auto dispatch(binding_t binding, provider_t& provider) -> as_returnable_t<request_t> {
        static constexpr auto resolution     = select_resolution<request_t, binding_t>();
        static constexpr auto implementation = resolution_to_implementation(resolution);
        if constexpr (implementation == implementation_t::use_accessor) {
            return use_accessor<request_t, dependency_chain_t>(binding, provider);
        } else if constexpr (implementation == implementation_t::create) {
            return create<request_t, dependency_chain_t>(binding, provider);
        } else if constexpr (implementation == implementation_t::cache) {
            return cache<request_t, dependency_chain_t>(binding, provider);
        }
    }

private:
    // -----------------------------------------------------------------------------------------------------------------
    // unique operation implementation
    // -----------------------------------------------------------------------------------------------------------------

    template <typename request_t, typename dependency_chain_t, typename binding_t, typename provider_t>
    auto use_accessor(binding_t, provider_t& provider) -> as_returnable_t<request_t> {
        // accessor bypasses all caching
        return request_traits_.template as_requested<request_t>(provider.get());
    }

    template <typename request_t, typename dependency_chain_t, typename binding_t, typename provider_t>
    auto create(binding_t, provider_t& provider) -> as_returnable_t<request_t> {
        return request_traits_.template as_requested<request_t>(provider.template create<dependency_chain_t>(*this));
    }

    template <typename request_t, typename dependency_chain_t, typename binding_t, typename provider_t>
    auto cache(binding_t, provider_t& provider) -> as_returnable_t<request_t> {
        return request_traits_.template as_requested<request_t>(
            cache_traits_.template resolve<request_t, typename provider_t::provided_t>(
                cache_, [&]() { return provider.template create<dependency_chain_t>(*this); }));
    }

    cache_t                                          cache_;
    [[no_unique_address]] cache_traits_t             cache_traits_;
    [[no_unique_address]] config_t                   config_;
    [[no_unique_address]] delegate_t                 delegate_;
    [[no_unique_address]] default_provider_factory_t default_provider_factory_;
    [[no_unique_address]] request_traits_t           request_traits_;
};

// ---------------------------------------------------------------------------------------------------------------------
// named policies
// ---------------------------------------------------------------------------------------------------------------------

//! common policy
struct container_policy_t {
    using default_provider_factory_t = provider::default_factory_t;
    using cache_traits_t             = cache_traits_t;
    using request_traits_t           = request_traits_t;
};

//! policy for root containers (no parent delegation)
struct root_container_policy_t : container_policy_t {
    using cache_t    = caches::type_indexed_t<>;
    using delegate_t = delegate::none_t;
};

//! policy for nested containers (delegates to parent)
template <typename parent_container_t>
struct nested_container_policy_t : container_policy_t {
    using cache_t    = caches::hash_table_t;
    using delegate_t = delegate::to_parent_t<parent_container_t>;
};

// ---------------------------------------------------------------------------------------------------------------------
// deduction guides
// ---------------------------------------------------------------------------------------------------------------------

//! deduction guide for root containers
template <is_binding... bindings_t>
container_t(bindings_t&&...)
    -> container_t<root_container_policy_t,
                   detail::config_from_tuple_t<decltype(resolve_bindings(std::declval<bindings_t>()...))>>;

//! deduction guide for nested containers
template <is_container parent_container_t, is_binding... bindings_t>
container_t(parent_container_t& parent_container, bindings_t&&...)
    -> container_t<nested_container_policy_t<parent_container_t>,
                   detail::config_from_tuple_t<decltype(resolve_bindings(std::declval<bindings_t>()...))>>;

// ---------------------------------------------------------------------------------------------------------------------
// type aliases
// ---------------------------------------------------------------------------------------------------------------------

//! root container with given bindings
template <is_binding... bindings_t>
using root_container_t =
    container_t<root_container_policy_t,
                detail::config_from_tuple_t<decltype(resolve_bindings(std::declval<bindings_t>()...))>>;

//! nested container with given parent and bindings
template <is_container parent_container_t, typename... bindings_t>
using nested_container_t =
    container_t<nested_container_policy_t<parent_container_t>,
                detail::config_from_tuple_t<decltype(resolve_bindings(std::declval<bindings_t>()...))>>;

}  // namespace dink
