/*!
    \file
    Copyright (C) 2025 Frank Secilia
*/

#pragma once

#include <dink/lib.hpp>
#include <dink/arity.hpp>
#include <dink/ctor_factory.hpp>
#include <dink/invoker.hpp>
#include <dink/request_traits.hpp>
#include <dink/scope.hpp>

namespace dink::provider {

//! unified creator provider - handles both ctors and user factories
template <typename constructed_t, typename factory_t = ctor_factory_t<constructed_t>>
struct creator_t {
    using default_scope_t = scope::transient_t;
    using provided_t      = constructed_t;

    [[no_unique_address]] factory_t factory;

    template <typename dependency_chain_t, typename container_t>
    auto create(container_t& container) -> constructed_t {
        using arg_t                 = arg_t<container_t, dependency_chain_t>;
        using single_arg_t          = single_arg_t<constructed_t, arg_t>;
        static constexpr auto arity = arity_v<constructed_t, factory_t>;
        return create<dependency_chain_t>(container, invoker_t<constructed_t, arity, arg_t, single_arg_t>{});
    }

    template <typename dependency_chain_t, typename container_t, typename factory_invoker_t>
    auto create(container_t& container, factory_invoker_t&& factory_invoker) -> constructed_t {
        return std::forward<factory_invoker_t>(factory_invoker).invoke_factory(factory, container);
    }
};

//! references an instance owned by the container (moved/copied in)
template <typename instance_t>
struct internal_reference_t {
    using provided_t = instance_t;
    instance_t instance;

    auto get() -> instance_t& { return instance; }
    auto get() const -> instance_t const& { return instance; }
};

//! references an instance owned externally (pointer stored)
template <typename instance_t>
struct external_reference_t {
    using provided_t = instance_t;
    instance_t* instance;

    auto get() -> instance_t& { return *instance; }
    auto get() const -> instance_t const& { return *instance; }
};

//! copies from a prototype owned by the container
template <typename instance_t>
struct internal_prototype_t {
    using provided_t = instance_t;
    instance_t prototype;

    auto get() const -> instance_t { return prototype; }
};

//! copies from an externally-owned prototype
template <typename instance_t>
struct external_prototype_t {
    using provided_t = instance_t;
    instance_t const* prototype;

    auto get() const -> instance_t { return *prototype; }
};

//! default provider, if unspecified, invokes the resolved type's ctor
template <typename request_t>
using default_t = creator_t<resolved_t<request_t>>;

//! factory for providers
template <template <typename provided_t> class provider_t>
struct provider_factory_t {
    template <typename provided_t>
    auto create() -> provider_t<provided_t> {
        return provider_t<provided_t>{};
    }
};

//! default factory creates default providers
using default_factory_t = provider_factory_t<default_t>;

template <typename value_t>
struct is_creator_f : std::false_type {};

template <typename constructed_t, typename factory_t>
struct is_creator_f<creator_t<constructed_t, factory_t>> : std::true_type {};

template <typename value_t>
inline static constexpr auto is_creator_v = is_creator_f<value_t>::value;

template <typename provider_t>
concept is_creator = requires(provider_t& provider, meta::concept_probe_t& container) {
    { provider.template create<type_list_t<>>(container) } -> std::same_as<typename provider_t::provided_t>;
};

template <typename provider_t>
concept is_accessor = requires(provider_t const& provider) {
    { provider.get() } -> std::convertible_to<typename provider_t::provided_t>;
};

}  // namespace dink::provider
