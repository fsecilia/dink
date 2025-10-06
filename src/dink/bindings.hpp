/*!
    \file
    Copyright (C) 2025 Frank Secilia
*/

#pragma once

#include <dink/lib.hpp>
#include <concepts>
#include <type_traits>
#include <utility>

namespace dink {

namespace scopes {

struct transient_t
{};

struct scoped_t
{};

struct singleton_t
{};

struct default_t
{};

} // namespace scopes

namespace providers {

template <typename provider_t> concept has_scope = requires { typename provider_t::default_scope; };
template <typename provider_t> concept is_creator = has_scope<provider_t>;
template <typename provider_t> concept is_accessor = !has_scope<provider_t>;

struct ctor_invoker_t
{
    using default_scope_t = scopes::transient_t;

    template <typename instance_t, typename container_t>
    auto operator()(container_t& container) const -> instance_t; // not yet implemented yet
};

template <typename factory_p>
struct factory_invoker_t
{
    using default_scope_t = scopes::transient_t;

    template <typename container_t>
    auto operator()(container_t& container) -> auto
    {
        return factory(container);
    }

    factory_p factory;
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

} // namespace providers

namespace binding {

struct no_scope_t;

} // namespace binding

template <typename from_p, typename to_p, typename provider_p, typename scope_t>
struct binding_t
{
    using from_t = from_p;
    using to_type = to_p;
    using provider_t = provider_p;
    using resolved_scope_t = std::conditional_t<
        std::same_as<scope_t, scopes::default_t>,
        std::conditional_t<providers::is_accessor<provider_p>, binding::no_scope_t, typename provider_p::default_scope>,
        scope_t>;

    provider_t provider;
};

template <typename from_p, typename to_p, typename provider_p>
class binding_dst_t
{
public:
    using from_t = from_p;
    using to_t = to_p;
    using provider_t = provider_p;

    template <typename scope_t>
    requires providers::is_creator<provider_t>
    auto in() && -> binding_t<from_t, to_t, provider_t, scope_t>
    {
        return {std::move(provider_)};
    }

    auto provider() -> provider_t& { return provider_; }

    explicit binding_dst_t(provider_t provider) : provider_(std::move(provider)) {}

private:
    provider_t provider_;
};

template <typename from_t>
class binding_src_t
{
public:
    template <typename to_t>
    auto to() const -> binding_dst_t<from_t, to_t, providers::ctor_invoker_t>
    {
        return {providers::ctor_invoker_t{}};
    }

    template <typename to_t, typename factory_t>
    auto to_factory(factory_t&& factory) const -> binding_dst_t<from_t, to_t, providers::factory_invoker_t<factory_t>>
    {
        return {providers::factory_invoker_t{std::forward<factory_t>(factory)}};
    }

    template <typename to_t>
    auto to_internal_reference(to_t&& instance) const
        -> binding_dst_t<from_t, to_t, providers::internal_reference_t<to_t>>
    {
        return {providers::internal_reference_t{std::forward<to_t>(instance)}};
    }

    template <typename to_t>
    auto to_external_reference(to_t& instance) const
        -> binding_dst_t<from_t, to_t, providers::external_reference_t<to_t>>
    {
        return {providers::external_reference_t{&instance}};
    }

    template <typename to_t>
    auto to_internal_prototype(to_t&& instance) const
        -> binding_dst_t<from_t, to_t, providers::internal_prototype_t<to_t>>
    {
        return {providers::internal_prototype_t{std::forward<to_t>(instance)}};
    }

    template <typename to_t>
    auto to_external_prototype(to_t const& instance) const
        -> binding_dst_t<from_t, to_t, providers::external_prototype_t<to_t>>
    {
        return {providers::external_prototype_t{&instance}};
    }
};

template <typename from_t>
auto bind() -> binding_src_t<from_t>
{
    return {};
}

template <typename binding_t>
constexpr auto is_binding_builder_v = requires {
    typename binding_t::from_type;
    typename binding_t::to_type;
    typename binding_t::provider_type;
} && !requires { typename binding_t::resolved_scope; };

} // namespace dink
