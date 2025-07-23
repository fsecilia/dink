/*!
    \file
    Copyright (C) 2025 Frank Secilia
*/

#pragma once

#include <dink/lib.hpp>
#include <dink/type_map.hpp>
#include <type_traits>
#include <utility>

namespace dink {

template <typename requested_t> concept shared = std::is_lvalue_reference_v<requested_t>;
template <typename requested_t> concept transient = !shared<requested_t>;

/*!
    composes object graphs recursively

    Composer creates object graphs. It deduces the parameters necessary to construct a requested type, constructs them,
    then uses them to construct the requested type. Construction recurses through each parameter, eventually creating
    the entire dependency tree.

    Composer has one function template, resolve, that returns an instance of the requested type. If the requested type
    is an lvalue reference, the result is created once and shared, otherwise, new instances are created as requested.
    The construction method is configurable per type. The instance may be constructed directly, through a static
    factory method, or by an external factory supplied by the user. The number and types of arguments required to call
    the construction method is determined automatically. If the construction method is overloaded, the shortest
    overload is chosen. Each argument is itself resolved by the composer before being passed to the construction
    method, and this recurses until the graph necessary to create an instance is complete.
*/
template <typename transient_resolver_t, typename shared_resolver_t>
class composer_t
{
public:
    template <transient requested_t>
    constexpr auto resolve() -> mapped_type_t<requested_t>
    {
        return transient_resolver_.template resolve<mapped_type_t<requested_t>>(*this);
    }

    template <shared requested_t>
    constexpr auto resolve() -> mapped_type_t<requested_t>&
    {
        return shared_resolver_.template resolve<mapped_type_t<requested_t>>(*this);
    }

    template <transient resolved_t>
    constexpr auto bind(resolved_t resolved) -> void
    {
        return transient_resolver_.bind(std::move(resolved));
    }

    template <shared resolved_t>
    constexpr auto bind(resolved_t resolved) -> void
    {
        return shared_resolver_.bind(resolved);
    }

    template <typename resolved_t>
    constexpr auto bind(std::reference_wrapper<resolved_t> resolved) -> void
    {
        return shared_resolver_.bind(resolved.get());
    }

    template <transient resolved_t>
    constexpr auto unbind(resolved_t&&) -> void
    {
        return transient_resolver_.template unbind<resolved_t>();
    }

    template <shared resolved_t>
    constexpr auto unbind(resolved_t) -> void
    {
        return shared_resolver_.template unbind<resolved_t>();
    }

    template <typename resolved_t>
    constexpr auto unbind(std::reference_wrapper<resolved_t>) -> void
    {
        return shared_resolver_.template unbind<typename std::reference_wrapper<resolved_t>::type>();
    }

private:
    [[no_unique_address]] transient_resolver_t transient_resolver_{};
    [[no_unique_address]] shared_resolver_t shared_resolver_{};
};

} // namespace dink
