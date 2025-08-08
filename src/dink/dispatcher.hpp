/*!
    \file
    Copyright (C) 2025 Frank Secilia
*/

#pragma once

#include <dink/lib.hpp>
#include <dink/factory_resolvable.hpp>
#include <type_traits>

namespace dink {

/*!
    dispatches resolve() to the invocation of a factory that succeeds with the fewest arguments

    dispatcher_t tries to invoke the given factory with an increasing number of arguments, starting from 0. The first
    invocation that succeeds is chosen. This choice is made at compile time using type recursion.
*/
template <
    typename resolved_t, typename composer_t, typename factory_t, template <typename, typename, int_t> class arg_t,
    typename... args_t>
struct dispatcher_t;

//! successful dispatcher resolves using factory
template <
    typename resolved_t, typename composer_t, typename factory_t, template <typename, typename, int_t> class arg_t,
    typename... args_t>
requires(factory_resolvable<resolved_t, factory_t, args_t...>)
struct dispatcher_t<resolved_t, composer_t, factory_t, arg_t, args_t...>
{
    inline static constexpr auto const resolved = true;

    constexpr auto operator()(composer_t& composer) const -> resolved_t
    {
        auto const factory = resolve_factory(composer);
        return factory(args_t{composer}...);
    }

    constexpr auto resolve_factory(composer_t& composer) const -> factory_t
    {
        if constexpr (std::is_default_constructible_v<factory_t>) { return factory_t{}; }
        else { return composer.template resolve<factory_t>(); }
    }
};

//! unsuccessful dispatcher tries the next in the chain by appending a new arg_t
template <
    typename resolved_t, typename composer_t, typename factory_t, template <typename, typename, int_t> class arg_t,
    typename... args_t>
requires(!factory_resolvable<resolved_t, factory_t, args_t...> && sizeof...(args_t) <= dink_max_deduced_params)
struct dispatcher_t<resolved_t, composer_t, factory_t, arg_t, args_t...>
    : dispatcher_t<
          resolved_t, composer_t, factory_t, arg_t, args_t..., arg_t<resolved_t, composer_t, sizeof...(args_t) + 1>>
{};

//! exceeding dink_max_deduced_params fails
template <
    typename resolved_t, typename composer_t, typename factory_t, template <typename, typename, int_t> class arg_t,
    typename... args_t>
requires(sizeof...(args_t) > dink_max_deduced_params)
struct dispatcher_t<resolved_t, composer_t, factory_t, arg_t, args_t...>
{
    inline static constexpr auto const resolved = false;
};

} // namespace dink
