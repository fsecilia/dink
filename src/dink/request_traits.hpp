/*!
    \file
    Copyright (C) 2025 Frank Secilia
*/

#pragma once

#include <dink/lib.hpp>
#include <dink/lifestyle.hpp>
#include <dink/meta.hpp>
#include <dink/smart_pointer_traits.hpp>
#include <memory>
#include <type_traits>
#include <utility>

namespace dink {

enum class transitive_lifestyle_t
{
    unmodified,
    transient,
    singleton
};

template <typename requested_t>
struct request_traits_f
{
    using value_type = requested_t;
    using return_type = requested_t;
    static constexpr transitive_lifestyle_t transitive_lifestyle = transitive_lifestyle_t::unmodified;

    template <typename source_t>
    static auto as_requested(source_t&& source) -> requested_t
    {
        return std::move(element_type(std::forward<source_t>(source)));
    }
};

template <typename requested_t>
struct request_traits_f<requested_t&&>
{
    using value_type = requested_t;
    using return_type = requested_t;
    static constexpr transitive_lifestyle_t transitive_lifestyle = transitive_lifestyle_t::transient;

    template <typename source_t>
    static auto as_requested(source_t&& source) -> requested_t
    {
        return std::move(element_type(std::forward<source_t>(source)));
    }
};

template <typename requested_t>
struct request_traits_f<requested_t&>
{
    using value_type = requested_t;
    using return_type = requested_t&;
    static constexpr transitive_lifestyle_t transitive_lifestyle = transitive_lifestyle_t::singleton;

    template <typename source_t>
    static auto as_requested(source_t&& source) -> requested_t&
    {
        return static_cast<requested_t&>(element_type(std::forward<source_t>(source)));
    }
};

template <typename requested_t>
struct request_traits_f<requested_t*>
{
    using value_type = requested_t;
    using return_type = requested_t*;
    static constexpr transitive_lifestyle_t transitive_lifestyle = transitive_lifestyle_t::singleton;

    template <typename source_t>
    static auto as_requested(source_t&& source) -> requested_t*
    {
        return &element_type(std::forward<source_t>(source));
    }
};

template <typename requested_t, typename deleter_t>
struct request_traits_f<std::unique_ptr<requested_t, deleter_t>>
{
    using value_type = std::remove_cvref_t<requested_t>;
    using return_type = std::unique_ptr<requested_t, deleter_t>;
    static constexpr transitive_lifestyle_t transitive_lifestyle = transitive_lifestyle_t::transient;

    template <typename source_t>
    static auto as_requested(source_t&& source) -> std::unique_ptr<requested_t, deleter_t>
    {
        using clean_source_t = std::remove_cvref_t<source_t>;
        static_assert(!std::is_pointer_v<clean_source_t> || is_shared_ptr_v<clean_source_t>);
        return std::unique_ptr<requested_t, deleter_t>(new clean_source_t{std::move(source)}, deleter_t{});
    }
};

template <typename requested_t>
struct request_traits_f<std::shared_ptr<requested_t>>
{
    using value_type = std::remove_cvref_t<requested_t>;
    using return_type = std::shared_ptr<requested_t>;
    static constexpr transitive_lifestyle_t transitive_lifestyle = transitive_lifestyle_t::unmodified;

    template <typename source_t>
    static auto as_requested(source_t&& source) -> std::shared_ptr<requested_t>
    {
        using clean_source_t = std::remove_cvref_t<source_t>;

        // Case 1: Source is already a shared_ptr (e.g., from nested cache or transient).
        if constexpr (is_shared_ptr_v<clean_source_t>) { return std::forward<source_t>(source); }
        else if constexpr (std::is_pointer_v<clean_source_t> && is_shared_ptr_v<std::remove_pointer_t<clean_source_t>>)
        {
            // Case 2: Source is a pointer to a shared_ptr (from global cache).
            // The global cache returns a pointer to the canonical shared_ptr. Dereference it.
            return *source;
        }
        else if constexpr (std::is_pointer_v<clean_source_t>)
        {
            // Case 3: Source is a pointer to the underlying value T (from a singleton accessor).
            // The source is a non-owning pointer to a singleton. Create a non-owning shared_ptr.
            return std::shared_ptr<requested_t>(source, [](auto*) {});
        }
        else
        {
            // Case 4: Source is the raw value T (from a transient provider).
            // The source is a temporary value. Create a new owning shared_ptr.
            return std::make_shared<requested_t>(std::forward<source_t>(source));
        }
    }
};

template <typename requested_t>
struct request_traits_f<std::weak_ptr<requested_t>>
{
    using value_type = std::remove_cvref_t<requested_t>;
    using return_type = std::weak_ptr<requested_t>;
    static constexpr transitive_lifestyle_t transitive_lifestyle = transitive_lifestyle_t::singleton;

    template <typename source_t>
    static auto as_requested(source_t&& source) -> std::weak_ptr<requested_t>
    {
        // delegate to shared_ptr path, then convert to weak_ptr
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

//! Type actually cached and provided for a given request
template <typename requested_t>
using returned_t = typename request_traits_f<requested_t>::return_type;

/*!
    Effective lifestyle to use for a specific request given its immediate type and lifestyle it was bound to
    
    If type is bound transient, but you ask for type&, that request is treated as singleton.
    If type is bound singleton, but you ask for type&&, that request is treated as transient.
*/
template <typename bound_lifestyle_t, typename request_t>
using effective_lifestyle_t = std::conditional_t<
    request_traits_f<request_t>::transitive_lifestyle == transitive_lifestyle_t::transient, lifestyle::transient_t,
    std::conditional_t<
        request_traits_f<request_t>::transitive_lifestyle == transitive_lifestyle_t::singleton, lifestyle::singleton_t,
        bound_lifestyle_t>>;

/*!
    Converts type from what is cached or provided to what was actually requested.
    
    The mapping from provided or cached types is to requested types is n:m. A provider always returns a simple value, 
    but that value may be cached in a singleton, which returns a reference, or a shared_ptr. The request may be for
    a copy, a reference, an rvalue reference, a pointer, a unique_ptr, shared_ptr, or weak_ptr. as_requested() handles
    the n:m mapping by delegating to the request_traits of the request.
*/
template <typename request_t, typename instance_t>
auto as_requested(instance_t&& instance) -> decltype(auto)
{
    return request_traits_f<request_t>::as_requested(std::forward<instance_t>(instance));
}

} // namespace dink
