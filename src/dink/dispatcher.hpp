/*!
    \file
    Copyright (C) 2025 Frank Secilia
*/

#pragma once

#include <dink/lib.hpp>
#include <type_traits>

namespace dink {

/*!
    max number of params dispatcher will try to deduce before erroring out

    This value is mostly arbitrary, just higher than the number of parameters likely in generated code.
*/
constexpr auto const max_deduced_params = 3;

//! true when invoking factory with args produces a result convertible to resolved_t
template <typename resolved_t, typename factory_t, typename... args_t>
concept resolvable = requires(factory_t factory, args_t... args) {
    { factory(args...) } -> std::convertible_to<resolved_t>;
};

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
requires(resolvable<resolved_t, factory_t, args_t...>)
struct dispatcher_t<resolved_t, composer_t, factory_t, arg_t, args_t...>
{
public:
    constexpr auto operator()(composer_t& composer) const -> resolved_t
    {
        auto factory = resolve_factory(composer);
        return factory(args_t{composer}...);
    }

private:
    constexpr auto resolve_factory(composer_t& composer) const -> factory_t
    {
        if constexpr (std::is_default_constructible_v<factory_t>) { return factory_t{}; }
        else { return composer.template resolve<factory_t>(); }
    }
};

//! unsuccessful dispatcher tries the next in the chain
template <
    typename resolved_t, typename composer_t, typename factory_t, template <typename, typename, int_t> class arg_t,
    typename... args_t>
requires(!resolvable<resolved_t, factory_t, args_t...> && sizeof...(args_t) <= max_deduced_params)
struct dispatcher_t<resolved_t, composer_t, factory_t, arg_t, args_t...>
    : dispatcher_t<
          resolved_t, composer_t, factory_t, arg_t, args_t..., arg_t<resolved_t, composer_t, sizeof...(args_t) + 1>>
{};

} // namespace dink
