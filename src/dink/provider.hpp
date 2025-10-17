/*!
    \file
    Copyright (C) 2025 Frank Secilia
*/

#pragma once

#include <dink/lib.hpp>
#include <dink/arity.hpp>
#include <dink/invoker.hpp>
#include <dink/request_adapter.hpp>
#include <dink/scope.hpp>
#include <dink/smart_pointer_traits.hpp>

namespace dink::provider {

template <typename constructed_t>
struct ctor_invoker_t {
    using default_scope_t = scope::transient_t;
    using provided_t      = constructed_t;

    template <typename request_t, typename dependency_chain_t, stability_t stability, typename container_t>
    auto create(container_t& container) -> auto {
        return create<request_t>(
            container,
            invoker_factory_t{}.template create<constructed_t, void, dependency_chain_t, stability, container_t>(
                container));
    }

    template <typename request_t, typename container_t, typename invoker_t>
    auto create(container_t& container, invoker_t&& invoker) -> auto {
        if constexpr (is_unique_ptr_v<request_t>) {
            // construct directly into unique_ptr
            return std::forward<invoker_t>(invoker).create_unique(container);
        } else if constexpr (is_shared_ptr_v<request_t>) {
            // construct directly into shared_ptr
            return std::forward<invoker_t>(invoker).create_shared(container);
        } else {
            // construct T
            return std::forward<invoker_t>(invoker).create_value(container);
        }
    }
};

template <typename constructed_t, typename factory_t>
struct factory_invoker_t {
    using default_scope_t = scope::transient_t;
    using provided_t      = constructed_t;

    [[no_unique_address]] factory_t factory;

    template <typename request_t, typename dependency_chain_t, stability_t stability, typename container_t>
    auto create(container_t& container) -> auto {
        return create<request_t>(
            container,
            invoker_factory_t{}.template create<constructed_t, factory_t, dependency_chain_t, stability, container_t>(
                container));
    }

    template <typename request_t, typename container_t, typename invoker_t>
    auto create(container_t& container, invoker_t&& invoker) -> auto {
        if constexpr (is_unique_ptr_v<request_t>) {
            // construct directly into unique_ptr
            return std::forward<invoker_t>(invoker).create_unique(factory, container);
        } else if constexpr (is_shared_ptr_v<request_t>) {
            // construct directly into shared_ptr
            return std::forward<invoker_t>(invoker).create_shared(factory, container);
        } else {
            // construct T
            return std::forward<invoker_t>(invoker).create_value(factory, container);
        }
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
using default_t = ctor_invoker_t<resolved_t<request_t>>;

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

template <typename provider_t>
concept is_creator = requires {
    typename provider_t::provided_t;
    typename provider_t::default_scope_t;
};

template <typename provider_t>
concept is_accessor = requires(provider_t const& provider) {
    { provider.get() } -> std::convertible_to<typename provider_t::provided_t>;
};

}  // namespace dink::provider
