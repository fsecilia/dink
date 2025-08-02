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
public:
    constexpr auto operator()(composer_t& composer) const -> resolved_t
    {
        auto const factory = resolve_factory(composer);
        return factory(args_t{composer}...);
    }

private:
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
requires(!factory_resolvable<resolved_t, factory_t, args_t...> && sizeof...(args_t) < dink_max_deduced_params)
struct dispatcher_t<resolved_t, composer_t, factory_t, arg_t, args_t...>
    : dispatcher_t<
          resolved_t, composer_t, factory_t, arg_t, args_t..., arg_t<resolved_t, composer_t, sizeof...(args_t) + 1>>
{};

//! running off the end produces an error
template <
    typename resolved_t, typename composer_t, typename factory_t, template <typename, typename, int_t> class arg_t,
    typename... args_t>
requires(sizeof...(args_t) == dink_max_deduced_params)
struct dispatcher_t<resolved_t, composer_t, factory_t, arg_t, args_t...>
{
    /*
        If you've hit this, dispatcher_t could not match a signature when trying to invoke the factory for resolved_t.
        Some of the parameters may be ambiguous to arg_t, or the number of params may be larger than
        dink_max_deduced_params. Follow factory_resolvable into the factory to find out why.

        One notable cause is when a leaf type does not define a static construct(), but at least one of its bases do.
        has_static_construct_method cannot determine this case independently of defining it in the leaf, so it still
        tries matching against static construct, which isn't available.

        If this is the case, the current workaround is to define a construct method in the leaf and forward to its
        ctor. This can become unweildy for deep hierarchies, but it works well enough in practice.

        /sa has_static_construct_method 
    */
    static_assert(factory_resolvable<resolved_t, factory_t, args_t...>);
};

} // namespace dink
