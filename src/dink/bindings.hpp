/*!
    \file
    Copyright (C) 2025 Frank Secilia
*/

#pragma once

#include <dink/lib.hpp>
#include <dink/canonical.hpp>
#include <dink/provider.hpp>
#include <dink/scope.hpp>
#include <concepts>
#include <type_traits>
#include <utility>

namespace dink {

//! binding configuration: from type -> to type -> provider -> scope
template <typename from_t, typename to_t, typename provider_t, typename scope_t>
struct binding_t {
    using from_type     = from_t;
    using to_type       = to_t;
    using provider_type = provider_t;
    using scope_type    = scope_t;

    [[no_unique_address]] provider_t provider;
};

//! binding builder after destination type is specified
template <typename from_t, typename to_t, typename provider_t>
class binding_dst_t {
public:
    using from_type     = from_t;
    using to_type       = to_t;
    using provider_type = provider_t;

    //! specifies scope for creators; unavailable for accessors
    template <typename scope_t>
        requires provider::is_creator<provider_t>
    auto in() && -> binding_t<from_t, to_t, provider_t, scope_t> {
        return {std::move(provider_)};
    }

    //! specifies default scope
    operator binding_t<from_t, to_t, provider_t, scope::default_t>() && { return {std::move(provider_)}; }

    explicit binding_dst_t(provider_t provider) : provider_(std::move(provider)) {}

private:
    provider_t provider_;
};

//! binding builder after source type is specified
template <typename from_t>
class binding_src_t {
public:
    // bind to type using ctor
    template <typename to_t>
    auto to() const -> auto {
        using provider_t = provider::ctor_invoker_t<to_t>;
        return binding_dst_t<from_t, to_t, provider_t>{provider_t{}};
    }

    // bind to type using factory
    template <typename to_t, typename factory_t>
    auto to_factory(factory_t&& factory) const -> auto {
        using provider_t = provider::factory_invoker_t<to_t, std::decay_t<factory_t>>;
        return binding_dst_t<from_t, to_t, provider_t>{provider_t{std::forward<factory_t>(factory)}};
    }

    // bind to internal reference
    template <typename to_t>
    auto to_internal_reference(to_t&& instance) const -> auto {
        using provider_t = provider::internal_reference_t<std::remove_cvref_t<to_t>>;
        return binding_dst_t<from_t, to_t, provider_t>{provider_t{std::forward<to_t>(instance)}};
    }

    // bind to external reference
    template <typename to_t>
    auto to_external_reference(to_t& instance) const -> auto {
        using provider_t = provider::external_reference_t<to_t>;
        return binding_dst_t<from_t, to_t, provider_t>{provider_t{instance}};
    }

    // bind to internal prototype
    template <typename to_t>
    auto to_internal_prototype(to_t&& instance) const -> auto {
        using provider_t = provider::internal_prototype_t<std::remove_cvref_t<to_t>>;
        return binding_dst_t<from_t, to_t, provider_t>{provider_t{std::forward<to_t>(instance)}};
    }

    // bind to external prototype
    template <typename to_t>
    auto to_external_prototype(to_t& instance) const -> auto {
        using provider_t = provider::external_prototype_t<to_t>;
        return binding_dst_t<from_t, to_t, provider_t>{provider_t{instance}};
    }

    //! specifies scope for creators; unavailable for accessors
    template <typename scope_t>
    auto in() && -> auto {
        using provider_t = provider::default_t<from_t>;
        return binding_t<from_t, from_t, provider_t, scope_t>{provider_t{}};
    }

    //! converts to final binding with default provider and scope
    operator binding_t<from_t, from_t, provider::default_t<from_t>, scope::default_t>() && {
        return {provider::default_t<from_t>{}};
    }
};

//! entry point for binding dsl
template <typename from_t>
auto bind() -> binding_src_t<canonical_t<from_t>> {
    static_assert(std::same_as<from_t, canonical_t<from_t>>,
                  "Cannot bind non-canonical type. Bind the base type (e.g., T) not smart pointers or references "
                  "(e.g., shared_ptr<T>)");
    return {};
}

namespace detail {

template <typename>
struct is_binding_f : std::false_type {};

template <typename from_t, typename to_t, typename scope_t, typename provider_t>
struct is_binding_f<binding_t<from_t, to_t, scope_t, provider_t>> : std::true_type {};

template <typename from_t, typename to_t, typename provider_t>
struct is_binding_f<binding_dst_t<from_t, to_t, provider_t>> : std::true_type {};

template <typename from_t>
struct is_binding_f<binding_src_t<from_t>> : std::true_type {};

}  // namespace detail

template <typename T>
concept is_binding = detail::is_binding_f<std::remove_cvref_t<T>>::value;

//! resolve default scope to provider's default (for creators) or no scope (for accessors)
template <typename from_t, typename to_t, typename provider_t, typename scope_t>
auto resolve_binding(binding_t<from_t, to_t, provider_t, scope_t>&& config) {
    if constexpr (std::same_as<scope_t, scope::default_t>) {
        if constexpr (provider::is_creator<provider_t>) {
            // use provider's default scope
            using resolved_scope_t = typename provider_t::default_scope_t;
            return binding_t<from_t, to_t, provider_t, resolved_scope_t>{std::move(config.provider)};
        } else {
            // accessors have no scope
            struct no_scope_t {};

            return binding_t<from_t, to_t, provider_t, no_scope_t>{std::move(config.provider)};
        }
    } else {
        // scope explicitly specified
        static_assert(provider::is_creator<provider_t>, "Cannot specify scope for accessor provider");
        return config;  // already correct type
    }
}

//! resolve an incomplete binding builder by applying the default scope
template <typename from_t, typename to_t, typename provider_t>
auto resolve_binding(binding_dst_t<from_t, to_t, provider_t>&& dst) {
    // explicitly invoke the conversion operator to get a config with a default scope
    binding_t<from_t, to_t, provider_t, scope::default_t> config = std::move(dst);

    // delegate to the original function to resolve the default scope to the correct one
    return resolve_binding(std::move(config));
}

template <is_binding... bindings_t>
auto resolve_bindings(bindings_t&&... bindings) {
    return std::make_tuple(resolve_binding(std::forward<bindings_t>(bindings))...);
}

}  // namespace dink
