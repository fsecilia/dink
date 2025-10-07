/*!
    \file
    Copyright (C) 2025 Frank Secilia
*/

#pragma once

#include <dink/lib.hpp>
#include <dink/scopes.hpp>
#include <memory>
#include <type_traits>
#include <utility>

namespace dink {

enum class transitive_scope_t
{
    unmodified,
    transient,
    scoped
};

template <typename requested_t>
struct request_traits_f
{
    using value_type = requested_t;
    static constexpr transitive_scope_t transitive_scope = transitive_scope_t::unmodified;

    template <typename instance_t>
    static auto as_requested(instance_t&& instance) -> auto
    {
        return std::forward<instance_t>(instance);
    }
};

template <typename requested_t>
struct request_traits_f<requested_t&&>
{
    using value_type = requested_t;
    static constexpr transitive_scope_t transitive_scope = transitive_scope_t::transient;

    template <typename instance_t>
    static auto as_requested(instance_t&& instance) -> auto
    {
        return std::move(instance);
    }
};

template <typename requested_t>
struct request_traits_f<requested_t&>
{
    using value_type = requested_t;
    static constexpr transitive_scope_t transitive_scope = transitive_scope_t::scoped;

    template <typename instance_t>
    static auto as_requested(instance_t&& instance) -> requested_t&
    {
        return static_cast<requested_t&>(instance);
    }
};

template <typename requested_t>
struct request_traits_f<requested_t*>
{
    using value_type = requested_t;
    static constexpr transitive_scope_t transitive_scope = transitive_scope_t::scoped;

    template <typename instance_t>
    static auto as_requested(instance_t&& instance) -> requested_t*
    {
        return &instance;
    }
};

template <typename requested_t>
struct request_traits_f<std::unique_ptr<requested_t>>
{
    using value_type = requested_t;
    static constexpr transitive_scope_t transitive_scope = transitive_scope_t::transient;

    template <typename instance_t>
    static auto as_requested(instance_t&& instance) -> std::unique_ptr<requested_t>
    {
        return std::make_unique<requested_t>(std::forward<instance_t>(instance));
    }
};

template <typename requested_t>
struct request_traits_f<std::shared_ptr<requested_t>>
{
    using value_type = requested_t;
    static constexpr transitive_scope_t transitive_scope = transitive_scope_t::unmodified;

    template <typename instance_t>
    static auto as_requested(instance_t&& instance) -> std::shared_ptr<requested_t>
    {
        if constexpr (std::same_as<std::remove_cvref_t<instance_t>, std::shared_ptr<requested_t>>) { return instance; }
        else { return std::make_shared<requested_t>(std::forward<instance_t>(instance)); }
    }
};

template <typename requested_t>
struct request_traits_f<std::weak_ptr<requested_t>>
{
    using value_type = requested_t;
    static constexpr transitive_scope_t transitive_scope = transitive_scope_t::scoped;

    template <typename instance_t>
    static auto as_requested(instance_t&& instance) -> std::weak_ptr<requested_t>
    {
        return std::weak_ptr<requested_t>(instance);
    }
};

//! type actual cached and provided for a given request
template <typename requested_t>
using resolved_t = request_traits_f<requested_t>::value_type;

/*!
    effective scope to use for a specific request given its immediate type and scope it was bound to
    
    If type is bound transient, but you ask for something like type&, that request is treated as though it were bound
    scoped. If type is bound scoped or singleton, but you ask for somethign like type&&, that request is treated as
    though it were bound transient.
*/
template <typename bound_scope_t, typename request_t>
using effective_scope_t = std::conditional_t<
    request_traits_f<request_t>::transitive_scope == transitive_scope_t::transient, scopes::transient_t,
    std::conditional_t<
        request_traits_f<request_t>::transitive_scope == transitive_scope_t::scoped
            && std::same_as<bound_scope_t, scopes::transient_t>,
        scopes::scoped_t, bound_scope_t>>;

//! converts type from what is cached or provided to what was actually requested
template <typename request_t, typename instance_t>
auto as_requested(instance_t&& instance) -> auto
{
    return request_traits_f<request_t>::as_requested(std::forward<instance_t>(instance));
}

} // namespace dink
