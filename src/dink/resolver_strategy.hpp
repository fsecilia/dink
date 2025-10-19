/*!
    \file
    Copyright (C) 2025 Frank Secilia
*/

#pragma once

#include <dink/lib.hpp>
#include <dink/cache_adapter.hpp>
#include <dink/provider.hpp>
#include <dink/scope.hpp>
#include <dink/smart_pointer_traits.hpp>
#include <algorithm>

namespace dink::resolver::strategy {

template <lifetime_t min_lifetime, lifetime_t dependency_lifetime>
constexpr auto assert_noncaptive() noexcept -> void {
    static_assert(min_lifetime <= dependency_lifetime,
                  "captive dependency detected: longer-lived instance cannot depend on shorter-lived instance");
}

// =====================================================================================================================
// Strategies
// =====================================================================================================================

// --------------------------------------------------------------------------------------------------------------------
// use_accessor: accessor providers bypass all caching
// --------------------------------------------------------------------------------------------------------------------

template <typename request_t, typename dependency_chain_t, lifetime_t min_lifetime>
struct use_accessor_t {
    template <typename cache_t, typename cache_adapter_t, typename provider_t, typename request_traits_t,
              typename container_t>
    auto resolve(cache_t&, cache_adapter_t&, provider_t& provider, request_traits_t& request_traits,
                 container_t&) const -> as_returnable_t<request_t> {
        // check for captive dependencies
        constexpr auto dependency_lifetime = lifetime_t::singleton;
        assert_noncaptive<min_lifetime, dependency_lifetime>();

        return request_traits.from_provided(provider.get());
    }
};

// --------------------------------------------------------------------------------------------------------------------
// always_create: never check cache, always create fresh
// --------------------------------------------------------------------------------------------------------------------

template <typename request_t, typename dependency_chain_t, lifetime_t min_lifetime>
struct always_create_t {
    template <typename cache_t, typename cache_adapter_t, typename provider_t, typename request_traits_t,
              typename container_t>
    auto resolve(cache_t&, cache_adapter_t&, provider_t& provider, request_traits_t& request_traits,
                 container_t& container) const -> as_returnable_t<request_t> {
        // check for captive dependencies
        constexpr auto dependency_lifetime = lifetime_t::transient;
        assert_noncaptive<min_lifetime, dependency_lifetime>();

        // narrow min lifetime to match current dependency
        constexpr auto propagated_lifetime = std::max(min_lifetime, dependency_lifetime);

        return request_traits.from_provided(
            provider.template create<request_t, dependency_chain_t, propagated_lifetime>(container));
    }
};

// --------------------------------------------------------------------------------------------------------------------
// cached_singleton: check cache, create and cache if needed, return reference/pointer to cached
// --------------------------------------------------------------------------------------------------------------------

template <typename request_t, typename dependency_chain_t, lifetime_t min_lifetime>
struct cached_singleton_t {
    template <typename cache_t, typename cache_adapter_t, typename provider_t, typename request_traits_t,
              typename container_t>
    auto resolve(cache_t& cache, cache_adapter_t& cache_adapter, provider_t& provider,
                 request_traits_t& request_traits, container_t& container) const -> as_returnable_t<request_t> {
        // check for captive dependencies
        constexpr auto dependency_lifetime = lifetime_t::singleton;
        assert_noncaptive<min_lifetime, dependency_lifetime>();

        /*
            Dependencies are captured during construction. If constructor captures by reference (e.g., T&), the
            reference request itself forces singleton caching of that dependency. We pass through parent's requirement,
            allowing each dependency to be evaluated independently based on its own request type.
        */
        constexpr auto propagated_lifetime = min_lifetime;

        auto factory = [&]() {
            return provider.template create<resolved_t<request_t>, dependency_chain_t, propagated_lifetime>(container);
        };

        auto&& cached = cache_adapter.template get_or_create<typename provider_t::provided_t>(cache, factory);
        return request_traits.from_provided(std::forward<decltype(cached)>(cached));
    }
};

// --------------------------------------------------------------------------------------------------------------------
// copy_from_cache: check cache (creating/caching if needed), return copy/move of cached value
// --------------------------------------------------------------------------------------------------------------------

template <typename request_t, typename dependency_chain_t, lifetime_t min_lifetime>
struct copy_from_cache_t {
    template <typename cache_t, typename cache_adapter_t, typename provider_t, typename request_traits_t,
              typename container_t>
    auto resolve(cache_t& cache, cache_adapter_t& cache_adapter, provider_t& provider,
                 request_traits_t& request_traits, container_t& container) const -> as_returnable_t<request_t> {
        // check for captive dependencies
        constexpr auto dependency_lifetime = lifetime_t::transient;
        assert_noncaptive<min_lifetime, dependency_lifetime>();

        // narrow min lifetime to match current dependency
        constexpr auto propagated_lifetime = std::max(min_lifetime, dependency_lifetime);

        auto factory = [&]() {
            return provider.template create<resolved_t<request_t>, dependency_chain_t, propagated_lifetime>(container);
        };

        auto&& cached = cache_adapter.template get_or_create<typename provider_t::provided_t>(cache, factory);
        return request_traits.from_provided(element_type(std::forward<decltype(cached)>(cached)));
    }
};

template <typename request_t, typename dependency_chain_t, lifetime_t min_lifetime>
class factory_t {
public:
    template <typename binding_or_nullptr_t>
    auto create() const -> auto {
        constexpr bool binding_found = !std::is_same_v<binding_or_nullptr_t, std::nullptr_t>;

        // these types don't request ownership
        static constexpr auto is_shared = std::is_lvalue_reference_v<request_t> || std::is_pointer_v<request_t> ||
                                          is_shared_ptr_v<request_t> || is_weak_ptr_v<request_t>;

        if constexpr (binding_found) {
            // a binding was found; choose strategy based on request and bound scope

            using binding_t  = std::remove_pointer_t<binding_or_nullptr_t>;
            using provider_t = typename binding_t::provider_type;
            using scope_t    = typename binding_t::scope_type;

            // types bound with accessor providers have their own strategy
            if constexpr (provider::is_accessor<provider_t>) return select<use_accessor_t>();

            // types with reference semantics are always singleton
            else if constexpr (is_shared) return select<cached_singleton_t>();

            // for value types, rvalue references, and unique_ptr, the strategy depends on the scope
            else if constexpr (std::same_as<scope_t, scope::singleton_t>) return select<copy_from_cache_t>();
            else return select<always_create_t>();
        } else {
            // no binding was found; choose strategy based on request type alone

            if constexpr (is_shared) return select<cached_singleton_t>();
            else return select<always_create_t>();
        }
    }

private:
    // choose strategy based on request type and binding (or lack thereof)
    template <template <typename, typename, lifetime_t> class strategy_t>
    auto select() const -> auto {
        return strategy_t<request_t, dependency_chain_t, min_lifetime>{};
    }
};

}  // namespace dink::resolver::strategy
