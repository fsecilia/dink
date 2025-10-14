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

//! primitive resolve operations
enum class operation_t
{
    use_accessor, // bound to accessor provider - bypass cache
    create_transient, // make new instance, don't cache
    get_or_create_singleton, // cache because bound singleton
    get_or_create_promoted_singleton, // cache because request forces it
    get_or_create_singleton_copy, // cache but return copy
    create_transient_shared, // new shared_ptr, don't cache
    get_or_create_singleton_shared, // cache shared_ptr
    delegate_to_shared // defer to shared_ptr logic
};

template <typename binding_t>
struct bound_scope_f
{
    using type = scope::default_t;
};

template <typename binding_t>
requires(!std::same_as<binding_t, not_found_t>)
struct bound_scope_f<binding_t>
{
    using type = typename std::remove_pointer_t<binding_t>::scope_type;
};

template <typename binding_t>
using bound_scope_t = typename bound_scope_f<binding_t>::type;

template <typename request_t, typename binding_t>
consteval auto select_operation() noexcept -> operation_t
{
    // check for accessor provider first
    if constexpr (!std::same_as<binding_t, not_found_t>)
    {
        if constexpr (provider::is_accessor<typename std::remove_pointer_t<binding_t>::provider_type>)
        {
            return operation_t::use_accessor;
        }
    }

    // decide based on scope + request form; if not bound, use default scope
    using bound_scope_t = bound_scope_t<binding_t>;

    using unqualified_request_t = std::remove_cvref_t<request_t>;

    if constexpr (is_weak_ptr_v<unqualified_request_t>)
    {
        // weak_ptr always delegates
        return operation_t::delegate_to_shared;
    }
    else if constexpr (is_shared_ptr_v<unqualified_request_t>)
    {
        // shared_ptr operations
        if constexpr (std::same_as<bound_scope_t, scope::singleton_t>)
        {
            return operation_t::get_or_create_singleton_shared;
        }
        else { return operation_t::create_transient_shared; }
    }
    else if constexpr (is_unique_ptr_v<unqualified_request_t> || std::is_rvalue_reference_v<request_t>)
    {
        // unique_ptr and T&& - exclusive ownership
        if constexpr (std::same_as<bound_scope_t, scope::singleton_t>)
        {
            return operation_t::get_or_create_singleton_copy;
        }
        else { return operation_t::create_transient; }
    }
    else if constexpr (std::is_lvalue_reference_v<request_t> || std::is_pointer_v<unqualified_request_t>)
    {
        // T&, T* - stable address required
        if constexpr (std::same_as<bound_scope_t, scope::singleton_t>) { return operation_t::get_or_create_singleton; }
        else { return operation_t::get_or_create_promoted_singleton; }
    }
    else
    {
        // T - value, follows bound scope
        if constexpr (std::same_as<bound_scope_t, scope::singleton_t>) { return operation_t::get_or_create_singleton; }
        else { return operation_t::create_transient; }
    }
}

// cache_traits - what form to store/retrieve from cache
template <typename request_t>
struct cache_traits_f
{
    using value_type = std::remove_cvref_t<request_t>;

    template <typename cache_t>
    static auto find_in_cache(cache_t& cache) -> auto
    {
        return cache.template get_instance<value_type>();
    }

    template <typename provided_t, typename cache_t, typename factory_t>
    static auto resolve_from_cache(cache_t& cache, factory_t&& factory) -> decltype(auto)
    {
        return cache.template get_or_create_instance<provided_t>(std::forward<factory_t>(factory));
    }
};

// specialization for shared_ptr/weak_ptr
template <typename T>
struct cache_traits_f<std::shared_ptr<T>>
{
    using value_type = std::remove_cvref_t<T>;

    template <typename cache_t>
    static auto find_in_cache(cache_t& cache) -> auto
    {
        return cache.template get_shared<value_type>();
    }

    template <typename provided_t, typename cache_t, typename factory_t>
    static auto resolve_from_cache(cache_t& cache, factory_t&& factory) -> decltype(auto)
    {
        return cache.template get_or_create_shared<provided_t>(std::forward<factory_t>(factory));
    }
};

template <typename T>
struct cache_traits_f<std::weak_ptr<T>> : cache_traits_f<std::shared_ptr<T>>
{};

//! instance-based api adapter over cache_traits_t's static api
struct cache_traits_t
{
    template <typename request_t, typename cache_t>
    auto find_in_cache(cache_t& cache) noexcept -> auto
    {
        return cache_traits_f<request_t>::find_in_cache(cache);
    }

    template <typename request_t, typename provided_t, typename cache_t, typename factory_t>
    auto resolve_from_cache(cache_t& cache, factory_t&& factory) -> decltype(auto)
    {
        return cache_traits_f<request_t>::template resolve_from_cache<provided_t>(
            cache, std::forward<factory_t>(factory)
        );
    }
};

} // namespace dink
