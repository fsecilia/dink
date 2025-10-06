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

// Forward declarations for policies
struct root_container_policy_t;
template <typename parent_t>
struct child_container_policy_t;

// Root container policy - no parent, no cache
struct root_container_policy_t
{
    template <typename instance_t>
    auto find_in_parent_chain()
    {
        // Return "not found" result
        using dummy_binding_t = resolved_binding_t<
            binding_t<instance_t, instance_t, providers::ctor_invoker_t, scopes::transient_t>, root_container_tag_t>;
        using no_binding_ptr = std::remove_pointer_t<decltype(&std::declval<dummy_binding_t>())>*;
        using no_slot_ptr = decltype(std::declval<dummy_binding_t>().slot)*;

        return std::make_tuple(
            static_cast<no_binding_ptr>(nullptr), static_cast<no_slot_ptr>(nullptr), scopes::transient_t{}
        );
    }

    template <typename instance_t>
    auto find_cached_instance() const noexcept -> std::shared_ptr<instance_t>
    {
        return nullptr;
    }

    static constexpr root_container_tag_t container_tag{};
};

// Child container policy - has parent and cache
template <typename parent_t>
struct child_container_policy_t
{
    parent_t* parent_;
    instance_cache_t cache_;

    template <typename instance_t>
    auto find_in_parent_chain()
    {
        return parent_->template find_in_chain<instance_t>();
    }

    template <typename instance_t>
    auto find_cached_instance() const noexcept -> std::shared_ptr<instance_t>
    {
        if (auto cached = cache_.template get<instance_t>()) { return cached; }
        return parent_->template find_cached_instance<instance_t>();
    }

    static constexpr child_container_tag_t container_tag{};
};

template <typename policy_t, typename... resolved_bindings_t>
class container_t : private policy_t
{
public:
    using policy_type = policy_t;

    template <typename request_t>
    auto resolve()
    {
        using value_type = typename request_traits_t<request_t>::value_type;

        auto [binding_ptr, slot_ptr, configured_scope] = find_binding<value_type>();

        using effective_scope = effective_scope_t<decltype(configured_scope), request_t>;

        if (binding_ptr
            && providers::is_accessor<typename std::remove_pointer_t<decltype(binding_ptr)>::binding_t::provider_t>)
        {
            return resolve_accessor<request_t>(binding_ptr);
        }
        else if constexpr (std::same_as<effective_scope, scopes::transient_t>)
        {
            return resolve_transient<request_t, value_type>(binding_ptr);
        }
        else if constexpr (std::same_as<effective_scope, scopes::singleton_t>)
        {
            return resolve_singleton<request_t>(binding_ptr);
        }
        else { return resolve_scoped<request_t, value_type>(binding_ptr, slot_ptr); }
    }

    template <typename instance_t>
    auto create_as_transient() -> instance_t
    {
        auto [binding_ptr, slot_ptr, configured_scope] = find_binding<instance_t>();
        return create_as_transient_impl<instance_t>(binding_ptr);
    }

    template <typename instance_t>
    auto find_in_chain()
    {
        if constexpr (has_binding<instance_t, decltype(bindings_)>())
        {
            auto& decorated = find_binding<instance_t>(bindings_);
            return std::make_tuple(
                &decorated, get_slot_ptr(decorated), typename decltype(decorated)::binding_t::resolved_scope_t{}
            );
        }
        else { return this->template find_in_parent_chain<instance_t>(); }
    }

    template <typename instance_t>
    auto find_cached_instance() const noexcept -> std::shared_ptr<instance_t>
    {
        return this->policy_t::template find_cached_instance<instance_t>();
    }

protected:
    // Constructor for root container
    template <typename... bindings_t>
    requires std::same_as<policy_t, root_container_policy_t>
    explicit container_t(bindings_t&&... bindings)
        : policy_t{}, bindings_{resolve_bindings<root_container_tag_t>(*this, std::forward<bindings_t>(bindings)...)}
    {}

    // Constructor for child container
    template <typename parent_t, typename... bindings_t>
    requires std::same_as<policy_t, child_container_policy_t<parent_t>>
    explicit container_t(parent_t& parent, bindings_t&&... bindings)
        : policy_t{&parent, {}},
          bindings_{resolve_bindings<child_container_tag_t>(*this, std::forward<bindings_t>(bindings)...)}
    {}

private:
    template <typename request_t, typename binding_ptr_t>
    auto resolve_accessor(binding_ptr_t binding_ptr)
    {
        return adapt_to_request_type<request_t>(binding_ptr->binding.provider());
    }

    template <typename request_t, typename instance_t, typename binding_ptr_t>
    auto resolve_transient(binding_ptr_t binding_ptr)
    {
        return adapt_to_request_type<request_t>(create_as_transient_impl<instance_t>(binding_ptr));
    }

