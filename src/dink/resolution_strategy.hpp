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

namespace strategies {

enum class type_t {
    use_accessor,      // accessor providers bypass all caching and creation
    always_create,     // never check cache, always create fresh (truly transient)
    cached_singleton,  // check cache, create and cache if needed, return reference/pointer to cached
    copy_from_cache    // check cache (creating/caching if needed), return copy/move of cached value
};

// base template declaration
template <typename request_t, typename dependency_chain_t, stability_t stability, type_t type>
struct strategy_t;

// --------------------------------------------------------------------------------------------------------------------
// use_accessor: accessor providers bypass all caching
// --------------------------------------------------------------------------------------------------------------------

template <typename request_t, typename dependency_chain_t, stability_t stability>
struct strategy_t<request_t, dependency_chain_t, stability, type_t::use_accessor> {
    static constexpr stability_t resolved_stability = stability_t::singleton;

    template <typename cache_t, typename cache_adapter_t, typename provider_t, typename request_adapter_t,
              typename container_t>
    auto resolve(cache_t&, cache_adapter_t&, provider_t& provider, request_adapter_t& request_adapter,
                 container_t&) const -> as_returnable_t<request_t> {
        assert_noncaptive<stability, resolved_stability>();
        return request_adapter.as_requested(provider.get());
    }
};

// --------------------------------------------------------------------------------------------------------------------
// always_create: never check cache, always create fresh
// --------------------------------------------------------------------------------------------------------------------

template <typename request_t, typename dependency_chain_t, stability_t stability>
struct strategy_t<request_t, dependency_chain_t, stability, type_t::always_create> {
    static constexpr stability_t resolved_stability = stability_t::transient;

    template <typename cache_t, typename cache_adapter_t, typename provider_t, typename request_adapter_t,
              typename container_t>
    auto resolve(cache_t&, cache_adapter_t&, provider_t& provider, request_adapter_t& request_adapter,
                 container_t& container) const -> as_returnable_t<request_t> {
        assert_noncaptive<stability, resolved_stability>();
        return request_adapter.as_requested(
            provider.template create<request_t, dependency_chain_t, stability_t::transient>(container));
    }
};

// --------------------------------------------------------------------------------------------------------------------
// cached_singleton: check cache, create and cache if needed, return reference/pointer to cached
// --------------------------------------------------------------------------------------------------------------------

template <typename request_t, typename dependency_chain_t, stability_t stability>
struct strategy_t<request_t, dependency_chain_t, stability, type_t::cached_singleton> {
    static constexpr stability_t resolved_stability = stability_t::singleton;

    template <typename cache_t, typename cache_adapter_t, typename provider_t, typename request_adapter_t,
              typename container_t>
    auto resolve(cache_t& cache, cache_adapter_t& cache_adapter, provider_t& provider,
                 request_adapter_t& request_adapter, container_t& container) const -> as_returnable_t<request_t> {
        assert_noncaptive<stability, resolved_stability>();
        return request_adapter.as_requested(
            cache_adapter.template get_or_create<typename provider_t::provided_t>(cache, [&]() {
                return provider.template create<resolved_t<request_t>, dependency_chain_t, stability>(container);
            }));
    }
};

// --------------------------------------------------------------------------------------------------------------------
// copy_from_cache: check cache (creating/caching if needed), return copy/move of cached value
// --------------------------------------------------------------------------------------------------------------------

template <typename request_t, typename dependency_chain_t, stability_t stability>
struct strategy_t<request_t, dependency_chain_t, stability, type_t::copy_from_cache> {
    static constexpr stability_t resolved_stability = stability_t::transient;

    template <typename cache_t, typename cache_adapter_t, typename provider_t, typename request_adapter_t,
              typename container_t>
    auto resolve(cache_t& cache, cache_adapter_t& cache_adapter, provider_t& provider,
                 request_adapter_t& request_adapter, container_t& container) const -> as_returnable_t<request_t> {
        assert_noncaptive<stability, resolved_stability>();
        return request_adapter.as_requested(
            element_type(cache_adapter.template get_or_create<typename provider_t::provided_t>(cache, [&]() {
                return provider.template create<resolved_t<request_t>, dependency_chain_t, stability_t::transient>(
                    container);
            })));
    }
};

}  // namespace strategies

template <typename request_t, typename dependency_chain_t, stability_t stability>
class strategy_factory_t {
public:
    template <typename binding_or_not_found_t>
    auto create(binding_or_not_found_t) const {
        constexpr auto resolution = select<binding_or_not_found_t>();
        return strategies::strategy_t<request_t, dependency_chain_t, stability, resolution>{};
    }

private:
    // selects strategy based on request type and binding (or lack thereof)
    template <typename binding_or_not_found_t>
    static consteval auto select() -> strategies::type_t {
        constexpr bool has_binding = !std::is_same_v<binding_or_not_found_t, not_found_t>;

        if constexpr (has_binding) {
            // has binding; use behavior based on request and bound scope

            using binding_t  = std::remove_pointer_t<binding_or_not_found_t>;
            using provider_t = typename binding_t::provider_type;
            using scope_t    = typename binding_t::scope_type;

            // types bound with accessor providers have their own strategy
            if constexpr (provider::is_accessor<provider_t>) return strategies::type_t::use_accessor;

            // types with persistent reference semantics are always singleton
            if constexpr (std::is_lvalue_reference_v<request_t> || std::is_pointer_v<request_t> ||
                          is_shared_ptr_v<request_t> || is_weak_ptr_v<request_t>) {
                return strategies::type_t::cached_singleton;
            }

            // for value types, rvalue references, and unique_ptr, the strategy depends on the scope
            return std::same_as<scope_t, scope::singleton_t> ? strategies::type_t::copy_from_cache
                                                             : strategies::type_t::always_create;
        } else {
            // no binding; use default behavior based on request type
            return std::is_lvalue_reference_v<request_t> || std::is_pointer_v<request_t> || is_shared_ptr_v<request_t>
                       ? strategies::type_t::cached_singleton
                       : strategies::type_t::always_create;
        }
    }
};

}  // namespace dink::resolution
