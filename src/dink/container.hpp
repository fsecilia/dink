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
    static constexpr bool forces_transient = false;
    static constexpr bool forces_scoped = false;
};

template <typename value_t>
struct request_traits_t<value_t&&>
{
    using value_type = value_t;
    static constexpr bool forces_transient = true;
};

template <typename value_t>
struct request_traits_t<std::unique_ptr<value_t>>
{
    using value_type = value_t;
    static constexpr bool forces_transient = true;
};

template <typename value_t>
struct request_traits_t<value_t*>
{
    using value_type = value_t;
    static constexpr bool forces_scoped = true;
};

template <typename value_t>
struct request_traits_t<value_t&>
{
    using value_type = value_t;
    static constexpr bool forces_scoped = true;
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
    static constexpr bool forces_scoped = true;
};

template <typename configured_scope_t, typename request_type_t>
struct effective_scope_f
{
    using type = std::conditional_t<
        request_traits_t<request_type_t>::forces_transient, scopes::transient_t,
        std::conditional_t<
            std::same_as<configured_scope_t, scopes::transient_t> && request_traits_t<request_type_t>::forces_scoped,
            scopes::scoped_t, configured_scope_t>>;
};

template <typename configured_scope_t, typename request_type_t>
using effective_scope_t = effective_scope_f<configured_scope_t, request_type_t>::type;

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

template <typename parent_container_t, typename... resolved_bindings_t>
class container_t
{
public:
    template <typename... bindings_t>
    explicit container_t(bindings_t&&... bindings)
        : container_t(resolve_bindings<root_container_tag_t>(std::forward<bindings_t>(bindings)...))
    {}

    template <typename parent_t, typename... bindings_t>
    explicit container_t(parent_t& parent, bindings_t&&... bindings)
        : container_t(parent, resolve_bindings<child_container_tag_t>(std::forward<bindings_t>(bindings)...))
    {}

    template <typename request_t>
    auto resolve()
    {
        using value_type = typename request_traits_t<request_t>::value_type;

        auto [binding_ptr, slot_ptr, configured_scope] = find_binding<value_type>();

        using effective_scope_t = typename effective_scope_f<decltype(configured_scope), request_t>::type;

        if (binding_ptr
            && providers::is_accessor<
                typename std::remove_pointer_t<decltype(binding_ptr)>::binding_type::provider_type>)
        {
            return resolve_accessor<request_t>(binding_ptr);
        }
        else if constexpr (std::same_as<effective_scope_t, scopes::transient_t>)
        {
            return resolve_transient<request_t, value_type>();
        }
        else if constexpr (std::same_as<effective_scope_t, scopes::singleton_t>)
        {
            return resolve_singleton<request_t>(binding_ptr);
        }
        else { return resolve_scoped<request_t, value_type>(binding_ptr, slot_ptr); }
    }

    template <typename instance_t>
    auto create_as_transient() -> instance_t
    {
        auto [binding_ptr, slot_ptr, configured_scope] = find_binding<instance_t>();

        if (binding_ptr) { return binding_ptr->binding.provider(*this); }
        else { return auto_wire<instance_t>(*this); }
    }

private:
    // Combined transform: finalize + decorate + bind to container_t
    template <typename container_tag_t, typename... bindings_t>
    auto resolve_bindings(bindings_t&&... bindings)
    {
        return std::make_tuple(resolve_binding<container_tag_t>(std::forward<bindings_t>(bindings))...);
    }

    template <typename container_tag_t, typename element_t>
    auto resolve_binding(element_t&& element)
    {
        // transform 1: finalize partial bindings (binding_target -> binding)
        auto finalized = finalize_binding(std::forward<element_t>(element));

        // transform 2: add scope infrastructure (binding -> resolved_binding)
        auto resolved_binding = resolved_binding_t<decltype(finalized), container_tag_t>{std::move(finalized)};

        // transform 3: bind provider to container_t (if needed for singleton/root-scoped)
        return bind_to_container<container_tag_t>(std::move(resolved_binding));
    }

