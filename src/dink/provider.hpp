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

template <typename resolved_t, typename invoker_factory_t = invoker_factory_t>
class ctor_invoker_t {
public:
    using default_scope_t = scope::transient_t;
    using provided_t      = resolved_t;

    template <typename request_t, typename dependency_chain_t, stability_t stability, typename container_t>
    auto create(container_t& container) -> auto {
        return create<request_t>(
            container,
            invoker_factory_.template create<resolved_t, void, dependency_chain_t, stability, container_t>());
    }

    template <typename request_t, typename container_t, typename invoker_t>
    auto create(container_t& container, invoker_t&& invoker) -> auto {
        if constexpr (is_unique_ptr_v<request_t>) {
            return std::forward<invoker_t>(invoker).create_unique(container);
        } else if constexpr (is_shared_ptr_v<request_t>) {
            return std::forward<invoker_t>(invoker).create_shared(container);
        } else {
            return std::forward<invoker_t>(invoker).create_value(container);
        }
    }

    explicit ctor_invoker_t(invoker_factory_t invoker_factory = {}) noexcept
        : invoker_factory_{std::move(invoker_factory)} {}

private:
    [[no_unique_address]] invoker_factory_t invoker_factory_{};
};

template <typename resolved_t, typename resolved_factory_t, typename invoker_factory_t = invoker_factory_t>
class factory_invoker_t {
public:
    using default_scope_t = scope::transient_t;
    using provided_t      = resolved_t;

    template <typename request_t, typename dependency_chain_t, stability_t stability, typename container_t>
    auto create(container_t& container) -> auto {
        return create<request_t>(
            container,
            invoker_factory_
                .template create<resolved_t, resolved_factory_t, dependency_chain_t, stability, container_t>());
    }

    template <typename request_t, typename container_t, typename invoker_t>
    auto create(container_t& container, invoker_t&& invoker) -> auto {
        if constexpr (is_unique_ptr_v<request_t>) {
            return std::forward<invoker_t>(invoker).create_unique(resolved_factory_, container);
        } else if constexpr (is_shared_ptr_v<request_t>) {
            return std::forward<invoker_t>(invoker).create_shared(resolved_factory_, container);
        } else {
            return std::forward<invoker_t>(invoker).create_value(resolved_factory_, container);
        }
    }

    explicit factory_invoker_t(resolved_factory_t resolved_factory = {},
                               invoker_factory_t  invoker_factory  = {}) noexcept
        : resolved_factory_{std::move(resolved_factory)}, invoker_factory_{std::move(invoker_factory)} {}

private:
    [[no_unique_address]] resolved_factory_t resolved_factory_;
    [[no_unique_address]] invoker_factory_t  invoker_factory_{};
};

//! references an instance owned by the container (moved/copied in)
template <typename instance_t>
class internal_reference_t {
public:
    using provided_t = instance_t;

    auto get() -> instance_t& { return instance_; }
    auto get() const -> instance_t const& { return instance_; }

    explicit internal_reference_t(instance_t&& instance) noexcept : instance_{std::move(instance)} {}

private:
    instance_t instance_;
};

//! references an instance owned externally (pointer stored)
template <typename instance_t>
class external_reference_t {
public:
    using provided_t = instance_t;

    auto get() -> instance_t& { return *instance_; }
    auto get() const -> instance_t const& { return *instance_; }

    explicit external_reference_t(instance_t& instance) noexcept : instance_{&instance} {}

private:
    instance_t* instance_;
};

//! copies from a prototype owned by the container (moved/copied in)
template <typename instance_t>
class internal_prototype_t {
public:
    using provided_t = instance_t;

    auto get() -> instance_t { return instance_; }
    auto get() const -> instance_t const { return instance_; }

    explicit internal_prototype_t(instance_t&& instance) noexcept : instance_{std::move(instance)} {}

private:
    instance_t instance_;
};

//! copies from an externally-owned prototype (pointer stored)
template <typename instance_t>
class external_prototype_t {
public:
    using provided_t = instance_t;

    auto get() -> instance_t { return *instance_; }
    auto get() const -> instance_t const { return *instance_; }

    explicit external_prototype_t(instance_t& instance) noexcept : instance_{&instance} {}

private:
    instance_t* instance_;
};

//! default provider, if unspecified, invokes the resolved type's ctor
template <typename request_t>
using default_t = ctor_invoker_t<resolved_t<request_t>>;

//! factory for providers
template <template <typename provided_t> class provider_t>
class provider_factory_t {
public:
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
