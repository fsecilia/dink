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
class container_impl_t {
public:
    using cache_t                    = policy_t::cache_t;
    using delegate_t                 = policy_t::delegate_t;
    using default_provider_factory_t = policy_t::default_provider_factory_t;

    // -----------------------------------------------------------------------------------------------------------------
    // constructors
    // -----------------------------------------------------------------------------------------------------------------

    //! constructs root container with given bindings
    template <is_binding... bindings_t>
    explicit container_impl_t(bindings_t&&... bindings) noexcept
        : config_{resolve_bindings(std::forward<bindings_t>(bindings)...)} {}

    //! constructs nested container with given parent and bindings
    template <is_container parent_t, is_binding... bindings_t>
    explicit container_impl_t(parent_t& parent, bindings_t&&... bindings) noexcept
        : config_{resolve_bindings(std::forward<bindings_t>(bindings)...)}, delegate_{parent} {}

    //! direct construction from components
    container_impl_t(cache_t cache, config_t config, delegate_t delegate,
                     default_provider_factory_t default_provider_factory) noexcept
        : cache_{std::move(cache)},
          config_{std::move(config)},
          delegate_{std::move(delegate)},
          default_provider_factory_{std::move(default_provider_factory)} {}

    // -----------------------------------------------------------------------------------------------------------------
    // resolution
    // -----------------------------------------------------------------------------------------------------------------

    //! finds or creates an instance of type request_t
    template <typename request_t, typename dependency_chain_t = type_list_t<>,
              stability_t stability = stability_t::transient>
    auto resolve() -> as_returnable_t<request_t> {
        auto resolver = resolver_t<cache_traits_t<request_t>, request_traits_t<request_t>, dependency_chain_t,
                                   stability, container_impl_t>{*this, cache_};
        return resolver.resolve();
    }

    // resolve_or_delegate implementation - called by children during delegation
    template <typename request_t, typename on_found_t, typename on_not_found_t>
    auto resolve_or_delegate(on_found_t&& on_found, on_not_found_t&& on_not_found) -> as_returnable_t<request_t> {
        // check local cache
        auto cache_traits = cache_traits_t<request_t>{};
        if (auto cached = cache_traits.find(cache_)) {
            auto request_traits = request_traits_t<request_t>{};
            return request_traits.from_cached(cached);
        }

        // check local binding
        auto local_binding = config_.template find_binding<resolved_t<request_t>>();
        if constexpr (!std::is_same_v<decltype(local_binding), not_found_t>) { return on_found(local_binding); }

        // recurse to parent
        return delegate_.template find_in_parent<request_t>(std::forward<on_found_t>(on_found),
                                                            std::forward<on_not_found_t>(on_not_found));
    }

    cache_t                                          cache_;
    [[no_unique_address]] config_t                   config_;
    [[no_unique_address]] delegate_t                 delegate_;
    [[no_unique_address]] default_provider_factory_t default_provider_factory_;
};

template <is_container_policy policy_t, is_config config_t>
class container_t : public container_impl_t<policy_t, config_t> {
    using impl_t = container_impl_t<policy_t, config_t>;

public:
    using impl_t::impl_t;

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

        return impl_t::template resolve<T>();
    }
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
