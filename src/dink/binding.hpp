/*!
    \file
    Copyright (C) 2025 Frank Secilia
*/

#pragma once

#include <dink/lib.hpp>
#include <memory>
#include <type_traits>
#include <utility>

namespace dink {
namespace binding {

//! providers define how an object is created or acquired
namespace providers {

//! invokes constructor of to_t
template <typename to_t>
struct type_t
{};

//! invokes a user-provided factory
template <typename factory_p>
struct factory_t
{
    factory_p factory;
};

//! holds an external pointer to a user-provided instance; returns reference
template <typename instance_t>
struct external_instance_t
{
    instance_t* instance;
};

//! holds an internal copy of a user-provided instance; returns reference
template <typename instance_t>
struct internal_instance_t
{
    instance_t instance;
};

//! holds an external pointer to a prototype instance; returns copies
template <typename prototype_t>
struct external_prototype_t
{
    prototype_t const* prototype;
};

//! holds an internal copy of a prototype instance; returns copies
template <typename prototype_t>
struct internal_prototype_t
{
    prototype_t prototype;
};

} // namespace providers

// ---------------------------------------------------------------------------------------------------------------------

//! scopes tag the lifetime of a resolved instance
namespace scopes {

//! a new instance is created for every request
struct transient_t
{};

/*!
    one instance is created per container and cached

    \note The name "scoped scope" is silly, but it is standardized in the literature.
*/
struct scoped_t
{};

//! a single instance is created for the lifetime of the application
struct singleton_t
{};

} // namespace scopes

// ---------------------------------------------------------------------------------------------------------------------

// constraints validate provider and scope combinations
namespace constraints {

//! by default, providers are compatible with all scopes
template <typename provider_t, typename scope_t>
struct valid_scope_f : std::true_type
{};

//! instance providers must always be singletons
template <typename interface_t, typename scope_t>
struct valid_scope_f<providers::external_instance_t<interface_t>, scope_t> : std::is_same<scope_t, scopes::singleton_t>
{};

//! instance providers must always be singletons
template <typename interface_t, typename scope_t>
struct valid_scope_f<providers::internal_instance_t<interface_t>, scope_t> : std::is_same<scope_t, scopes::singleton_t>
{};

//! prototype providers must always be transient
template <typename interface_t, typename scope_t>
struct valid_scope_f<providers::external_prototype_t<interface_t>, scope_t> : std::is_same<scope_t, scopes::transient_t>
{};

//! prototype providers must always be transient
template <typename interface_t, typename scope_t>
struct valid_scope_f<providers::internal_prototype_t<interface_t>, scope_t> : std::is_same<scope_t, scopes::transient_t>
{};

//! true when given combination is valid
template <typename provider_t, typename scope_t>
inline constexpr auto valid_scope_v = valid_scope_f<provider_t, scope_t>::value;

//! constrains valid combinations of provider and scope
template <typename provider_t, typename scope_t> concept valid_scope = valid_scope_v<provider_t, scope_t>;

} // namespace constraints

// ---------------------------------------------------------------------------------------------------------------------

//! conditionally defines storage for given scope
template <typename scope_t, typename interface_t>
struct binding_storage_t;

//! by default, scopes need no storage.
template <typename scope_t, typename interface_t>
struct binding_storage_t
{};

//! scopes::scoped_t caches its instance in the scope itself
template <typename interface_t>
struct binding_storage_t<scopes::scoped_t, interface_t>
{
    mutable std::shared_ptr<interface_t> cache_;
};

// ---------------------------------------------------------------------------------------------------------------------

//! binds a type to a provider in a scope
template <typename interface_p, typename provider_p, typename scope_p>
struct binding_t : binding_storage_t<scope_p, interface_p>
{
    using interface_t = interface_p;
    using provider_t = provider_p;
    using scope_t = scope_p;

    provider_t provider;

    /*!
        rebinds to given scope

        The initial binding is defaulted based on provider. in_scope() rebinds to `new_scope_t`.
    */
    template <typename new_scope_t>
    requires(constraints::valid_scope<provider_t, new_scope_t>)
    constexpr auto in_scope() const -> binding_t<interface_t, provider_t, new_scope_t>
    {
        return {binding_storage_t<scope_t, interface_t>{}, std::move(provider)};
    }
};

//! fluent binding api entry point
template <typename interface_t>
struct binder_t
{
    //! binds to a implementation type with default transient scope
    template <typename implementation_t>
    constexpr auto to() const -> binding_t<interface_t, providers::type_t<implementation_t>, scopes::transient_t>
    {
        static_assert(
            std::is_convertible_v<implementation_t, interface_t>,
            "implementation type must be convertible to interface type."
        );

        return {};
    }

    //! binds to a factory function with a default transient scope
    template <typename factory_t>
    constexpr auto to_factory(factory_t&& factory) const
        -> binding_t<interface_t, providers::factory_t<factory_t>, scopes::transient_t>
    {
        return {.provider = {.factory = std::forward<factory_t>(factory)}};
    }

    //! binds to an external, mutable instance by reference
    template <typename implementation_t>
    requires(!std::is_const_v<implementation_t>)
    constexpr auto to_external_instance(implementation_t& instance) const
        -> binding_t<interface_t, providers::external_instance_t<interface_t>, scopes::singleton_t>
    {
        static_assert(
            std::is_convertible_v<implementation_t, interface_t>,
            "implementation type must be convertible to interface type."
        );

        return {.provider = {.instance = &instance}};
    }

    //! binds to an external, const instance by reference
    template <typename implementation_t>
    constexpr auto to_external_instance(implementation_t const& instance) const
        -> binding_t<interface_t const, providers::external_instance_t<interface_t const>, scopes::singleton_t>
    {
        static_assert(
            std::is_convertible_v<implementation_t, interface_t>,
            "implementation type must be convertible to interface type."
        );

        return {.provider = {.instance = &instance}};
    }

    //! binds by moving or copying from instance
    template <typename implementation_t>
    constexpr auto to_internal_instance(implementation_t instance) const
        -> binding_t<interface_t, providers::internal_instance_t<interface_t>, scopes::singleton_t>
    {
        static_assert(
            std::is_convertible_v<implementation_t, interface_t>,
            "implementation type must be convertible to interface type."
        );

        return {.provider = {.instance = std::move(instance)}};
    }

    //! binds to an external prototype by reference
    template <typename implementation_t>
    constexpr auto to_external_prototype(implementation_t const& prototype) const
        -> binding_t<interface_t, providers::external_prototype_t<implementation_t>, scopes::transient_t>
    {
        static_assert(
            std::is_convertible_v<implementation_t, interface_t>,
            "implementation type must be convertible to interface type."
        );

        return {.provider = {.prototype = &prototype}};
    }

    //! binds by moving or copying from prototype
    template <typename implementation_t>
    constexpr auto to_internal_prototype(implementation_t prototype) const
        -> binding_t<interface_t, providers::internal_prototype_t<implementation_t>, scopes::transient_t>
    {
        static_assert(
            std::is_convertible_v<implementation_t, interface_t>,
            "implementation type must be convertible to interface type."
        );

        return {.provider = {.prototype = std::move(prototype)}};
    }
};

} // namespace binding

//! creates a fluent binding
template <typename interface_t>
[[nodiscard]] constexpr auto bind() -> binding::binder_t<interface_t>
{
    return {};
}

} // namespace dink
