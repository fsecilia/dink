/*!
    \file
    Copyright (C) 2025 Frank Secilia
*/

#pragma once

#include <dink/lib.hpp>
#include <dink/meta.hpp>
#include <dink/scope.hpp>
#include <dink/smart_pointer_traits.hpp>
#include <memory>
#include <type_traits>
#include <utility>

namespace dink {

// ---------------------------------------------------------------------------------------------------------------------
// forward declarations and aliases
// ---------------------------------------------------------------------------------------------------------------------

template <typename requested_t>
struct request_traits_f;

//! type actually cached and provided for a given request
template <typename requested_t>
using resolved_t = typename request_traits_f<requested_t>::value_type;

// ---------------------------------------------------------------------------------------------------------------------
// metafunctions
// ---------------------------------------------------------------------------------------------------------------------

//! resolves type to a valid function return type by removing rvalue references
template <typename type_t>
struct as_returnable_f;

//! general case is passthrough
template <typename type_t>
struct as_returnable_f
{
    using type = type_t;
};

//! rvalue reference specialization resolves to unqualified value type
template <typename type_t>
struct as_returnable_f<type_t&&>
{
    using type = type_t;
};

//! alias resolves type to valid function return type
template <typename type_t>
using as_returnable_t = typename as_returnable_f<type_t>::type;

// ---------------------------------------------------------------------------------------------------------------------
// effective scope
// ---------------------------------------------------------------------------------------------------------------------

/*!
    chooses effective scope for a request given its bound scope and request type

    Rules:
    - Transient request qualifiers (T, T&&, unique_ptr<T>) override bound scope -> always transient
    - Singleton request qualifiers (T&, T const&, T*, weak_ptr<T>) override bound scope -> always singleton
    - Otherwise, use the bound scope

    \tparam bound_scope_t scope type was bound with 
    \tparam request_t actual request type (with qualifiers)
*/
template <typename bound_scope_t, typename request_t>
using effective_scope_t = std::conditional_t<
    std::same_as<typename request_traits_f<request_t>::transitive_scope_type, scope::transient_t>, scope::transient_t,
    std::conditional_t<
        std::same_as<typename request_traits_f<request_t>::transitive_scope_type, scope::singleton_t>,
        scope::singleton_t, bound_scope_t>>;

// ---------------------------------------------------------------------------------------------------------------------
// request conversion
// ---------------------------------------------------------------------------------------------------------------------

/*!
    converts a provided/cached instance to the requested type

    Handles the n:m mapping between what providers and cache return (value, reference, pointer, shared_ptr)
    and what can be requested (T, T&, T const&, T&&, T*, unique_ptr<T>, shared_ptr<T>, weak_ptr<T>)
*/
template <typename request_t, typename instance_t>
auto as_requested(instance_t&& instance) -> decltype(auto)
{
    return request_traits_f<request_t>::as_requested(std::forward<instance_t>(instance));
}

// ---------------------------------------------------------------------------------------------------------------------
// request traits
// ---------------------------------------------------------------------------------------------------------------------

/*!
    request traits for value types (T)

    - Resolves to the type itself
    - Uses default scope behavior (respects bound scope)
    - Returns by value (copy or move)
*/
template <typename requested_t>
struct request_traits_f
{
    using value_type = requested_t;
    using transitive_scope_type = scope::default_t;

    template <typename cache_t>
    static auto find_in_cache(cache_t& cache) noexcept -> auto
    {
        return cache.template get_instance<value_type>();
    }

    template <typename provided_t, typename cache_t, typename factory_t>
    static auto resolve_from_cache(cache_t& cache, factory_t&& factory) -> decltype(auto)
    {
        return cache.template get_or_create_instance<provided_t>(std::forward<factory_t>(factory));
    }

    template <typename source_t>
    static auto as_requested(source_t&& source) -> requested_t
    {
        return element_type(std::forward<source_t>(source));
    }
};

/*!
    request traits for rvalue references (T&&)

    - Always transient (forces a new instance)
    - Returns by rvalue reference (enables move semantics)
*/
template <typename requested_t>
struct request_traits_f<requested_t&&> : request_traits_f<requested_t>
{
    using transitive_scope_type = scope::transient_t;
};

/*!
    request traits for lvalue references (T&)

    - Always singleton (requires stable address)
    - Returns by lvalue reference
*/
template <typename requested_t>
struct request_traits_f<requested_t&>
{
    using value_type = requested_t;
    using transitive_scope_type = scope::singleton_t;

    template <typename cache_t>
    static auto find_in_cache(cache_t& cache) noexcept -> auto
    {
        return cache.template get_instance<value_type>();
    }

