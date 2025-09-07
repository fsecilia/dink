/*!
    \file
    Copyright (C) 2025 Frank Secilia
*/

#pragma once

#include <dink/lib.hpp>
#include <dink/factory_resolvable.hpp>
#include <type_traits>
#include <utility>

namespace dink {
namespace factories {

/*!
    true if type has a static construct() method, regardless of signature

    This concept is greedy. It will detect a static construct() from a base class if even if the leaf does define one.
    Preventing this case is... nontrivial. Member access is checked after template resolution. The solution will likely
    require a new version of c++ with reflection.
*/
template <typename type_t>
concept has_static_construct_method = requires {
    { &std::remove_cvref_t<type_t>::construct };
};

//! inverse of has_static_construct_method
template <typename type_t> concept missing_static_construct_method = !has_static_construct_method<type_t>;

//! invokes resolved_t's ctor directly
template <typename resolved_t>
class direct_ctor_t
{
public:
    template <typename... args_t>
    static constexpr auto resolvable = requires(args_t&&... args) {
        { resolved_t{std::forward<args_t>(args)...} } -> std::convertible_to<resolved_t>;
    };

    template <typename... args_t>
    requires(std::is_constructible_v<resolved_t, args_t...>)
    constexpr auto operator()(args_t&&... args) const -> resolved_t
    {
        return resolved_t{std::forward<args_t>(args)...};
    }
};

//! invokes static member resolved_t::construct
template <typename resolved_t>
class static_construct_method_t;

template <has_static_construct_method resolved_t>
class static_construct_method_t<resolved_t>
{
public:
    template <typename... args_t>
    static constexpr auto resolvable = requires(args_t&&... args) {
        { resolved_t::construct(std::forward<args_t>(args)...) } -> std::convertible_to<resolved_t>;
    };

    template <typename... args_t>
    constexpr auto operator()(args_t&&... args) const -> resolved_t
    {
        return resolved_t::construct(std::forward<args_t>(args)...);
    }
};

//! invokes call operator on a factory supplied by the user
template <typename resolved_t, typename resolved_factory_t>
class external_t
{
public:
    template <typename... args_t>
    static constexpr auto resolvable = requires(resolved_factory_t resolved_factory, args_t&&... args) {
        { resolved_factory(std::forward<args_t>(args)...) } -> std::convertible_to<resolved_t>;
    };

    template <typename... args_t>
    requires(std::is_invocable_v<resolved_factory_t const&, args_t...>)
    constexpr auto operator()(args_t&&... args) const -> resolved_t
    {
        return resolved_factory_(std::forward<args_t>(args)...);
    }

    explicit constexpr external_t(resolved_factory_t resolved_factory) noexcept
        : resolved_factory_{std::move(resolved_factory)}
    {}

    explicit constexpr external_t() noexcept = default;

private:
    resolved_factory_t resolved_factory_;
};

/*!
    default factory if none is specified

    Chooses static_construct_method_t if resolved_t has a static construct() method, direct_ctor_t otherwise.
*/
template <typename resolved_t>
struct default_t;

template <missing_static_construct_method resolved_t>
struct default_t<resolved_t> : public direct_ctor_t<resolved_t>
{};

template <has_static_construct_method resolved_t>
struct default_t<resolved_t> : public static_construct_method_t<resolved_t>
{};

} // namespace factories

/*!
    customization point to instantiate resolved_t

    The final instantiation of a resolved type happens in this type's call operator. By default, it first tries a static
    construct() method in resolved_t, then falls back to direct ctor invocation. Users can specialize factory_t to
    use their own external factory, or to override this behavior entirely.

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
