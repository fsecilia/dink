/*!
    \file
    Copyright (C) 2025 Frank Secilia
*/

#pragma once

#include <dink/lib.hpp>
#include <dink/binding_transform.hpp>
#include <dink/bindings.hpp>
#include <dink/instance_cache.hpp>
#include <dink/request_traits.hpp>
#include <concepts>
#include <memory>
#include <type_traits>
#include <utility>

namespace dink {

template <typename binding_t>
struct binding_descriptor_t
{
    binding_t* binding = nullptr;
    using scope_type = typename binding_t::binding_t::resolved_scope_t;

    bool found() const { return binding != nullptr; }

    bool has_slot() const
    {
        if (!binding) return false;
        return requires { binding->scope.slot; };
    }

    auto get_slot() -> auto*
    {
        if constexpr (requires { binding->scope.slot; }) return &binding->scope.slot;
        return nullptr;
    }

    bool is_accessor() const
    {
        if (!binding) return false;
        return providers::is_accessor<typename binding_t::provider_t>;
    }
};

namespace policies {
namespace storage {

struct no_storage_t
{
    template <typename T>
    auto get_cached() -> std::shared_ptr<T>
    {
        return nullptr;
    }

    template <typename T, typename factory_t>
    auto get_or_create_cached(factory_t&&) -> std::shared_ptr<T>
    {
        return nullptr;
    }
};

struct instance_cache_storage_t
{
    instance_cache_t cache_;

    template <typename T>
    auto get_cached() -> std::shared_ptr<T>
    {
        return cache_.template get<T>();
    }

    template <typename T, typename factory_t>
    auto get_or_create_cached(factory_t&& factory) -> std::shared_ptr<T>
    {
        return cache_.template get_or_create<T>(std::forward<factory_t>(factory));
    }
};

} // namespace storage

namespace nesting {

struct no_parent_t
{
    template <typename T>
    constexpr auto find_parent_binding()
    {
        using null_t = binding_t<scope_binding_t<
            binding_config_t<T, T, providers::ctor_invoker_t, scopes::transient_t>, root_container_tag_t>>;
        return binding_descriptor_t<null_t>{};
    }

    template <typename T>
    auto find_parent_cached() -> std::shared_ptr<T>
    {
        return nullptr;
    }
};

template <typename parent_t>
struct child_t
{
    parent_t* parent;

    explicit child_t(parent_t& p) : parent{&p} {}

    template <typename T>
    constexpr auto find_parent_binding()
    {
        return parent->template find_binding<T>();
    }

    template <typename T>
    auto find_parent_cached() -> std::shared_ptr<T>
    {
        return parent->template find_cached<T>();
    }
};

} // namespace nesting

namespace scope_resolution {

struct static_t
{
    template <typename request_t, typename value_t, typename container_t>
    auto resolve_scoped_no_slot(container_t* container)
    {
        // Root promotes scoped to singleton
        auto provider = [container]() { return container->template create_transient<value_t>(); };
        static auto instance = std::make_shared<value_t>(provider());
        return as_requested<request_t>(*instance);
    }
};

template <typename nesting_t, typename storage_t>
struct instance_cache_t
{
    nesting_t* nesting_;
    storage_t* storage_;

    instance_cache_t(storage_t* storage, nesting_t* nesting) : storage_{storage}, nesting_{nesting} {}

    template <typename request_t, typename value_t, typename container_t>
    auto resolve_scoped_no_slot(container_t* container) -> auto
    {
        // Check cache hierarchy
        if (auto cached = storage_->template get_cached<value_t>()) { return as_requested<request_t>(*cached); }
        if (auto cached = nesting_->template find_parent_cached<value_t>()) { return as_requested<request_t>(*cached); }

        // Create and cache
        auto instance = storage_->template get_or_create_cached<value_t>([container]() {
            return container->template create_transient<value_t>();
        });
        return as_requested<request_t>(*instance);
    }
};

} // namespace scope_resolution
} // namespace policies

// Container implementation
template <typename policy_t, typename... resolved_bindings_t>
class container_t : private policy_t::storage_t, private policy_t::nesting_t
{
public:
    using storage_policy_t = policy_t::storage_t;
    using nesting_policy_t = policy_t::nesting_t;
    using scoped_policy_t = policy_t::scoped_t;

    // Constructor for root
    template <typename... bindings_t>
    requires std::same_as<nesting_policy_t, policies::nesting::no_parent_t>
    explicit container_t(bindings_t&&... bindings)
        : bindings_{resolve_bindings<root_container_tag_t>(*this, std::forward<bindings_t>(bindings)...)},
          scoped_resolver_{}
    {}

