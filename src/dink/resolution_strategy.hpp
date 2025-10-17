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

namespace dink::resolver::strategy {

template <stability_t stability, stability_t dependency_stability>
constexpr auto assert_noncaptive() noexcept -> void {
    static_assert(stability <= dependency_stability,
                  "captive dependency detected: longer-lived instance cannot depend on shorter-lived instance");
}

// =====================================================================================================================
// Strategies
// =====================================================================================================================

// --------------------------------------------------------------------------------------------------------------------
// use_accessor: accessor providers bypass all caching
// --------------------------------------------------------------------------------------------------------------------

template <typename request_t, typename dependency_chain_t, stability_t stability>
struct use_accessor_t {
    template <typename cache_t, typename cache_adapter_t, typename provider_t, typename request_adapter_t,
              typename container_t>
    auto resolve(cache_t&, cache_adapter_t&, provider_t& provider, request_adapter_t& request_adapter,
                 container_t&) const -> as_returnable_t<request_t> {
        constexpr auto dependency_stability = stability_t::singleton;
        assert_noncaptive<stability, dependency_stability>();

        return request_adapter.as_requested(provider.get());
    }
};

// --------------------------------------------------------------------------------------------------------------------
// always_create: never check cache, always create fresh
// --------------------------------------------------------------------------------------------------------------------

template <typename request_t, typename dependency_chain_t, stability_t stability>
struct always_create_t {
    template <typename cache_t, typename cache_adapter_t, typename provider_t, typename request_adapter_t,
              typename container_t>
    auto resolve(cache_t&, cache_adapter_t&, provider_t& provider, request_adapter_t& request_adapter,
                 container_t& container) const -> as_returnable_t<request_t> {
        constexpr auto dependency_stability = stability_t::transient;
        assert_noncaptive<stability, dependency_stability>();

        // transients propagate transient stability
        constexpr auto propagated_stability = dependency_stability;

        return request_adapter.as_requested(
            provider.template create<request_t, dependency_chain_t, propagated_stability>(container));
    }
};

// --------------------------------------------------------------------------------------------------------------------
// cached_singleton: check cache, create and cache if needed, return reference/pointer to cached
// --------------------------------------------------------------------------------------------------------------------

template <typename request_t, typename dependency_chain_t, stability_t stability>
struct cached_singleton_t {
    template <typename cache_t, typename cache_adapter_t, typename provider_t, typename request_adapter_t,
              typename container_t>
    auto resolve(cache_t& cache, cache_adapter_t& cache_adapter, provider_t& provider,
                 request_adapter_t& request_adapter, container_t& container) const -> as_returnable_t<request_t> {
        constexpr auto dependency_stability = stability_t::singleton;
        assert_noncaptive<stability, dependency_stability>();

        // singletons propagate parent stability
        constexpr auto propagated_stability = stability;

        auto factory = [&]() {
            return provider.template create<resolved_t<request_t>, dependency_chain_t, propagated_stability>(container);
        };

        auto&& cached = cache_adapter.template get_or_create<typename provider_t::provided_t>(cache, factory);
        return request_adapter.as_requested(std::forward<decltype(cached)>(cached));
    }
};

// --------------------------------------------------------------------------------------------------------------------
// copy_from_cache: check cache (creating/caching if needed), return copy/move of cached value
// --------------------------------------------------------------------------------------------------------------------

template <typename request_t, typename dependency_chain_t, stability_t stability>
struct copy_from_cache_t {
    template <typename cache_t, typename cache_adapter_t, typename provider_t, typename request_adapter_t,
              typename container_t>
    auto resolve(cache_t& cache, cache_adapter_t& cache_adapter, provider_t& provider,
                 request_adapter_t& request_adapter, container_t& container) const -> as_returnable_t<request_t> {
        constexpr auto dependency_stability = stability_t::transient;
        assert_noncaptive<stability, dependency_stability>();

        // transients propagate transient stability
        constexpr auto propagated_stability = dependency_stability;

        auto factory = [&]() {
            return provider.template create<resolved_t<request_t>, dependency_chain_t, propagated_stability>(container);
        };

        auto&& cached = cache_adapter.template get_or_create<typename provider_t::provided_t>(cache, factory);
        return request_adapter.as_requested(element_type(std::forward<decltype(cached)>(cached)));
    }
};

template <typename request_t, typename dependency_chain_t, stability_t stability>
class factory_t {
public:
    template <typename binding_or_not_found_t>
    auto create(binding_or_not_found_t) const -> auto {
        constexpr bool binding_found = !std::is_same_v<binding_or_not_found_t, not_found_t>;

        // these types don't request ownership
        static constexpr auto is_shared = std::is_lvalue_reference_v<request_t> || std::is_pointer_v<request_t> ||
                                          is_shared_ptr_v<request_t> || is_weak_ptr_v<request_t>;

        if constexpr (binding_found) {
            // a binding was found; choose strategy based on request and bound scope

            using binding_t  = std::remove_pointer_t<binding_or_not_found_t>;
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
    template <template <typename, typename, stability_t> class strategy_t>
    auto select() const -> auto {
        return strategy_t<request_t, dependency_chain_t, stability>{};
    }
};

}  // namespace dink::resolver::strategy
