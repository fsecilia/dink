/*!
    \file
    Copyright (C) 2025 Frank Secilia
*/

#pragma once

#include <dink/lib.hpp>
#include <dink/lifestyle.hpp>
#include <dink/meta.hpp>
#include <memory>
#include <type_traits>
#include <utility>

namespace dink {

namespace detail {

template <typename>
struct is_shared_ptr_f : std::false_type
{};

template <typename element_t>
struct is_shared_ptr_f<std::shared_ptr<element_t>> : std::true_type
{};

template <typename element_t>
constexpr bool is_shared_ptr_v = is_shared_ptr_f<std::remove_cvref_t<element_t>>::value;

template <typename>
struct is_weak_ptr_f : std::false_type
{};

template <typename element_t>
struct is_weak_ptr_f<std::weak_ptr<element_t>> : std::true_type
{};

template <typename element_t>
constexpr bool is_weak_ptr_v = is_weak_ptr_f<std::remove_cvref_t<element_t>>::value;

template <typename source_t>
constexpr auto get_underlying(source_t&& source) -> decltype(auto)
{
    if constexpr (std::is_pointer_v<std::remove_cvref_t<source_t>> || is_shared_ptr_v<source_t>) return *source;
    else return std::forward<source_t>(source);
}

} // namespace detail

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
        return std::move(detail::get_underlying(std::forward<source_t>(source)));
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
        return std::move(detail::get_underlying(std::forward<source_t>(source)));
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
        return detail::get_underlying(std::forward<source_t>(source));
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
        return &detail::get_underlying(std::forward<source_t>(source));
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
        if constexpr (std::is_pointer_v<clean_source_t> || detail::is_shared_ptr_v<clean_source_t>)
        {
            static_assert(
                meta::dependent_false_v<source_t>,
                "Cannot request unique_ptr for a cached singleton - ownership conflict"
            );
        }
        else { return std::unique_ptr<requested_t, deleter_t>(new clean_source_t{std::move(source)}, deleter_t{}); }
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

        if constexpr (detail::is_shared_ptr_v<clean_source_t>) { return std::forward<source_t>(source); }
        else
        {
            static_assert(
                meta::dependent_false_v<source_t>,
                "Cannot request unique_ptr for a cached singleton - ownership conflict"
            );
        }
#if 0
        if constexpr (detail::is_shared_ptr_v<clean_source_t>) { return std::forward<source_t>(source); }
        else if constexpr (std::is_pointer_v<clean_source_t>)
        {
            return std::shared_ptr<requested_t>(source, [](auto*) {});
        }
        else { return std::make_shared<requested_t>(std::forward<source_t>(source)); }
#endif
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

//! Converts type from what is cached or provided to what was actually requested
template <typename request_t, typename instance_t>
auto as_requested(instance_t&& instance) -> decltype(auto)
{
    return request_traits_f<request_t>::as_requested(std::forward<instance_t>(instance));
}

} // namespace dink