    // Constructor for child
    template <typename parent_t, typename... bindings_t>
    requires(!std::same_as<nesting_policy_t, policies::nesting::no_parent_t>)
    explicit container_t(parent_t& parent, bindings_t&&... bindings)
        : nesting_policy_t(parent),
          bindings_{resolve_bindings<child_container_tag_t>(*this, std::forward<bindings_t>(bindings)...)},
          scoped_resolver_{static_cast<storage_policy_t*>(this), static_cast<nesting_policy_t*>(this)}
    {}

    template <typename request_t>
    auto resolve()
    {
        using resolved_t = resolved_t<request_t>;
        auto binding_descriptor = find_binding<resolved_t>();

        // if no binding is found, the type is transient, so use the default provider
        static auto const is_transient = !binding_descriptor.found();
        if (is_transient) return as_requested<request_t>(default_provider_.template operator()<resolved_t>(*this));

        // accessors don't use any caching, so just return them immediately
        if (binding_descriptor.is_accessor())
        {
            return as_requested<request_t>(binding_descriptor.binding->binding.provider());
        }

        // use scope-based caching
        using effective_scope_t = effective_scope_t<typename decltype(binding_descriptor)::scope_type, request_t>;
        if constexpr (std::same_as<effective_scope_t, scopes::transient_t>)
        {
            return as_requested<request_t>(binding_descriptor.binding->binding.provider(*this));
        }
        else if constexpr (std::same_as<effective_scope_t, scopes::singleton_t>)
        {
            return as_requested<request_t>(binding_descriptor.binding->get_or_create());
        }
        else
        {
            // effective scope is scoped
            if constexpr (binding_descriptor.has_slot())
            {
                auto instance = binding_descriptor.slot()->instance;
                if (!instance)
                {
                    instance = std::make_shared<resolved_t>(binding_descriptor.binding->config.provider(*this));
                    binding_descriptor.slot()->instance = instance;
                }
                return as_requested<request_t>(*instance);
            }

            return scoped_resolver_.template resolve_scoped_no_slot<request_t, resolved_t>(this);
        }
    }

    template <typename value_t>
    constexpr auto find_binding()
    {
        constexpr size_t local_idx = find_binding_index<value_t>();
        if constexpr (local_idx < sizeof...(resolved_bindings_t))
        {
            auto& binding = std::get<local_idx>(bindings_);
            return binding_descriptor_t<std::remove_reference_t<decltype(binding)>>{&binding};
        }
        else { return nesting_policy_t::template find_parent_binding<value_t>(); }
    }

    template <typename value_t>
    auto find_cached() -> std::shared_ptr<value_t>
    {
        if (auto cached = storage_policy_t::template get_cached<value_t>()) return cached;
        return nesting_policy_t::template find_parent_cached<value_t>();
    }

    template <typename instance_t>
    auto create_transient() -> instance_t
    {
        auto binding_descriptor = find_binding<instance_t>();
        if (binding_descriptor.found()) return binding_descriptor.binding->binding.provider(*this);
        return default_provider_.template operator()<instance_t>(*this);
    }

private:
    template <typename T, size_t I = 0>
    static consteval size_t find_binding_index()
    {
        if constexpr (I >= sizeof...(resolved_bindings_t)) return I;
        else if constexpr (std::same_as<T, typename std::tuple_element_t<I, decltype(bindings_)>::from_t>) return I;
        else return find_binding_index<T, I + 1>();
    }

    std::tuple<resolved_bindings_t...> bindings_;
    providers::ctor_invoker_t default_provider_;
    scoped_policy_t scoped_resolver_;
};

struct root_container_policy_t
{
    using storage_t = policies::storage::no_storage_t;
    using nesting_t = policies::nesting::no_parent_t;
    using scope_resolution_t = policies::scope_resolution::static_t;
};

template <typename... resolved_bindings_t>
using root_container_t = container_t<root_container_policy_t>;

template <typename parent_t>
struct child_container_policy_t
{
    using storage_t = policies::storage::instance_cache_storage_t;
    using nesting_t = policies::nesting::child_t<parent_t>;
    using scope_resolution_t = policies::scope_resolution::instance_cache_t<nesting_t, storage_t>;
};

template <typename parent_t, typename... resolved_bindings_t>
using child_container_t = container_t<child_container_policy_t<parent_t>, resolved_bindings_t...>;

} // namespace dink
