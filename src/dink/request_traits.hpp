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

enum class transitive_scope_t
{
    unmodified,
    transient,
    singleton
};

template <typename requested_t>
struct request_traits_f
{
    using value_type = requested_t;
    using transitive_scope_type = scope::default_t;

    template <typename cache_p>
    static auto find_in_cache(cache_p& cache) noexcept -> auto
    {
        return cache.template get_instance<value_type>();
    }

    template <typename provided_t, typename cache_p, typename factory_p>
    static auto resolve_from_cache(cache_p& cache, factory_p&& factory) -> decltype(auto)
    {
        return cache.template get_or_create_instance<provided_t>(std::forward<factory_p>(factory));
    }

    template <typename source_t>
    static auto as_requested(source_t&& source) -> requested_t
    {
        return element_type(std::forward<source_t>(source));
    }
};

template <typename requested_t>
struct request_traits_f<requested_t&&> : request_traits_f<requested_t>
{
    using transitive_scope_type = scope::transient_t;
};

template <typename requested_t>
struct request_traits_f<requested_t&>
{
    using value_type = requested_t;
    using transitive_scope_type = scope::singleton_t;

    template <typename cache_p>
    static auto find_in_cache(cache_p& cache) noexcept -> auto
    {
        return cache.template get_instance<value_type>();
    }

    template <typename provided_t, typename cache_p, typename factory_p>
    static auto resolve_from_cache(cache_p& cache, factory_p&& factory) -> decltype(auto)
    {
        return cache.template get_or_create_instance<provided_t>(std::forward<factory_p>(factory));
    }

    template <typename source_t>
    static auto as_requested(source_t&& source) -> requested_t&
    {
        return element_type(std::forward<source_t>(source));
    }
};

template <typename requested_t>
struct request_traits_f<requested_t*>
{
    using value_type = requested_t;
    using transitive_scope_type = scope::singleton_t;

    template <typename cache_p>
    static auto find_in_cache(cache_p& cache) noexcept -> auto
    {
        return cache.template get_instance<value_type>();
    }

    template <typename provided_t, typename cache_p, typename factory_p>
    static auto resolve_from_cache(cache_p& cache, factory_p&& factory) -> decltype(auto)
    {
        return &cache.template get_or_create_instance<provided_t>(std::forward<factory_p>(factory));
    }

    template <typename source_t>
    static auto as_requested(source_t&& source) -> requested_t*
    {
        // for requested_t*, source will never be a temporary
        return &element_type(std::forward<source_t>(source));
    }
};

template <typename requested_t, typename deleter_t>
struct request_traits_f<std::unique_ptr<requested_t, deleter_t>>
{
    using value_type = std::remove_cvref_t<requested_t>;
    using transitive_scope_type = scope::default_t;

    // unique_ptr is always transient, so it never interacts with the cache
    template <typename cache_p>
    static auto find_in_cache(cache_p& cache) noexcept -> auto
    {
        return cache.template get_instance<value_type>();
    }

    template <typename provided_t, typename cache_p, typename factory_p>
    static auto resolve_from_cache(cache_p& cache, factory_p&& factory)
    {
        return cache.template get_or_create_instance<provided_t>(std::forward<factory_p>(factory));
    }

    template <typename source_t>
    static auto as_requested(source_t&& source) -> std::unique_ptr<requested_t, deleter_t>
    {
        return std::make_unique<requested_t>(element_type(std::forward<source_t>(source)));
    }
};

template <typename requested_t>
struct request_traits_f<std::shared_ptr<requested_t>>
{
    using value_type = std::remove_cvref_t<requested_t>;
    using transitive_scope_type = scope::default_t;

    template <typename cache_p>
    static auto find_in_cache(cache_p& cache) noexcept -> auto
    {
        return cache.template get_shared<value_type>();
    }

    template <typename provided_t, typename cache_p, typename factory_p>
    static auto resolve_from_cache(cache_p& cache, factory_p&& factory)
    {
        return cache.template get_or_create_shared<provided_t>(std::forward<factory_p>(factory));
    }

    template <typename source_t>
    static auto as_requested(source_t&& source) -> std::shared_ptr<requested_t>
    {
        // incredibly simple: the source is either the correct shared_ptr or a raw type
        if constexpr (is_shared_ptr_v<std::remove_cvref_t<source_t>>)
        {
            return std::forward<source_t>(source); // from cache or transient provider
        }
        else
        {
            // from a raw transient provider
            return std::make_shared<requested_t>(std::forward<source_t>(source));
        }
    }
};

template <typename requested_t>
struct request_traits_f<std::weak_ptr<requested_t>> : request_traits_f<std::shared_ptr<requested_t>>
{
    using transitive_scope_type = scope::singleton_t;

    template <typename source_t>
    static auto as_requested(source_t&& source) -> std::weak_ptr<requested_t>
    {
        // delegate to shared_ptr logic and convert
        return request_traits_f<std::shared_ptr<requested_t>>::as_requested(std::forward<source_t>(source));
    }
};

template <typename requested_t>
struct request_traits_f<requested_t const> : request_traits_f<requested_t>
{};

template <typename requested_t>
struct request_traits_f<requested_t const&> : request_traits_f<requested_t&>
{};

template <typename requested_t>
struct request_traits_f<requested_t const*> : request_traits_f<requested_t*>
{};

//! Type actually cached and provided for a given request
template <typename requested_t>
using resolved_t = typename request_traits_f<requested_t>::value_type;

// \brief Metafunction to convert a type to a valid function return type.
// \details The primary template handles all types that are already valid.
template <typename type_p>
struct as_returnable_f
{
    using type = type_p;
};

// \brief Partial specialization to convert an rvalue reference to a value.
template <typename type_p>
struct as_returnable_f<type_p&&>
{
    using type = type_p;
};

// \brief Alias template for convenience.
template <typename type_p>
using as_returnable_t = typename as_returnable_f<type_p>::type;

/*!
    effective scope to use for a specific request given its immediate type and scope it was bound to

    If T is bound transient, but you ask for T&, T const&, T*, or weak_ptr<T> that request is treated as singleton.
    If T is bound singleton, but you ask for T, T&& or unique_ptr<T>, that request is treated as transient and receives
    a copy of the cached T.
*/
template <typename bound_scope_t, typename request_t>
using effective_scope_t = std::conditional_t<
    std::same_as<typename request_traits_f<request_t>::transitive_scope_type, scope::transient_t>, scope::transient_t,
    std::conditional_t<
        std::same_as<typename request_traits_f<request_t>::transitive_scope_type, scope::singleton_t>,
        scope::singleton_t, bound_scope_t>>;

/*!
    converts type from what is cached or provided to what was actually requested

    The mapping from provided or cached types to requested types is n:m. A provider always returns a simple value type,
    but that value may be cached, which has different forms. What the caches return depends on context. When caching or
    requesting shared_ptrs, the result is always a shared_ptr. When caching a value, the result is a reference. When
    requesting a value which may not yet be cached, the result is a pointer.

    That means there are 4 kinds of type that may be provided: value, reference, pointer, or shared_ptr.

    The request may be for a copy, a reference, a reference to const, an rvalue reference, a pointer, a unique_ptr,
    shared_ptr, or weak_ptr. This means there are 8 kinds of types that may be requested.

    as_requested() handles this n:m mapping by delegating to the request_traits of the request.
*/
template <typename request_t, typename instance_t>
auto as_requested(instance_t&& instance) -> decltype(auto)
{
    return request_traits_f<request_t>::as_requested(std::forward<instance_t>(instance));
}

} // namespace dink