    template <typename request_t, typename binding_ptr_t>
    auto resolve_singleton(binding_ptr_t binding_ptr)
    {
        return adapt_to_request_type<request_t>(binding_ptr->get_or_create());
    }

    template <typename request_t, typename instance_t, typename binding_ptr_t, typename slot_ptr_t>
    auto resolve_scoped(binding_ptr_t binding_ptr, slot_ptr_t slot_ptr)
    {
        if (slot_ptr && slot_ptr->instance) { return adapt_to_request_type<request_t>(*slot_ptr->instance); }

        if (slot_ptr) { return resolve_scoped_with_slot<request_t, instance_t>(binding_ptr, slot_ptr); }

        // Check if we're in a root container
        if constexpr (std::same_as<policy_t, root_container_policy_t>)
        {
            return resolve_scoped_in_root<request_t, instance_t>();
        }
        else { return resolve_scoped_in_child<request_t, instance_t>(); }
    }

    template <typename request_t, typename instance_t, typename binding_ptr_t, typename slot_ptr_t>
    auto resolve_scoped_with_slot(binding_ptr_t binding_ptr, slot_ptr_t slot_ptr)
    {
        slot_ptr->instance = std::make_shared<instance_t>(binding_ptr->binding.provider(*this));
        return adapt_to_request_type<request_t>(*slot_ptr->instance);
    }

    template <typename request_t, typename instance_t>
    auto resolve_scoped_in_root()
    {
        root_promotion_provider<instance_t, decltype(*this)> provider{this};
        auto instance = get_promoted_singleton<instance_t, decltype(provider)>(provider);
        return adapt_to_request_type<request_t>(*instance);
    }

    template <typename request_t, typename instance_t>
    auto resolve_scoped_in_child()
    {
        if (auto cached = find_cached_instance<instance_t>()) { return adapt_to_request_type<request_t>(*cached); }

        auto instance = this->cache_.template get_or_create<instance_t>([&]() {
            return create_as_transient<instance_t>();
        });

        return adapt_to_request_type<request_t>(*instance);
    }

    template <typename instance_t, typename binding_ptr_t>
    auto create_as_transient_impl(binding_ptr_t binding_ptr) -> instance_t
    {
        if (binding_ptr) { return binding_ptr->binding.provider(*this); }
        else { return auto_wire<instance_t>(*this); }
    }

    template <typename from_t, typename tuple_t>
    static constexpr bool has_binding();

    template <typename from_t, typename tuple_t>
    static constexpr auto& find_binding(tuple_t& t);

    template <typename instance_t>
    auto find_binding()
    {
        if constexpr (has_binding<instance_t, decltype(bindings_)>())
        {
            auto& decorated = find_binding<instance_t>(bindings_);
            return std::make_tuple(
                &decorated, get_slot_ptr(decorated), typename decltype(decorated)::binding_t::resolved_scope_t{}
            );
        }
        else { return this->template find_in_parent_chain<instance_t>(); }
    }

    template <typename binding_t>
    auto get_slot_ptr(binding_t& binding)
    {
        if constexpr (requires { binding.slot; }) { return &binding.slot; }
        else { return static_cast<decltype(&binding.slot)>(nullptr); }
    }

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
            // shared_ptr<T> - if instance is already shared_ptr, return it; otherwise wrap
            if constexpr (std::same_as<std::remove_cvref_t<instance_t>, std::shared_ptr<value_type>>)
            {
                return instance;
            }
            else { return std::make_shared<value_type>(std::forward<instance_t>(instance)); }
        }
        else if constexpr (std::same_as<request_t, std::weak_ptr<value_type>>)
        {
            // weak_ptr<T> - instance must already be shared_ptr
            static_assert(
                std::same_as<std::remove_cvref_t<instance_t>, std::shared_ptr<value_type>>,
                "weak_ptr can only be created from shared_ptr"
            );
            return std::weak_ptr<value_type>(instance);
        }
    }

    template <typename instance_t, typename container_t>
    static instance_t auto_wire(container_t& container);

    std::tuple<resolved_bindings_t...> bindings_;
};

// Type aliases for user-facing types
template <typename... resolved_bindings_t>
using root_container_t = container_t<root_container_policy_t, resolved_bindings_t...>;

template <typename parent_t, typename... resolved_bindings_t>
using child_container_t = container_t<child_container_policy_t<parent_t>, resolved_bindings_t...>;

// Deduction guides
template <typename... bindings_t>
container_t(bindings_t&&...)
    ->root_container_t<decltype(resolve_binding<root_container_tag_t>(
        std::declval<bindings_t>(), std::declval<container_t<root_container_policy_t>&>()
    ))...>;

template <typename parent_t, typename... bindings_t>
container_t(parent_t&, bindings_t&&...)
    ->child_container_t<
        parent_t,
        decltype(resolve_binding<child_container_tag_t>(
            std::declval<bindings_t>(), std::declval < container_t<child_container_policy_t<parent_t>&>()
        ))...>;

} // namespace dink
