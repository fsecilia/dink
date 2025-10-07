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

template <typename value_t>
struct request_traits_t
{
    using value_type = value_t;
};

template <typename value_t>
struct request_traits_t<value_t&&>
{
    using value_type = value_t;
};

template <typename value_t>
struct request_traits_t<std::unique_ptr<value_t>>
{
    using value_type = value_t;
};

template <typename value_t>
struct request_traits_t<value_t*>
{
    using value_type = value_t;
};

template <typename value_t>
struct request_traits_t<value_t&>
{
    using value_type = value_t;
};

template <typename value_t>
struct request_traits_t<std::shared_ptr<value_t>>
{
    using value_type = value_t;
};

template <typename value_t>
struct request_traits_t<std::weak_ptr<value_t>>
{
    using value_type = value_t;
};

// Does this request type require moving/consuming the instance?
template <typename request_t>
concept requires_ownership = std::same_as<request_t, typename request_traits_t<request_t>::value_type&&>
    || std::is_same_v<request_t, std::unique_ptr<typename request_traits_t<request_t>::value_type>>;

// Does this request type return a non-owning reference that could dangle?
template <typename request_t>
concept requires_persistence = std::is_pointer_v<request_t>
    || std::is_lvalue_reference_v<request_t>
    || std::is_same_v<request_t, std::weak_ptr<typename request_traits_t<request_t>::value_type>>;

template <typename configured_scope_t, typename request_t>
using effective_scope_t = std::conditional_t<
    requires_ownership<request_t>, scopes::transient_t,
    std::conditional_t<
        requires_persistence<request_t> && std::same_as<configured_scope_t, scopes::transient_t>, scopes::scoped_t,
        configured_scope_t>>;

template <typename instance_t, typename root_container_t>
struct root_promotion_provider
{
    root_container_t* root_container;

    auto operator()() const -> instance_t { return root_container->template create_as_transient<instance_t>(); }
};

template <typename instance_t, typename provider_t>
auto get_promoted_singleton(provider_t& provider) -> std::shared_ptr<instance_t>&
{
    static auto instance = std::make_shared<instance_t>(provider());
    return instance;
}

// Helper function for adapting instances to request types
template <typename request_t, typename instance_t>
static auto adapt_to_request_type(instance_t&& instance)
{
    using value_type = typename request_traits_t<request_t>::value_type;

    if constexpr (std::same_as<request_t, value_type>) return std::forward<instance_t>(instance);
    else if constexpr (std::same_as<request_t, value_type&>) return static_cast<value_type&>(instance);
    else if constexpr (std::same_as<request_t, value_type&&>) return std::move(instance);
    else if constexpr (std::same_as<request_t, value_type*>) return &instance;
    else if constexpr (std::same_as<request_t, std::unique_ptr<value_type>>)
    {
        return std::make_unique<value_type>(std::forward<instance_t>(instance));
    }
    else if constexpr (std::same_as<request_t, std::shared_ptr<value_type>>)
    {
        if constexpr (std::same_as<std::remove_cvref_t<instance_t>, std::shared_ptr<value_type>>) { return instance; }
        else { return std::make_shared<value_type>(std::forward<instance_t>(instance)); }
    }
    else if constexpr (std::same_as<request_t, std::weak_ptr<value_type>>)
    {
        static_assert(
            std::same_as<std::remove_cvref_t<instance_t>, std::shared_ptr<value_type>>,
            "weak_ptr can only be created from shared_ptr"
        );
        return std::weak_ptr<value_type>(instance);
    }
}

// Shared helpers for binding lookup
template <typename from_t, typename tuple_t>
static constexpr bool has_binding_in_tuple();

template <typename from_t, typename tuple_t>
static constexpr auto& find_binding_in_tuple(tuple_t& t);

template <typename binding_t>
static auto get_slot_ptr(binding_t& binding)
{
    if constexpr (requires { binding.slot; }) { return &binding.slot; }
    else { return static_cast<decltype(&binding.slot)>(nullptr); }
}

