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

namespace dink::resolution {

template <stability_t stability, stability_t dependency_stability>
constexpr auto assert_noncaptive() noexcept -> void {
    static_assert(stability <= dependency_stability,
                  "captive dependency detected: longer-lived instance cannot depend on shorter-lived instance");
}

// =====================================================================================================================
// Strategies
// =====================================================================================================================

enum class strategy_type_t {
    use_accessor,      // accessor providers bypass all caching and creation
    always_create,     // never check cache, always create fresh (truly transient)
    cached_singleton,  // check cache, create and cache if needed, return reference/pointer to cached
    copy_from_cache    // check cache (creating/caching if needed), return copy/move of cached value
};

// base template declaration
template <typename request_t, typename dependency_chain_t, stability_t stability, strategy_type_t type>
struct strategy_t;

// --------------------------------------------------------------------------------------------------------------------
// use_accessor: accessor providers bypass all caching
// --------------------------------------------------------------------------------------------------------------------

template <typename request_t, typename dependency_chain_t, stability_t stability>
struct strategy_t<request_t, dependency_chain_t, stability, strategy_type_t::use_accessor> {
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
struct strategy_t<request_t, dependency_chain_t, stability, strategy_type_t::always_create> {
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
struct strategy_t<request_t, dependency_chain_t, stability, strategy_type_t::cached_singleton> {
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
struct strategy_t<request_t, dependency_chain_t, stability, strategy_type_t::copy_from_cache> {
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
class strategy_factory_t {
public:
    template <typename binding_or_not_found_t>
    auto create(binding_or_not_found_t) const {
        constexpr auto strategy_type = choose_strategy_type<binding_or_not_found_t>();
        return strategy_t<request_t, dependency_chain_t, stability, strategy_type>{};
    }

private:
    // choose strategy based on request type and binding (or lack thereof)
    template <typename binding_or_not_found_t>
    static consteval auto choose_strategy_type() -> strategy_type_t {
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
            if constexpr (provider::is_accessor<provider_t>) return strategy_type_t::use_accessor;

            // types with reference semantics are always singleton
            if constexpr (is_shared) return strategy_type_t::cached_singleton;

            // for value types, rvalue references, and unique_ptr, the strategy depends on the scope
            return std::same_as<scope_t, scope::singleton_t> ? strategy_type_t::copy_from_cache
                                                             : strategy_type_t::always_create;
        } else {
            // no binding was found; choose strategy based on request type alone

            return is_shared ? strategy_type_t::cached_singleton : strategy_type_t::always_create;
        }
    }
};

}  // namespace dink::resolution
