/*!
    \file
    Copyright (C) 2025 Frank Secilia
*/

#pragma once

#include <dink/lib.hpp>
#include <dink/bindings.hpp>
#include <dink/cache/hash_table.hpp>
#include <dink/cache/type_indexed.hpp>
#include <dink/cache_adapter.hpp>
#include <dink/config.hpp>
#include <dink/delegate.hpp>
#include <dink/not_found.hpp>
#include <dink/provider.hpp>
#include <dink/request_adapter.hpp>
#include <dink/resolution_strategy.hpp>
#include <dink/resolver.hpp>
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
};

template <typename container_t>
concept is_container = requires(container_t& container) {
    { container.template resolve<meta::concept_probe_t>() } -> std::same_as<meta::concept_probe_t>;
};

// ---------------------------------------------------------------------------------------------------------------------
// container class
// ---------------------------------------------------------------------------------------------------------------------

template <is_container_policy policy_t, is_config config_t>
class container_t {
public:
    using cache_t                    = policy_t::cache_t;
    using delegate_t                 = policy_t::delegate_t;
    using default_provider_factory_t = policy_t::default_provider_factory_t;

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

    //! direct construction from components
    container_t(cache_t cache, config_t config, delegate_t delegate,
                default_provider_factory_t default_provider_factory) noexcept
        : cache_{std::move(cache)},
          config_{std::move(config)},
          delegate_{std::move(delegate)},
          default_provider_factory_{std::move(default_provider_factory)} {}

    // -----------------------------------------------------------------------------------------------------------------
    // resolution
    // -----------------------------------------------------------------------------------------------------------------

    //! finds or creates an instance of type T
    template <typename T>
    auto resolve() -> as_returnable_t<T> {
        using canonical_t = canonical_t<T>;

        static_assert(std::is_object_v<canonical_t> || std::is_reference_v<T>,
                      "Cannot resolve: request_t must be an object type, reference, or pointer");

        static_assert(!std::is_void_v<canonical_t>, "Cannot resolve void");

        static_assert(!std::is_function_v<std::remove_pointer_t<canonical_t>>,
                      "Cannot resolve function types directly");

        static_assert(!std::is_const_v<std::remove_reference_t<T>> || std::is_reference_v<T> || std::is_pointer_v<T>,
                      "Requesting 'const T' by value - did you mean 'const T&'?");

        static_assert(!std::is_array_v<std::remove_reference_t<T>>,
                      "Cannot resolve arrays directly - request the element type instead");

        return resolve_arg<T>();
    }

    //! finds or creates an instance of type request_t
    template <typename request_t, typename dependency_chain_t = type_list_t<>,
              stability_t stability = stability_t::transient>
    auto resolve_arg() -> as_returnable_t<request_t> {
        using resolver_policy_t = resolver_policy_t<request_t, type_list_t<>, stability_t::transient>;
        auto resolver =
            resolver_t<resolver_policy_t, request_t, type_list_t<>, stability_t::transient>{resolver_policy_t{}};
        return resolver.resolve(*this, cache_, config_, delegate_, default_provider_factory_);
    }

    // called by delegate during parent lookup - forwards to resolver
    template <typename request_t, typename resolver_t, typename on_found_t, typename on_not_found_t>
    auto resolve_or_delegate(resolver_t& resolver, on_found_t&& on_found, on_not_found_t&& on_not_found)
        -> as_returnable_t<request_t> {
        return resolver.resolve_or_delegate(cache_, config_, delegate_, std::forward<on_found_t>(on_found),
                                            std::forward<on_not_found_t>(on_not_found));
    }

private:
    cache_t                                          cache_;
    [[no_unique_address]] config_t                   config_;
    [[no_unique_address]] delegate_t                 delegate_;
    [[no_unique_address]] default_provider_factory_t default_provider_factory_;
};

// ---------------------------------------------------------------------------------------------------------------------
// named policies
// ---------------------------------------------------------------------------------------------------------------------

//! common policy
struct container_policy_t {
    using default_provider_factory_t = provider::default_factory_t;
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
