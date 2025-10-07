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

// Storage policies
struct no_storage_policy
{
    template <typename T>
    auto get_cached(T*) -> std::shared_ptr<T>
    {
        return nullptr;
    }

    template <typename T, typename factory_t>
    auto get_or_create_cached(T*, factory_t&&) -> std::shared_ptr<T>
    {
        return nullptr;
    }
};

struct cache_storage_policy
{
    instance_cache_t cache_;

    template <typename T>
    auto get_cached(T*) -> std::shared_ptr<T>
    {
        return cache_.template get<T>();
    }

    template <typename T, typename factory_t>
    auto get_or_create_cached(T*, factory_t&& factory) -> std::shared_ptr<T>
    {
        return cache_.template get_or_create<T>(std::forward<factory_t>(factory));
    }
};

// Parent policies
struct no_parent_policy
{
    template <typename T>
    auto find_parent_binding(T*)
    {
        using null_t = binding_t<scope_binding_t<
            binding_config_t<T, T, providers::ctor_invoker_t, scopes::transient_t>, root_container_tag_t>>;
        return binding_descriptor_t<null_t>{};
    }

    template <typename T>
    auto find_parent_cached(T*) -> std::shared_ptr<T>
    {
        return nullptr;
    }
};

template <typename parent_t>
struct has_parent_policy
{
    parent_t* parent_;

    explicit has_parent_policy(parent_t& p) : parent_(&p) {}

    template <typename T>
    auto find_parent_binding(T*)
    {
        return parent_->template find_binding<T>();
    }

    template <typename T>
    auto find_parent_cached(T*) -> std::shared_ptr<T>
    {
        return parent_->template find_cached<T>();
    }
};

// Scoped resolution policies
struct root_scoped_policy
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

template <typename storage_t, typename parent_t>
struct child_scoped_policy
{
    storage_t* storage_;
    parent_t* parent_;

    child_scoped_policy(storage_t* storage, parent_t* parent) : storage_{storage}, parent_{parent} {}

    template <typename request_t, typename value_t, typename container_t>
    auto resolve_scoped_no_slot(container_t* container) -> auto
    {
        // Check cache hierarchy
        if (auto cached = storage_->get_cached(static_cast<value_t*>(nullptr)))
        {
            return as_requested<request_t>(*cached);
        }
        if (auto cached = parent_->find_parent_cached(static_cast<value_t*>(nullptr)))
        {
            return as_requested<request_t>(*cached);
        }

        // Create and cache
        auto instance = storage_->get_or_create_cached(static_cast<value_t*>(nullptr), [container]() {
            return container->template create_transient<value_t>();
        });
        return as_requested<request_t>(*instance);
    }
};

// Container implementation
template <typename storage_policy, typename parent_policy, typename scoped_policy, typename... resolved_bindings_t>
class container_impl : private storage_policy, private parent_policy
{
public:
    // Constructor for root
    template <typename... bindings_t>
    requires std::same_as<parent_policy, no_parent_policy>
    explicit container_impl(bindings_t&&... bindings)
        : bindings_{resolve_bindings<root_container_tag_t>(*this, std::forward<bindings_t>(bindings)...)},
          scoped_resolver_{}
    {}

    // Constructor for child
    template <typename parent_t, typename... bindings_t>
    requires(!std::same_as<parent_policy, no_parent_policy>)
    explicit container_impl(parent_t& parent, bindings_t&&... bindings)
        : parent_policy(parent),
          bindings_{resolve_bindings<child_container_tag_t>(*this, std::forward<bindings_t>(bindings)...)},
          scoped_resolver_{static_cast<storage_policy*>(this), static_cast<parent_policy*>(this)}
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
    auto find_binding()
    {
        constexpr size_t idx = find_binding_index<value_t>();

        if constexpr (idx < sizeof...(resolved_bindings_t))
        {
            auto& binding = std::get<idx>(bindings_);
            using binding_t = std::remove_reference_t<decltype(binding)>;

            binding_descriptor_t<binding_t> binding_descriptor;
            binding_descriptor.binding = &binding;
            return binding_descriptor;
        }
        else { return parent_policy::find_parent_binding(static_cast<value_t*>(nullptr)); }
    }

    template <typename value_t>
    auto find_cached() -> std::shared_ptr<value_t>
    {
        if (auto cached = storage_policy::get_cached(static_cast<value_t*>(nullptr))) return cached;
        return parent_policy::find_parent_cached(static_cast<value_t*>(nullptr));
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
    static constexpr size_t find_binding_index()
    {
        if constexpr (I >= sizeof...(resolved_bindings_t)) return I;
        else if constexpr (std::same_as<T, typename std::tuple_element_t<I, decltype(bindings_)>::from_t>) return I;
        else return find_binding_index<T, I + 1>();
    }

    std::tuple<resolved_bindings_t...> bindings_;
    providers::ctor_invoker_t default_provider_;
    scoped_policy scoped_resolver_;
};

// Root container typedef
template <typename... resolved_bindings_t>
using root_container_t
    = container_impl<no_storage_policy, no_parent_policy, root_scoped_policy, resolved_bindings_t...>;

// Child container typedef
template <typename parent_t, typename... resolved_bindings_t>
using child_container_t = container_impl<
    cache_storage_policy, has_parent_policy<parent_t>,
    child_scoped_policy<cache_storage_policy, has_parent_policy<parent_t>>, resolved_bindings_t...>;

} // namespace dink
