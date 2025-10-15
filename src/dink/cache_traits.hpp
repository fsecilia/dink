/*!
    \file
    Copyright (C) 2025 Frank Secilia
*/

#pragma once

#include <dink/lib.hpp>
#include <dink/not_found.hpp>
#include <dink/provider.hpp>
#include <dink/scope.hpp>
#include <dink/smart_pointer_traits.hpp>
#include <memory>
#include <type_traits>
#include <utility>

namespace dink {

template <typename binding_t>
struct bound_scope_f {
    using type = scope::default_t;
};

template <typename binding_t>
    requires(!std::same_as<binding_t, not_found_t>)
struct bound_scope_f<binding_t> {
    using type = typename std::remove_pointer_t<binding_t>::scope_type;
};

template <typename binding_t>
using bound_scope_t = typename bound_scope_f<binding_t>::type;

//! primitive resolve operations
enum class resolution_t {
    use_accessor,     // bound to accessor provider - bypass cache
    create,           // make new instance, don't cache
    cache,            // cache because bound singleton
    cache_promoted,   // cache because request forces it
    copy_from_cache,  // cache but return copy
    create_shared,    // new shared_ptr, don't cache
    cache_shared,     // cache shared_ptr
    defer_shared      // defer to shared_ptr logic
};

template <typename request_t, typename binding_t>
consteval auto select_resolution() noexcept -> resolution_t {
    // check for accessor provider first
    if constexpr (!std::same_as<binding_t, not_found_t>) {
        if constexpr (provider::is_accessor<typename std::remove_pointer_t<binding_t>::provider_type>) {
            return resolution_t::use_accessor;
        }
    }

    // decide based on scope + request form; if not bound, use default scope
    using bound_scope_t         = bound_scope_t<binding_t>;
    using unqualified_request_t = std::remove_cvref_t<request_t>;
    if constexpr (is_weak_ptr_v<unqualified_request_t>) {
        // weak_ptr always delegates
        return resolution_t::defer_shared;
    } else if constexpr (is_shared_ptr_v<unqualified_request_t>) {
        // shared_ptr operations
        if constexpr (std::same_as<bound_scope_t, scope::singleton_t>) {
            return resolution_t::cache_shared;
        } else {
            return resolution_t::create_shared;
        }
    } else if constexpr (is_unique_ptr_v<unqualified_request_t> || std::is_rvalue_reference_v<request_t>) {
        // unique_ptr and T&& - exclusive ownership
        if constexpr (std::same_as<bound_scope_t, scope::singleton_t>) {
            return resolution_t::copy_from_cache;
        } else {
            return resolution_t::create;
        }
    } else if constexpr (std::is_lvalue_reference_v<request_t> || std::is_pointer_v<unqualified_request_t>) {
        // T&, T* - stable address required
        if constexpr (std::same_as<bound_scope_t, scope::singleton_t>) {
            return resolution_t::cache;
        } else {
            return resolution_t::cache_promoted;
        }
    } else {
        // T - value, follows bound scope
        if constexpr (std::same_as<bound_scope_t, scope::singleton_t>) {
            return resolution_t::cache;
        } else {
            return resolution_t::create;
        }
    }
}

enum class implementation_t { use_accessor, create, cache };
consteval auto resolution_to_implementation(resolution_t operation) noexcept -> implementation_t {
    static constexpr implementation_t map[] = {
        implementation_t::use_accessor,  // use_accessor,
        implementation_t::create,        // create,
        implementation_t::cache,         // cache,
        implementation_t::cache,         // cache_promoted,
        implementation_t::cache,         // copy_from_cache,
        implementation_t::create,        // create_shared,
        implementation_t::cache,         // cache_shared,
        implementation_t::cache,         // defer_shared
    };
    return map[static_cast<int_t>(operation)];
}

// cache_traits - what form to store/retrieve from cache
template <typename request_t>
struct cache_traits_f {
    using value_type = std::remove_cvref_t<request_t>;
    using key_type   = resolved_t<request_t>;

    template <typename cache_t>
    static auto find(cache_t& cache) -> auto {
        return cache.template get_instance<value_type>();
    }

    template <typename provided_t, typename cache_t, typename factory_t>
    static auto get_or_create(cache_t& cache, factory_t&& factory) -> decltype(auto) {
        return cache.template get_or_create_instance<provided_t>(std::forward<factory_t>(factory));
    }
};

// specialization for shared_ptr/weak_ptr
template <typename T>
struct cache_traits_f<std::shared_ptr<T>> {
    using value_type = std::remove_cvref_t<T>;
    using key_type   = std::shared_ptr<T>;

    template <typename cache_t>
    static auto find(cache_t& cache) -> auto {
        return cache.template get_shared<value_type>();
    }

    template <typename provided_t, typename cache_t, typename factory_t>
    static auto get_or_create(cache_t& cache, factory_t&& factory) -> decltype(auto) {
        return cache.template get_or_create_shared<provided_t>(std::forward<factory_t>(factory));
    }
};

template <typename T>
struct cache_traits_f<std::weak_ptr<T>> : cache_traits_f<std::shared_ptr<T>> {};

//! instance-based api adapter over cache_traits_t's static api
struct cache_traits_t {
    template <typename request_t, typename cache_t>
    auto find(cache_t& cache) noexcept -> auto {
        return cache_traits_f<request_t>::find(cache);
    }

    template <typename request_t, typename provided_t, typename cache_t, typename factory_t>
    auto get_or_create(cache_t& cache, factory_t&& factory) -> decltype(auto) {
        return cache_traits_f<request_t>::template get_or_create<provided_t>(cache, std::forward<factory_t>(factory));
    }
};

template <typename request_t>
using cache_key_t = typename cache_traits_f<request_t>::key_type;

}  // namespace dink
