/*!
    \file
    Copyright (C) 2025 Frank Secilia
*/

#pragma once

#include <dink/lib.hpp>
#include <dink/canonical.hpp>
#include <dink/lifecycle.hpp>
#include <dink/providers.hpp>
#include <concepts>
#include <type_traits>
#include <utility>

namespace dink {

//! complete binding configuration: from type -> to type -> provider -> scope
template <typename from_t, typename to_t, typename provider_t, typename scope_t>
struct binding_t
{
    using from_type = from_t;
    using to_type = to_t;
    using provider_type = provider_t;
    using scope_type = scope_t;

    provider_t provider;
};

//! binding builder after destination type is specified
template <typename from_t, typename to_t, typename provider_t>
class binding_dst_t
{
public:
    using from_type = from_t;
    using to_type = to_t;
    using provider_type = provider_t;

    // specify scope (only for creators)
    template <typename scope_t>
    requires providers::is_creator<provider_t>
    auto in() && -> binding_t<from_t, to_t, provider_t, scope_t>
    {
        return {std::move(provider_)};
    }

    // if no scope specified, use default (or accessor has no scope)
    operator binding_t<from_t, to_t, provider_t, scopes::default_t>() && { return {std::move(provider_)}; }

    explicit binding_dst_t(provider_t provider) : provider_(std::move(provider)) {}

private:
    provider_t provider_;
};

//! binding builder after source type is specified
template <typename from_t>
class binding_src_t
{
public:
    // bind to type using ctor
    template <typename to_t>
    auto to() const -> binding_dst_t<from_t, to_t, providers::creator_t<to_t>>
    {
        return binding_dst_t<from_t, to_t, providers::creator_t<to_t>>{providers::creator_t<to_t>{}};
    }

    // bind to type using factory
    template <typename to_t, typename factory_t>
    auto to_factory(factory_t&& factory) const
        -> binding_dst_t<from_t, to_t, providers::creator_t<to_t, std::decay_t<factory_t>>>
    {
        return binding_dst_t<from_t, to_t, providers::creator_t<to_t, std::decay_t<factory_t>>>{
            providers::creator_t<to_t, std::decay_t<factory_t>>{std::forward<factory_t>(factory)}
        };
    }

    // bind to internal reference
    template <typename to_t>
    auto to_internal_reference(to_t&& instance) const
        -> binding_dst_t<from_t, to_t, providers::internal_reference_t<std::decay_t<to_t>>>
    {
        return binding_dst_t<from_t, to_t, providers::internal_reference_t<std::decay_t<to_t>>>{
            providers::internal_reference_t<std::decay_t<to_t>>{std::forward<to_t>(instance)}
        };
    }

    // bind to external reference
    template <typename to_t>
    auto to_external_reference(to_t& instance) const
        -> binding_dst_t<from_t, to_t, providers::external_reference_t<to_t>>
    {
        return binding_dst_t<from_t, to_t, providers::external_reference_t<to_t>>{
            providers::external_reference_t<to_t>{&instance}
        };
    }

    // bind to internal prototype
    template <typename to_t>
    auto to_internal_prototype(to_t&& instance) const
        -> binding_dst_t<from_t, to_t, providers::internal_prototype_t<std::decay_t<to_t>>>
    {
        return binding_dst_t<from_t, to_t, providers::internal_prototype_t<std::decay_t<to_t>>>{
            providers::internal_prototype_t<std::decay_t<to_t>>{std::forward<to_t>(instance)}
        };
    }

    // bind to external prototype
    template <typename to_t>
    auto to_external_prototype(to_t const& instance) const
        -> binding_dst_t<from_t, to_t, providers::external_prototype_t<to_t>>
    {
        return binding_dst_t<from_t, to_t, providers::external_prototype_t<to_t>>{
            providers::external_prototype_t<to_t>{&instance}
        };
    }
};

//! entry point for binding dsl
template <typename from_t>
auto bind() -> binding_src_t<canonical_t<from_t>>
{
    static_assert(
        std::same_as<from_t, canonical_t<from_t>>,
        "Cannot bind non-canonical type. Bind the base type (e.g., T) not smart pointers or references (e.g., shared_ptr<T>)"
    );
    return {};
}

//! resolve default scope to provider's default (for creators) or no scope (for accessors)
template <typename from_t, typename to_t, typename provider_t, typename scope_t>
auto resolve_binding(binding_t<from_t, to_t, provider_t, scope_t>&& config)
{
    if constexpr (std::same_as<scope_t, scopes::default_t>)
    {
        if constexpr (providers::is_creator<provider_t>)
        {
            // use provider's default scope
            using resolved_scope_t = typename provider_t::default_scope;
            return binding_t<from_t, to_t, provider_t, resolved_scope_t>{std::move(config.provider)};
        }
        else
        {
            // accessors have no scope
            struct no_scope_t
            {};
            return binding_t<from_t, to_t, provider_t, no_scope_t>{std::move(config.provider)};
        }
    }
    else
    {
        // scope explicitly specified
        static_assert(providers::is_creator<provider_t>, "Cannot specify scope for accessor providers");
        return config; // already correct type
    }
}

//! resolve an incomplete binding builder by applying the default scope
template <typename from_t, typename to_t, typename provider_t>
auto resolve_binding(binding_dst_t<from_t, to_t, provider_t>&& dst)
{
    // explicitly invoke the conversion operator to get a config with a default scope
    binding_t<from_t, to_t, provider_t, scopes::default_t> config = std::move(dst);

    // delegate to the original function to resolve the default scope to the correct one
    return resolve_binding(std::move(config));
}

template <typename... bindings_t>
auto resolve_bindings(bindings_t&&... bindings)
{
    return std::make_tuple(resolve_binding(std::forward<bindings_t>(bindings))...);
}

} // namespace dink
