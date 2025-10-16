/*!
    \file
    Copyright (C) 2025 Frank Secilia
*/

#pragma once

#include <dink/lib.hpp>
#include <dink/cache_traits.hpp>
#include <dink/provider.hpp>
#include <dink/scope.hpp>
#include <dink/smart_pointer_traits.hpp>

namespace dink {

// =====================================================================================================================
// Strategy Selection
// =====================================================================================================================

enum class resolution_strategy_t {
    use_accessor,      // accessor providers bypass all caching and creation
    cached_singleton,  // check cache, create and cache if needed, return reference/pointer to cached
    always_create,     // never check cache, always create fresh (truly transient)
    copy_from_cache    // check cache (creating/caching if needed), return copy/move of cached value
};

// selects strategy based on request type and binding (or lack thereof)
template <typename request_t, typename binding_or_not_found_t>
consteval auto select_resolution_strategy() -> resolution_strategy_t {
    constexpr bool has_binding = !std::is_same_v<binding_or_not_found_t, not_found_t>;

    if constexpr (has_binding) {
        using binding_t  = std::remove_pointer_t<binding_or_not_found_t>;
        using provider_t = typename binding_t::provider_type;
        using scope_t    = typename binding_t::scope_type;

        if constexpr (provider::is_accessor<provider_t>) {
            // accessor providers always bypass caching
            return resolution_strategy_t::use_accessor;
        } else if constexpr (std::is_reference_v<request_t> || std::is_pointer_v<request_t>) {
            // reference and pointer requests force singleton behavior
            return resolution_strategy_t::cached_singleton;
        } else if constexpr (is_shared_ptr_v<request_t> || is_weak_ptr_v<request_t>) {
            // shared_ptr requests use canonical cached shared_ptr
            return resolution_strategy_t::cached_singleton;
        } else if constexpr (is_unique_ptr_v<request_t>) {
            // unique_ptr requests are always transient semantically
            // but if bound as singleton, copy from the cached instance
            if constexpr (std::same_as<scope_t, scope::singleton_t>) { return resolution_strategy_t::copy_from_cache; }
            return resolution_strategy_t::always_create;
        }
        if constexpr (std::same_as<scope_t, scope::singleton_t>) {
            // value and rvalue reference requests respect bound scope
            return resolution_strategy_t::copy_from_cache;
        } else {
            return resolution_strategy_t::always_create;
        }
    } else {
        // no binding - use default behavior based on request type
        // references and pointers need singleton behavior for stable addresses
        if constexpr (std::is_reference_v<request_t> || std::is_pointer_v<request_t> || is_shared_ptr_v<request_t>) {
            return resolution_strategy_t::cached_singleton;
        } else {
            return resolution_strategy_t::always_create;
        }
    }
}

// =====================================================================================================================
// Strategy Implementations
// =====================================================================================================================

template <resolution_strategy_t strategy>
struct resolution_strategy;

// --------------------------------------------------------------------------------------------------------------------
// use_accessor: accessor providers bypass all caching
// --------------------------------------------------------------------------------------------------------------------

template <>
struct resolution_strategy<resolution_strategy_t::use_accessor> {
    template <typename request_t, typename cache_t, typename cache_traits_t>
    static auto check_cache(cache_t&, cache_traits_t&) -> std::nullptr_t {
        return nullptr;  // never checks cache
    }

    template <typename request_t, typename dependency_chain_t, typename cache_t, typename cache_traits_t,
              typename provider_t, typename request_traits_t, typename container_t>
    static auto resolve(cache_t&, cache_traits_t&, provider_t& provider, request_traits_t& request_traits, container_t&)
        -> as_returnable_t<request_t> {
        return request_traits.template as_requested<request_t>(provider.get());
    }
};

// --------------------------------------------------------------------------------------------------------------------
// cached_singleton: check cache, create and cache if needed, return reference/pointer to cached
// --------------------------------------------------------------------------------------------------------------------

template <>
struct resolution_strategy<resolution_strategy_t::cached_singleton> {
    template <typename request_t, typename cache_t, typename cache_traits_t>
    static auto check_cache(cache_t& cache, cache_traits_t& cache_traits) -> auto {
        return cache_traits.template find<cache_key_t<request_t>>(cache);
    }

    template <typename request_t, typename dependency_chain_t, typename cache_t, typename cache_traits_t,
              typename provider_t, typename request_traits_t, typename container_t>
    static auto resolve(cache_t& cache, cache_traits_t& cache_traits, provider_t& provider,
                        request_traits_t& request_traits, container_t& container) -> as_returnable_t<request_t> {
        // get_or_create returns the right thing based on cache_key_t:
        // - if cache_key_t is shared_ptr<T>, returns shared_ptr<T>
        // - if cache_key_t is T, returns T&
        return request_traits.template as_requested<request_t>(
            cache_traits.template get_or_create<cache_key_t<request_t>, typename provider_t::provided_t>(
                cache, [&]() { return provider.template create<request_t, dependency_chain_t>(container); }));
    }
};

// --------------------------------------------------------------------------------------------------------------------
// always_create: never check cache, always create fresh
// --------------------------------------------------------------------------------------------------------------------

template <>
struct resolution_strategy<resolution_strategy_t::always_create> {
    template <typename request_t, typename cache_t, typename cache_traits_t>
    static auto check_cache(cache_t&, cache_traits_t&) -> std::nullptr_t {
        return nullptr;  // never checks cache
    }

    template <typename request_t, typename dependency_chain_t, typename cache_t, typename cache_traits_t,
              typename provider_t, typename request_traits_t, typename container_t>
    static auto resolve(cache_t&, cache_traits_t&, provider_t& provider, request_traits_t& request_traits,
                        container_t& container) -> as_returnable_t<request_t> {
        return request_traits.template as_requested<request_t>(
            provider.template create<request_t, dependency_chain_t>(container));
    }
};

// --------------------------------------------------------------------------------------------------------------------
// copy_from_cache: check cache (creating/caching if needed), return copy/move of cached value
// --------------------------------------------------------------------------------------------------------------------

template <>
struct resolution_strategy<resolution_strategy_t::copy_from_cache> {
    template <typename request_t, typename cache_t, typename cache_traits_t>
    static auto check_cache(cache_t& cache, cache_traits_t& cache_traits) -> auto {
        return cache_traits.template find<cache_key_t<request_t>>(cache);
    }

    template <typename request_t, typename dependency_chain_t, typename cache_t, typename cache_traits_t,
              typename provider_t, typename request_traits_t, typename container_t>
    static auto resolve(cache_t& cache, cache_traits_t& cache_traits, provider_t& provider,
                        request_traits_t& request_traits, container_t& container) -> as_returnable_t<request_t> {
        // create and cache (get_or_create handles double-checked locking internally)
        // return copy from cached
        return request_traits.template as_requested<request_t>(
            element_type(cache_traits.template get_or_create<cache_key_t<request_t>, typename provider_t::provided_t>(
                cache, [&]() { return provider.template create<request_t, dependency_chain_t>(container); })));
    }
};

}  // namespace dink
