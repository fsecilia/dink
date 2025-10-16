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
#include <dink/resolution_strategy.hpp>
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
        // search self and parent chain with two continuations
        return search<request_t>(
            [&](auto found_binding) -> as_returnable_t<request_t> {
                return on_binding_found<request_t, dependency_chain_t>(found_binding);
            },
            [&]() -> as_returnable_t<request_t> { return on_binding_not_found<request_t, dependency_chain_t>(); });
    }

    // search implementation - called by children during delegation
    // continuation-passing style: calls on_found if binding found, otherwise recurses or calls on_not_found
    template <typename request_t, typename on_found_t, typename on_not_found_t>
    auto search(on_found_t&& on_found, on_not_found_t&& on_not_found) -> as_returnable_t<request_t> {
        // 1. Check local cache - if found, return it directly
        if (auto cached = cache_traits_.template find<cache_key_t<request_t>>(cache_)) {
            return request_traits_.template from_cached<request_t>(cached);
        }

        // 2. Check local binding - if found, call on_found continuation (executes in originator context)
        auto local_binding = config_.template find_binding<resolved_t<request_t>>();
        if constexpr (!std::is_same_v<decltype(local_binding), not_found_t>) { return on_found(local_binding); }

        // 3. Recurse to parent with same continuations
        return delegate_.template search<request_t>(std::forward<on_found_t>(on_found),
                                                    std::forward<on_not_found_t>(on_not_found));
    }

private:
    // on_found: binding found (locally or in parent), create in originator context
    template <typename request_t, typename dependency_chain_t, typename found_binding_t>
    auto on_binding_found(found_binding_t found_binding) -> as_returnable_t<request_t> {
        static_assert(!std::is_same_v<decltype(found_binding), not_found_t>);
        static constexpr auto found_strategy = select_resolution_strategy<request_t, decltype(found_binding)>();
        return resolution_strategy<found_strategy>::template resolve<request_t, dependency_chain_t>(
            cache_, cache_traits_, found_binding->provider, request_traits_, *this);
    }

    // on_not_found: nothing found anywhere, create with default provider in originator context
    template <typename request_t, typename dependency_chain_t>
    auto on_binding_not_found() -> as_returnable_t<request_t> {
        static constexpr auto strategy         = select_resolution_strategy<request_t, not_found_t>();
        auto                  default_provider = default_provider_factory_.template create<resolved_t<request_t>>();
        return resolution_strategy<strategy>::template resolve<request_t, dependency_chain_t>(
            cache_, cache_traits_, default_provider, request_traits_, *this);
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