    template <typename provided_t, typename cache_t, typename factory_t>
    static auto resolve_from_cache(cache_t& cache, factory_t&& factory) -> decltype(auto)
    {
        return cache.template get_or_create_instance<provided_t>(std::forward<factory_t>(factory));
    }

    template <typename source_t>
    static auto as_requested(source_t&& source) -> requested_t&
    {
        return element_type(std::forward<source_t>(source));
    }
};

/*!
    request traits for raw pointers (T*)

    - Always singleton (pointer implies shared lifetime)
    - Returns address of cached instance
*/
template <typename requested_t>
struct request_traits_f<requested_t*>
{
    using value_type = requested_t;
    using transitive_scope_type = scope::singleton_t;

    template <typename cache_t>
    static auto find_in_cache(cache_t& cache) noexcept -> auto
    {
        return cache.template get_instance<value_type>();
    }

    template <typename provided_t, typename cache_t, typename factory_t>
    static auto resolve_from_cache(cache_t& cache, factory_t&& factory) -> decltype(auto)
    {
        return &cache.template get_or_create_instance<provided_t>(std::forward<factory_t>(factory));
    }

    template <typename source_t>
    static auto as_requested(source_t&& source) -> requested_t*
    {
        return &element_type(std::forward<source_t>(source));
    }
};

/*!
    request traits for unique_ptr<T>

    - Always transient (unique ownership semantics)
    - Never cached (each request gets a new unique_ptr)
    - Wraps result in unique_ptr
*/
template <typename requested_t, typename deleter_t>
struct request_traits_f<std::unique_ptr<requested_t, deleter_t>>
{
    using value_type = std::remove_cvref_t<requested_t>;
    using transitive_scope_type = scope::default_t;

    template <typename cache_t>
    static auto find_in_cache(cache_t& cache) noexcept -> auto
    {
        return cache.template get_instance<value_type>();
    }

    template <typename provided_t, typename cache_t, typename factory_t>
    static auto resolve_from_cache(cache_t& cache, factory_t&& factory)
    {
        return cache.template get_or_create_instance<provided_t>(std::forward<factory_t>(factory));
    }

    template <typename source_t>
    static auto as_requested(source_t&& source) -> std::unique_ptr<requested_t, deleter_t>
    {
        return std::make_unique<requested_t>(element_type(std::forward<source_t>(source)));
    }
};

/*!
    request traits for shared_ptr<T>

    - Uses default scope behavior
    - Caches canonical shared_ptr instances
    - Returns shared ownership
*/
template <typename requested_t>
struct request_traits_f<std::shared_ptr<requested_t>>
{
    using value_type = std::remove_cvref_t<requested_t>;
    using transitive_scope_type = scope::default_t;

    template <typename cache_t>
    static auto find_in_cache(cache_t& cache) noexcept -> auto
    {
        return cache.template get_shared<value_type>();
    }

    template <typename provided_t, typename cache_t, typename factory_t>
    static auto resolve_from_cache(cache_t& cache, factory_t&& factory)
    {
        return cache.template get_or_create_shared<provided_t>(std::forward<factory_t>(factory));
    }

    template <typename source_t>
    static auto as_requested(source_t&& source) -> std::shared_ptr<requested_t>
    {
        if constexpr (is_shared_ptr_v<std::remove_cvref_t<source_t>>)
        {
            // Already a shared_ptr from cache
            return std::forward<source_t>(source);
        }
        else
        {
            // Raw value from transient provider
            return std::make_shared<requested_t>(std::forward<source_t>(source));
        }
    }
};

/*!
    request traits for weak_ptr<T>

    - Always singleton (weak_ptr requires cached shared_ptr)
    - Leverages shared_ptr caching
    - Returns weak reference to cached shared_ptr
*/
template <typename requested_t>
struct request_traits_f<std::weak_ptr<requested_t>> : request_traits_f<std::shared_ptr<requested_t>>
{
    using transitive_scope_type = scope::singleton_t;

    template <typename source_t>
    static auto as_requested(source_t&& source) -> std::weak_ptr<requested_t>
    {
        // Delegate to shared_ptr logic and convert to weak_ptr
        return request_traits_f<std::shared_ptr<requested_t>>::as_requested(std::forward<source_t>(source));
    }
};

//! request traits for const value types - delegates to non-const version
template <typename requested_t>
struct request_traits_f<requested_t const> : request_traits_f<requested_t>
{};

//! request traits for const lvalue references - delegates to non-const reference version
template <typename requested_t>
struct request_traits_f<requested_t const&> : request_traits_f<requested_t&>
{};

//! request traits for const pointers - delegates to non-const pointer version
template <typename requested_t>
struct request_traits_f<requested_t const*> : request_traits_f<requested_t*>
{};

} // namespace dink
