/*!
    \file
    Copyright (C) 2025 Frank Secilia
*/

#pragma once

#include <dink/lib.hpp>
#include <type_traits>

namespace dink {
namespace factories {

//! true if type has a static construct() method, regardless of signature.
template <typename type_t>
concept has_static_construct_method = requires {
    { &std::remove_cvref_t<type_t>::construct };
};

//! Invokes resolved_t's ctor directly.
template <typename resolved_t>
class direct_ctor_t
{
public:
    template <typename... args_t>
    requires(std::is_constructible_v<resolved_t, args_t...>)
    constexpr auto operator()(args_t&&... args) const -> resolved_t
    {
        return resolved_t{std::forward<args_t>(args)...};
    }
};

//! Invokes static member resolved_t::construct.
template <typename resolved_t>
class static_construct_method_t;

template <has_static_construct_method resolved_t>
class static_construct_method_t<resolved_t>
{
public:
    template <typename... args_t>
    constexpr auto operator()(args_t&&... args) const -> resolved_t
    {
        return resolved_t::construct(std::forward<args_t>(args)...);
    }
};

//! Invokes call operator on a factory supplied by the user.
template <typename resolved_t, typename resolved_factory_t>
class external_t
{
public:
    template <typename... args_t>
    requires(std::is_invocable_v<resolved_factory_t, resolved_factory_t&, args_t...>)
    constexpr auto operator()(args_t&&... args) const -> resolved_t
    {
        return resolved_factory_(std::forward<args_t>(args)...);
    }

    explicit constexpr external_t(resolved_factory_t resolved_factory) noexcept
        : resolved_factory_{std::move(resolved_factory)}
    {}

private:
    resolved_factory_t resolved_factory_{};
};

/*!
    Default factory if none is specified.

    Chooses static_construct_method_t if resolved_t has a static construct() method, direct_ctor_t otherwise.
*/
template <typename resolved_t>
struct default_t : public direct_ctor_t<resolved_t>
{};

template <has_static_construct_method resolved_t>
struct default_t<resolved_t> : public static_construct_method_t<resolved_t>
{};

} // namespace factories

/*!
    Customization point to instantiate resolved_t.

    The final instantiation of a resolved type happens in this type's call operator. By default, it first tries a
    static "construct" method in resolved_t, then falls back to direct ctor invocation if nothing was found. Users can
    specialize factory_t to override this behavior, or to use their own external factory.

    To make customization easier, namespace factories has base types for common factory patterns. Users can derive a
    specialized factory_t from these to choose a specific behavior.

    During resolution, the instance of this factory is resolved by the composer. The factory type must be either
    default-constructible or resolvable. If it is default-constructible, it is default-constructed rather than
    resolved.
*/
template <typename resolved_t>
struct factory_t : factories::default_t<resolved_t>
{};

} // namespace dink
