/*!
    \file
    Copyright (C) 2025 Frank Secilia
*/

#pragma once

#include <dink/lib.hpp>
#include <concepts>
#include <tuple>
#include <type_traits>
#include <utility>

namespace dink {

template <typename from_t, typename to_t>
struct target_transient_scope_t
{
    constexpr auto resolve(auto& container) const -> to_t { return container.template resolve<to_t>(); }
};

template <typename from_t, typename to_t>
struct target_singleton_scope_t
{
    constexpr auto resolve(auto& container) const -> to_t&
    {
        static auto instance = container.template resolve<to_t>();
        return instance;
    }
};

template <typename from_t, typename to_t>
using transient = target_transient_scope_t<from_t, to_t>;

template <typename from_t, typename to_t>
using singleton = target_singleton_scope_t<from_t, to_t>;

// ---------------------------------------------------------------------------------------------------------------------

template <typename from_t, typename to_t>
struct target_type_t
{
    template <template <typename, typename> class target_scope_t>
    constexpr auto in() -> target_scope_t<from_t, to_t>
    {
        return {};
    }
};

template <typename from_t, std::invocable to_t>
struct target_callable_t
{
    constexpr auto resolve(auto& container) const -> to_t
    {
        auto callable = container.template resolve<to_t>();
        return callable();
    }
};

template <typename from_t, typename to_t>
struct target_instance_t
{
    to_t to;
    constexpr auto resolve(auto&) const -> to_t& { return to; }
};

template <typename from_t, std::invocable to_t>
struct target_factory_t
{
    to_t to;
    constexpr auto resolve(auto&) const -> auto { return to(); }
};

template <typename from_t>
struct src_t
{
    template <typename to_t>
    auto to() const noexcept -> target_type_t<from_t, to_t>
    {
        return target_type_t<from_t, to_t>{};
    }

    template <std::invocable to_t>
    auto to() const noexcept -> target_callable_t<from_t, to_t>
    {
        return target_callable_t<from_t, to_t>{};
    }

    template <typename to_t>
    auto to(to_t&& target_instance) const noexcept -> target_instance_t<from_t, to_t>
    {
        return target_instance_t<from_t, to_t>{std::forward<to_t>(target_instance)};
    }

    template <std::invocable to_t>
    auto to(to_t&& target_factory) const noexcept -> target_factory_t<from_t, to_t>
    {
        return target_factory_t<from_t, to_t>{std::forward<to_t>(target_factory)};
    }
};

} // namespace dink
