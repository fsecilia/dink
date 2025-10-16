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

namespace dink {

// =====================================================================================================================
// Strategy Selection
// =====================================================================================================================

enum class resolution_t {
    use_accessor,      // accessor providers bypass all caching and creation
    always_create,     // never check cache, always create fresh (truly transient)
    cached_singleton,  // check cache, create and cache if needed, return reference/pointer to cached
    copy_from_cache    // check cache (creating/caching if needed), return copy/move of cached value
};

// selects strategy based on request type and binding (or lack thereof)
template <typename request_t, typename binding_or_not_found_t>
consteval auto select_resolution() -> resolution_t {
    constexpr bool has_binding = !std::is_same_v<binding_or_not_found_t, not_found_t>;

    if constexpr (has_binding) {
        using binding_t  = std::remove_pointer_t<binding_or_not_found_t>;
        using provider_t = typename binding_t::provider_type;
        using scope_t    = typename binding_t::scope_type;

        if constexpr (provider::is_accessor<provider_t>) {
            return resolution_t::use_accessor;
        } else if constexpr (std::is_reference_v<request_t> || std::is_pointer_v<request_t>) {
            return resolution_t::cached_singleton;
        } else if constexpr (is_shared_ptr_v<request_t> || is_weak_ptr_v<request_t>) {
            return resolution_t::cached_singleton;
        } else if constexpr (is_unique_ptr_v<request_t>) {
            if constexpr (std::same_as<scope_t, scope::singleton_t>) { return resolution_t::copy_from_cache; }
            return resolution_t::always_create;
        }
        if constexpr (std::same_as<scope_t, scope::singleton_t>) {
            return resolution_t::copy_from_cache;
        } else {
            return resolution_t::always_create;
        }
    } else {
        // no binding - use default behavior based on request type
        if constexpr (std::is_reference_v<request_t> || std::is_pointer_v<request_t> || is_shared_ptr_v<request_t>) {
            return resolution_t::cached_singleton;
        } else {
            return resolution_t::always_create;
        }
    }
}

template <stability_t stability, stability_t dependency_stability>
constexpr auto assert_noncaptive() noexcept -> void {
    static_assert(stability <= dependency_stability,
                  "captive dependency detected: longer-lived instance cannot depend on shorter-lived instance");
}

// =====================================================================================================================
// Strategy Implementations
// =====================================================================================================================

// base template declaration
template <typename request_t, typename dependency_chain_t, stability_t stability, resolution_t resolution>
struct resolution_strategy_t;

// --------------------------------------------------------------------------------------------------------------------
// use_accessor: accessor providers bypass all caching
// --------------------------------------------------------------------------------------------------------------------

template <typename request_t, typename dependency_chain_t, stability_t stability>
struct resolution_strategy_t<request_t, dependency_chain_t, stability, resolution_t::use_accessor> {
    static constexpr stability_t resolved_stability = stability_t::singleton;

    template <typename cache_t, typename cache_traits_t, typename provider_t, typename request_traits_t,
              typename container_t>
    auto resolve(cache_t&, cache_traits_t&, provider_t& provider, request_traits_t& request_traits, container_t&) const
        -> as_returnable_t<request_t> {
        assert_noncaptive<stability, resolved_stability>();
        return request_traits.as_requested(provider.get());
    }
};

// --------------------------------------------------------------------------------------------------------------------
// always_create: never check cache, always create fresh
// --------------------------------------------------------------------------------------------------------------------

template <typename request_t, typename dependency_chain_t, stability_t stability>
struct resolution_strategy_t<request_t, dependency_chain_t, stability, resolution_t::always_create> {
    static constexpr stability_t resolved_stability = stability_t::transient;

    template <typename cache_t, typename cache_traits_t, typename provider_t, typename request_traits_t,
              typename container_t>
    auto resolve(cache_t&, cache_traits_t&, provider_t& provider, request_traits_t& request_traits,
                 container_t& container) const -> as_returnable_t<request_t> {
        assert_noncaptive<stability, resolved_stability>();
        return request_traits.as_requested(
            provider.template create<request_t, dependency_chain_t, stability_t::transient>(container));
    }
};

// --------------------------------------------------------------------------------------------------------------------
// cached_singleton: check cache, create and cache if needed, return reference/pointer to cached
// --------------------------------------------------------------------------------------------------------------------

template <typename request_t, typename dependency_chain_t, stability_t stability>
struct resolution_strategy_t<request_t, dependency_chain_t, stability, resolution_t::cached_singleton> {
    static constexpr stability_t resolved_stability = stability_t::singleton;

    template <typename cache_t, typename cache_traits_t, typename provider_t, typename request_traits_t,
              typename container_t>
    auto resolve(cache_t& cache, cache_traits_t& cache_traits, provider_t& provider, request_traits_t& request_traits,
                 container_t& container) const -> as_returnable_t<request_t> {
        assert_noncaptive<stability, resolved_stability>();
        return request_traits.as_requested(
            cache_traits.template get_or_create<typename provider_t::provided_t>(cache, [&]() {
                return provider.template create<resolved_t<request_t>, dependency_chain_t, stability>(container);
            }));
    }
};

// --------------------------------------------------------------------------------------------------------------------
// copy_from_cache: check cache (creating/caching if needed), return copy/move of cached value
// --------------------------------------------------------------------------------------------------------------------

template <typename request_t, typename dependency_chain_t, stability_t stability>
struct resolution_strategy_t<request_t, dependency_chain_t, stability, resolution_t::copy_from_cache> {
    static constexpr stability_t resolved_stability = stability_t::transient;

    template <typename cache_t, typename cache_traits_t, typename provider_t, typename request_traits_t,
              typename container_t>
    auto resolve(cache_t& cache, cache_traits_t& cache_traits, provider_t& provider, request_traits_t& request_traits,
                 container_t& container) const -> as_returnable_t<request_t> {
        assert_noncaptive<stability, resolved_stability>();
        return request_traits.as_requested(
            element_type(cache_traits.template get_or_create<typename provider_t::provided_t>(cache, [&]() {
                return provider.template create<resolved_t<request_t>, dependency_chain_t, stability_t::transient>(
                    container);
            })));
    }
};

}  // namespace dink
