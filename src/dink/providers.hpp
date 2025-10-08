/*!
    \file
    Copyright (C) 2025 Frank Secilia
*/

#pragma once

#include <dink/lib.hpp>
#include <dink/arity.hpp>
#include <dink/ctor_factory.hpp>
#include <dink/factory_invoker.hpp>
#include <dink/scopes.hpp>

namespace dink::providers {

template <typename provider_t> concept is_creator = requires { typename provider_t::creator_tag; };
template <typename provider_t> concept is_accessor = !is_creator<provider_t>;

//! unified creator provider - handles both ctors and user factories
template <typename constructed_t, typename factory_t = ctor_factory_t<constructed_t>>
struct creator_t
{
    struct creator_tag;
    using default_scope = scopes::transient_t;

    factory_t factory;

    static constexpr auto arity = arity_v<constructed_t, factory_t>;

    template <typename dependency_chain_t, typename container_t>
    auto create(container_t& container) -> constructed_t
    {
        using arg_t = arg_t<container_t, dependency_chain_t>;
        using single_arg_t = single_arg_t<constructed_t, arg_t>;
        return create<dependency_chain_t>(container, factory_invoker_t<constructed_t, arity, arg_t, single_arg_t>{});
    }

    template <typename dependency_chain_t, typename container_t, typename factory_invoker_t>
    auto create(container_t& container, factory_invoker_t&& factory_invoker) -> constructed_t
    {
        return std::invoke(std::forward<factory_invoker_t>(factory_invoker), factory, container);
    }
};

//! references an instance owned by the container (moved/copied in)
template <typename instance_t>
struct internal_reference_t
{
    instance_t instance;

    auto get() -> instance_t& { return instance; }
    auto get() const -> instance_t const& { return instance; }
};

//! references an instance owned externally (pointer stored)
template <typename instance_t>
struct external_reference_t
{
    instance_t* instance;

    auto get() -> instance_t& { return *instance; }
    auto get() const -> instance_t const& { return *instance; }
};

//! copies from a prototype owned by the container
template <typename instance_t>
struct internal_prototype_t
{
    instance_t prototype;

    auto get() const -> instance_t { return prototype; }
};

//! copies from an externally-owned prototype
template <typename instance_t>
struct external_prototype_t
{
    instance_t const* prototype;

    auto get() const -> instance_t { return *prototype; }
};

} // namespace dink::providers