    template <typename container_tag_t, typename prev_resolved_t>
    auto bind_to_container(prev_resolved_t&& resolved_binding)
    {
        using prev_binding_t = typename prev_resolved_t::binding_t;
        using resolved_scope_t = typename prev_binding_t::resolved_scope_t;

        if constexpr (std::same_as<resolved_scope_t, scopes::singleton_t>
                      || (std::same_as<resolved_scope_t, scopes::scoped_t>
                          && std::same_as<container_tag_t, root_container_tag_t>))
        {
            using prev_provider_t = typename prev_binding_t::provider_t;

            return resolved_binding_t<
                binding_t<
                    typename prev_binding_t::from_type, typename prev_binding_t::to_type,
                    bound_provider_t<prev_provider_t, container_t>, resolved_scope_t>,
                container_tag_t>{binding_t{
                bound_provider_t<prev_provider_t, container_t>{std::move(resolved_binding.binding.provider), this}
            }};
        }
        else { return std::forward<prev_resolved_t>(resolved_binding); }
    }

    template <typename request_t, typename binding_ptr_t>
    auto resolve_accessor(binding_ptr_t binding_ptr)
    {
        return adapt_to_request_type<request_t>(binding_ptr->binding.provider());
    }

    template <typename request_t, typename instance_t>
    auto resolve_transient()
    {
        auto instance = create_as_transient<instance_t>();
        return adapt_to_request_type<request_t>(std::move(instance));
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

        if constexpr (std::same_as<parent_container_t, void>)
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

        auto instance = cache_.template get_or_create<instance_t>([&]() { return create_as_transient<instance_t>(); });

        return adapt_to_request_type<request_t>(*instance);
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
                &decorated, get_slot_ptr(decorated), typename decltype(decorated)::binding_type::resolved_scope{}
            );
        }
        else if constexpr (!std::same_as<parent_container_t, void>)
        {
            if (parent_) { return parent_->template find_in_chain<instance_t>(); }
        }

        using no_binding_ptr = std::remove_pointer_t<
            decltype(&std::declval<resolved_binding_t<
                         binding_t<instance_t, instance_t, providers::ctor_invoker_t, scopes::transient_t>,
                         root_container_tag_t>>())>*;
        using no_slot_ptr = decltype(get_slot_ptr(
            std::declval<resolved_binding_t<
                binding_t<instance_t, instance_t, providers::ctor_invoker_t, scopes::transient_t>,
                root_container_tag_t>>()
        ));
        return std::make_tuple(
            static_cast<no_binding_ptr>(nullptr), static_cast<no_slot_ptr>(nullptr), scopes::transient_t{}
        );
    }

    template <typename instance_t>
    auto find_cached_instance() const noexcept -> std::shared_ptr<instance_t>
    {
        if constexpr (!std::same_as<parent_container_t, void>)
        {
            if (auto cached = cache_.template get<instance_t>()) { return cached; }
        }

        if constexpr (!std::same_as<parent_container_t, void>)
        {
            if (parent_) { return parent_->template find_cached_instance<instance_t>(); }
        }

        return nullptr;
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

    container_t(std::tuple<resolved_bindings_t...> resolved_bindings) noexcept
        : parent_{nullptr}, bindings_{std::move(resolved_bindings)}, cache_{}
    {}

    container_t(parent_container_t* parent, std::tuple<resolved_bindings_t...> resolved_bindings)
        : parent_{parent}, bindings_(std::move(resolved_bindings)), cache_{}
    {}

    [[no_unique_address]] parent_container_t* parent_;
    std::tuple<resolved_bindings_t...> bindings_;

    [[no_unique_address]] std::conditional_t<std::same_as<parent_container_t, void>, no_cache_t, instance_cache_t>
        cache_;
};

} // namespace dink
