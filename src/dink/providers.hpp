/*!
    \file
    Copyright (C) 2025 Frank Secilia
*/

#pragma once

#include <dink/lib.hpp>
#include <dink/scopes.hpp>

namespace dink::providers {

template <typename provider_t> concept has_scope = requires { typename provider_t::default_scope; };
template <typename provider_t> concept is_creator = has_scope<provider_t>;
template <typename provider_t> concept is_accessor = !has_scope<provider_t>;

struct ctor_invoker_t
{
    using default_scope_t = scopes::transient_t;

    template <typename instance_t, typename container_t>
    auto operator()(container_t& container) const -> instance_t; // not yet implemented yet
};

template <typename factory_t>
struct factory_invoker_t
{
    using default_scope_t = scopes::transient_t;

    template <typename container_t>
    auto operator()(container_t& container) -> auto
    {
        return factory(container);
    }

    factory_t factory;
};

template <typename instance_t>
struct internal_reference_t
{
    auto operator()() const noexcept -> instance_t& { return instance; }

    mutable instance_t instance;
};

template <typename instance_t>
struct external_reference_t
{
    auto operator()() const noexcept -> instance_t& { return *instance; }

    instance_t* instance;
};

template <typename instance_t>
struct internal_prototype_t
{
    auto operator()() const -> instance_t { return instance; }

    instance_t instance;
};

template <typename instance_t>
struct external_prototype_t
{
    auto operator()() const -> instance_t { return *prototype; }

    instance_t const* prototype;
};

} // namespace dink::providers
