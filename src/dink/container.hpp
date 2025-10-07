/*!
    \file
    Copyright (C) 2025 Frank Secilia
*/

#pragma once

#include <dink/lib.hpp>
#include <dink/binding_transform.hpp>
#include <dink/bindings.hpp>
#include <dink/instance_cache.hpp>
#include <concepts>
#include <memory>
#include <type_traits>
#include <utility>

namespace dink {

enum class request_kind
{
    value,
    owning,
    persistent
};

template <typename T>
struct request_traits
{
    using value_type = T;
    static constexpr request_kind kind = request_kind::value;
};

template <typename T>
struct request_traits<T&&>
{
    using value_type = T;
    static constexpr request_kind kind = request_kind::owning;
};

template <typename T>
struct request_traits<std::unique_ptr<T>>
{
    using value_type = T;
    static constexpr request_kind kind = request_kind::owning;
};

template <typename T>
struct request_traits<T*>
{
    using value_type = T;
    static constexpr request_kind kind = request_kind::persistent;
};

template <typename T>
struct request_traits<T&>
{
    using value_type = T;
    static constexpr request_kind kind = request_kind::persistent;
};

template <typename T>
struct request_traits<std::shared_ptr<T>>
{
    using value_type = T;
    static constexpr request_kind kind = request_kind::value;
};

template <typename T>
struct request_traits<std::weak_ptr<T>>
{
    using value_type = T;
    static constexpr request_kind kind = request_kind::persistent;
};

// Effective scope calculation
template <typename scope_t, typename request_t>
struct effective_scope
{
    using traits = request_traits<request_t>;
    using type = std::conditional_t<
        traits::kind == request_kind::owning, scopes::transient_t,
        std::conditional_t<
            traits::kind == request_kind::persistent && std::same_as<scope_t, scopes::transient_t>, scopes::scoped_t,
            scope_t>>;
};

template <typename scope_t, typename request_t>
using effective_scope_t = typename effective_scope<scope_t, request_t>::type;

template <typename request_t, typename instance_t>
auto adapt_instance(instance_t&& instance)
{
    using traits = request_traits<request_t>;
    using value_t = typename traits::value_type;

    if constexpr (std::same_as<request_t, value_t>) return std::forward<instance_t>(instance);
    else if constexpr (std::same_as<request_t, value_t&>) return static_cast<value_t&>(instance);
    else if constexpr (std::same_as<request_t, value_t&&>) return std::move(instance);
    else if constexpr (std::same_as<request_t, value_t*>) return &instance;
    else if constexpr (std::same_as<request_t, std::unique_ptr<value_t>>)
        return std::make_unique<value_t>(std::forward<instance_t>(instance));
    else if constexpr (std::same_as<request_t, std::shared_ptr<value_t>>)
    {
        if constexpr (std::same_as<std::remove_cvref_t<instance_t>, std::shared_ptr<value_t>>) return instance;
        else return std::make_shared<value_t>(std::forward<instance_t>(instance));
    }
    else if constexpr (std::same_as<request_t, std::weak_ptr<value_t>>) return std::weak_ptr<value_t>(instance);
}

// Binding info
template <typename binding_t>
struct binding_info
{
    binding_t* binding = nullptr;
    decltype(std::declval<binding_t>().slot)* slot = nullptr;
    using scope_type = typename binding_t::binding_t::resolved_scope_t;

    bool found() const { return binding != nullptr; }
    bool has_slot() const { return slot != nullptr; }
    bool is_accessor() const
    {
        if (!binding) return false;
        return providers::is_accessor<typename binding_t::binding_t::provider_t>;
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
        using dummy_t
            = resolved_binding_t<binding_t<T, T, providers::ctor_invoker_t, scopes::transient_t>, root_container_tag_t>;
        return binding_info<dummy_t>{};
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
        return adapt_instance<request_t>(*instance);
    }
};

template <typename storage_t, typename parent_t>
struct child_scoped_policy
{
    storage_t* storage_;
    parent_t* parent_;

    child_scoped_policy(storage_t* s, parent_t* p) : storage_(s), parent_(p) {}

    template <typename request_t, typename value_t, typename container_t>
    auto resolve_scoped_no_slot(container_t* container)
    {
        // Check cache hierarchy
        if (auto cached = storage_->get_cached(static_cast<value_t*>(nullptr)))
        {
            return adapt_instance<request_t>(*cached);
        }
        if (auto cached = parent_->find_parent_cached(static_cast<value_t*>(nullptr)))
        {
            return adapt_instance<request_t>(*cached);
        }

        // Create and cache
        auto instance = storage_->get_or_create_cached(static_cast<value_t*>(nullptr), [container]() {
            return container->template create_transient<value_t>();
        });
        return adapt_instance<request_t>(*instance);
    }
};

// Container implementation
template <typename storage_policy, typename parent_policy, typename scoped_policy, typename... resolved_bindings_t>
class container_impl : private storage_policy, private parent_policy
{
    std::tuple<resolved_bindings_t...> bindings_;
    providers::ctor_invoker_t default_provider_;
    scoped_policy scoped_resolver_;

    template <typename T, size_t I = 0>
    static constexpr size_t find_binding_index()
    {
        if constexpr (I >= sizeof...(resolved_bindings_t)) { return sizeof...(resolved_bindings_t); }
        else
        {
            using elem_t = std::tuple_element_t<I, decltype(bindings_)>;
            using from_t = typename elem_t::binding_t::from_t;
            if constexpr (std::same_as<T, from_t>) return I;
            else return find_binding_index<T, I + 1>();
        }
    }

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
        using value_t = typename request_traits<request_t>::value_type;
        auto info = find_binding<value_t>();

        if (!info.found()) { return adapt_instance<request_t>(default_provider_.template operator()<value_t>(*this)); }

        if (info.is_accessor()) { return adapt_instance<request_t>(info.binding->binding.provider()); }

        using scope = effective_scope_t<typename decltype(info)::scope_type, request_t>;

        if constexpr (std::same_as<scope, scopes::transient_t>)
        {
            return adapt_instance<request_t>(info.binding->binding.provider(*this));
        }
        else if constexpr (std::same_as<scope, scopes::singleton_t>)
        {
            return adapt_instance<request_t>(info.binding->get_or_create());
        }
        else
        { // scoped
            if (info.has_slot() && info.slot->instance) { return adapt_instance<request_t>(*info.slot->instance); }
            if (info.has_slot())
            {
                info.slot->instance = std::make_shared<value_t>(info.binding->binding.provider(*this));
                return adapt_instance<request_t>(*info.slot->instance);
            }
            return scoped_resolver_.template resolve_scoped_no_slot<request_t, value_t>(this);
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

            binding_info<binding_t> info;
            info.binding = &binding;
            if constexpr (requires { binding.slot; }) { info.slot = &binding.slot; }
            return info;
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
        auto info = find_binding<instance_t>();
        if (info.found()) { return info.binding->binding.provider(*this); }
        return default_provider_.template operator()<instance_t>(*this);
    }
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