// Common base class with shared resolution logic
class container_common_t
{
protected:
    template <typename container_t, typename request_t, typename find_in_chain_fn, typename scoped_resolver_fn>
    auto resolve_impl(container_t& container, find_in_chain_fn find_in_chain, scoped_resolver_fn scoped_resolver)
    {
        using value_type = typename request_traits_t<request_t>::value_type;

        auto [binding_ptr, slot_ptr, configured_scope] = find_in_chain.template operator()<value_type>();

        using effective_scope = effective_scope_t<decltype(configured_scope), request_t>;

        if (binding_ptr
            && providers::is_accessor<typename std::remove_pointer_t<decltype(binding_ptr)>::binding_t::provider_t>)
        {
            return resolve_accessor<request_t>(binding_ptr);
        }
        else if constexpr (std::same_as<effective_scope, scopes::transient_t>)
        {
            return resolve_transient<container_t, request_t, value_type>(container, binding_ptr);
        }
        else if constexpr (std::same_as<effective_scope, scopes::singleton_t>)
        {
            return resolve_singleton<request_t>(binding_ptr);
        }
        else
        {
            return resolve_scoped<container_t, request_t, value_type>(
                container, binding_ptr, slot_ptr, scoped_resolver
            );
        }
    }

    template <typename request_t, typename binding_ptr_t>
    auto resolve_accessor(binding_ptr_t binding_ptr)
    {
        return adapt_to_request_type<request_t>(binding_ptr->binding.provider());
    }

    template <typename container_t, typename request_t, typename instance_t, typename binding_ptr_t>
    auto resolve_transient(container_t& container, binding_ptr_t binding_ptr)
    {
        if (binding_ptr) { return adapt_to_request_type<request_t>(binding_ptr->binding.provider(container)); }
        else
        {
            return adapt_to_request_type<request_t>(
                container.default_provider_.template operator()<instance_t>(container)
            );
        }
    }

    template <typename request_t, typename binding_ptr_t>
    auto resolve_singleton(binding_ptr_t binding_ptr)
    {
        return adapt_to_request_type<request_t>(binding_ptr->get_or_create());
    }

    template <
        typename container_t, typename request_t, typename instance_t, typename binding_ptr_t, typename slot_ptr_t,
        typename scoped_resolver_t>
    auto resolve_scoped(
        container_t& container, binding_ptr_t binding_ptr, slot_ptr_t slot_ptr, scoped_resolver_t scoped_resolver
    )
    {
        if (slot_ptr && slot_ptr->instance) { return adapt_to_request_type<request_t>(*slot_ptr->instance); }

        if (slot_ptr)
        {
            slot_ptr->instance = std::make_shared<instance_t>(binding_ptr->binding.provider(container));
            return adapt_to_request_type<request_t>(*slot_ptr->instance);
        }

        // No slot - delegate to container-specific resolver (strategy pattern)
        return scoped_resolver.template operator()<request_t, instance_t>();
    }
};

// Forward declarations
template <typename... resolved_bindings_t>
class root_container_t;

template <typename parent_t, typename... resolved_bindings_t>
class child_container_t;

// Root container - no parent, no cache
template <typename... resolved_bindings_t>
class root_container_t : private container_common_t
{
public:
    template <typename... bindings_t>
    explicit root_container_t(bindings_t&&... bindings)
        : bindings_{resolve_bindings<root_container_tag_t>(*this, std::forward<bindings_t>(bindings)...)}
    {}

    template <typename request_t>
    auto resolve()
    {
        auto find_in_chain_fn = [this]<typename inst_t>() { return this->find_in_chain<inst_t>(); };

        auto scoped_resolver = [this]<typename req_t, typename inst_t>() {
            root_promotion_provider<inst_t, root_container_t> provider{this};
            auto instance = get_promoted_singleton<inst_t, decltype(provider)>(provider);
            return adapt_to_request_type<req_t>(*instance);
        };

        return this->template resolve_impl<root_container_t, request_t>(*this, find_in_chain_fn, scoped_resolver);
    }

    template <typename instance_t>
    auto create_as_transient() -> instance_t
    {
        auto [binding_ptr, slot_ptr, configured_scope] = find_in_chain<instance_t>();
        if (binding_ptr) { return binding_ptr->binding.provider(*this); }
        else { return auto_wire<instance_t>(*this); }
    }

    template <typename instance_t>
    auto find_in_chain()
    {
        if constexpr (has_binding_in_tuple<instance_t, decltype(bindings_)>())
        {
            auto& decorated = find_binding_in_tuple<instance_t>(bindings_);
            return std::make_tuple(
                &decorated, get_slot_ptr(decorated), typename decltype(decorated)::binding_t::resolved_scope_t{}
            );
        }
        else
        {
            // Not found - return "null" result
            using dummy_binding_t = resolved_binding_t<
                binding_t<instance_t, instance_t, providers::ctor_invoker_t, scopes::transient_t>,
                root_container_tag_t>;
            using no_binding_ptr = std::remove_pointer_t<decltype(&std::declval<dummy_binding_t>())>*;
            using no_slot_ptr = decltype(std::declval<dummy_binding_t>().slot)*;

            return std::make_tuple(
                static_cast<no_binding_ptr>(nullptr), static_cast<no_slot_ptr>(nullptr), scopes::transient_t{}
            );
        }
    }

    template <typename instance_t>
    auto find_cached_instance() const noexcept -> std::shared_ptr<instance_t>
    {
        return nullptr;
    }

private:
    providers::ctor_invoker_t default_provider_;
    std::tuple<resolved_bindings_t...> bindings_;
};

// Child container - has parent and cache
template <typename parent_t, typename... resolved_bindings_t>
class child_container_t : private container_common_t
{
public:
    // Main constructor - uses default ctor_invoker
    template <typename... bindings_t>
    explicit child_container_t(parent_t& parent, bindings_t&&... bindings)
        : child_container_t(parent, providers::ctor_invoker_t{}, std::forward<bindings_t>(bindings)...)
    {}

    // Testing constructor - can inject custom default provider
    template <typename default_provider_t, typename... bindings_t>
    explicit child_container_t(parent_t& parent, default_provider_t default_provider, bindings_t&&... bindings)
        : parent_{&parent}, default_provider_{std::move(default_provider)}, cache_{}
    {
        this->bindings_ = resolve_bindings<child_container_tag_t>(*this, std::forward<bindings_t>(bindings)...);
    }

    template <typename request_t>
    auto resolve()
    {
        auto find_in_chain_fn = [this]<typename inst_t>() { return this->find_in_chain<inst_t>(); };

        auto scoped_resolver = [this]<typename req_t, typename inst_t>() {
            if (auto cached = find_cached_instance<inst_t>()) { return adapt_to_request_type<req_t>(*cached); }

            auto instance = cache_.template get_or_create<inst_t>([&]() {
                return this->create_as_transient<inst_t>();
            });

            return adapt_to_request_type<req_t>(*instance);
        };

        return this->template resolve_impl<child_container_t, request_t>(*this, find_in_chain_fn, scoped_resolver);
    }

    template <typename instance_t>
    auto create_as_transient() -> instance_t
    {
        auto [binding_ptr, slot_ptr, configured_scope] = find_in_chain<instance_t>();
        if (binding_ptr) { return binding_ptr->binding.provider(*this); }
        else { return default_provider_.template operator()<instance_t>(*this); }
    }

    template <typename instance_t>
    auto find_in_chain()
    {
        if constexpr (has_binding_in_tuple<instance_t, decltype(bindings_)>())
        {
            auto& decorated = find_binding_in_tuple<instance_t>(bindings_);
            return std::make_tuple(
                &decorated, get_slot_ptr(decorated), typename decltype(decorated)::binding_t::resolved_scope_t{}
            );
        }
        else
        {
            // Not found locally - check parent
            return parent_->template find_in_chain<instance_t>();
        }
    }

    template <typename instance_t>
    auto find_cached_instance() noexcept -> std::shared_ptr<instance_t>
    {
        if (auto cached = cache_.template get<instance_t>()) { return cached; }
        return parent_->template find_cached_instance<instance_t>();
    }

private:
    parent_t* parent_;
    providers::ctor_invoker_t default_provider_;
    std::tuple<resolved_bindings_t...> bindings_;
    instance_cache_t cache_;
};

// Deduction guide for root container
template <typename... bindings_t>
root_container_t(bindings_t&&...) -> root_container_t<decltype(resolve_binding<root_container_tag_t>(
    std::declval<bindings_t>(), std::declval<root_container_t<>&>()
))...>;

// Deduction guide for child container
template <typename parent_t, typename... bindings_t>
child_container_t(parent_t&, bindings_t&&...) -> child_container_t<
    parent_t,
    decltype(resolve_binding<child_container_tag_t>(
        std::declval<bindings_t>(), std::declval<child_container_t<parent_t>&>()
    ))...>;

} // namespace dink
