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

template <typename T, typename RootContainerType>
struct root_promotion_provider
{
    RootContainerType* root;

    auto operator()() const -> T { return root->template create_as_transient<T>(); }
};

template <typename T, typename Provider>
std::shared_ptr<T>& get_promoted_singleton(Provider& provider)
{
    static std::shared_ptr<T> instance;
    if (!instance) { instance = std::make_shared<T>(provider()); }
    return instance;
}

template <typename ParentContainer, typename... ResolvedBindings>
class container
{
    [[no_unique_address]] ParentContainer* parent_;
    std::tuple<ResolvedBindings...> bindings_;

    [[no_unique_address]] std::conditional_t<std::same_as<ParentContainer, void>, no_cache_t, instance_cache_t> cache_;

    explicit container(std::tuple<ResolvedBindings...> resolved_bindings)
        : parent_(nullptr), bindings_(std::move(resolved_bindings)), cache_()
    {}

    template <typename Parent>
    container(Parent& parent, std::tuple<ResolvedBindings...> resolved_bindings)
        : parent_(&parent), bindings_(std::move(resolved_bindings)), cache_()
    {}

public:
    template <typename... Bindings>
    explicit container(Bindings&&... bindings)
        : container(finalize_and_decorate<root_container_tag_t>(std::forward<Bindings>(bindings)...))
    {}

    template <typename Parent, typename... Bindings>
    container(Parent& parent, Bindings&&... bindings)
        : container(parent, finalize_and_decorate<child_container_tag_t>(std::forward<Bindings>(bindings)...))
    {}

    template <typename RequestType>
    auto resolve()
    {
        using value_type = typename request_traits_t<RequestType>::value_type;

        auto [binding_ptr, slot_ptr, configured_scope] = find_in_chain<value_type>();

        // Accessor path - just call operator()
        if (binding_ptr
            && providers::is_accessor<
                typename std::remove_pointer_t<decltype(binding_ptr)>::binding_type::provider_type>)
        {
            return adapt_to_request_type<RequestType>(binding_ptr->binding.provider());
        }

        // Creator path - scope-based resolution
        using eff_scope = typename effective_scope_f<decltype(configured_scope), RequestType>::type;

        if constexpr (std::same_as<eff_scope, scopes::transient_t>)
        {
            return resolve_transient<RequestType, value_type>();
        }
        else if constexpr (std::same_as<eff_scope, scopes::singleton_t>)
        {
            return resolve_singleton<RequestType>(binding_ptr);
        }
        else { return resolve_scoped<RequestType, value_type>(binding_ptr, slot_ptr); }
    }

    template <typename T>
    T create_as_transient()
    {
        auto [binding_ptr, slot_ptr, configured_scope] = find_in_chain<T>();

        if (binding_ptr) { return binding_ptr->binding.provider(*this); }
        else { return auto_wire<T>(*this); }
    }

private:
    // Combined transform: finalize + decorate + bind to container
    template <typename ContainerTag, typename... Bindings>
    auto finalize_and_decorate(Bindings&&... bindings)
    {
        return std::make_tuple(cook_binding<ContainerTag>(std::forward<Bindings>(bindings))...);
    }

    template <typename ContainerTag, typename BuilderOrBinding>
    auto cook_binding(BuilderOrBinding&& item)
    {
        // Transform 1: Finalize partial bindings (binding_target → binding)
        auto finalized = finalize_binding(std::forward<BuilderOrBinding>(item));

        // Transform 2: Add scope infrastructure (binding → resolved_binding)
        auto resolved = resolved_binding_t<decltype(finalized), ContainerTag>{std::move(finalized)};

        // Transform 3: Bind provider to container (if needed for singleton/root-scoped)
        return bind_to_container<ContainerTag>(std::move(resolved));
    }

    template <typename ContainerTag, typename Resolved>
    auto bind_to_container(Resolved&& r)
    {
        using binding_type = typename Resolved::binding_type;
        using resolved_scope_type = typename binding_type::resolved_scope;

        if constexpr (std::same_as<resolved_scope_type, scopes::singleton_t>
                      || (std::same_as<resolved_scope_type, scopes::scoped_t>
                          && std::same_as<ContainerTag, root_container_tag_t>))
        {
            using provider_type = typename binding_type::provider_type;

            return resolved_binding_t<
                binding_t<
                    typename binding_type::from_type, typename binding_type::to_type,
                    bound_provider_t<provider_type, container>, resolved_scope_type>,
                ContainerTag>{
                binding_t{bound_provider_t<provider_type, container>{std::move(r.binding.provider), this}}
            };
        }
        else { return std::forward<Resolved>(r); }
    }

    template <typename RequestType, typename T>
    auto resolve_transient()
    {
        auto instance = create_as_transient<T>();
        return adapt_to_request_type<RequestType>(std::move(instance));
    }

    template <typename RequestType, typename BindingPtr>
    auto resolve_singleton(BindingPtr binding_ptr)
    {
        return adapt_to_request_type<RequestType>(binding_ptr->get_or_create());
    }

    template <typename RequestType, typename T, typename BindingPtr, typename SlotPtr>
    auto resolve_scoped(BindingPtr binding_ptr, SlotPtr slot_ptr)
    {
        if (slot_ptr && slot_ptr->instance) { return adapt_to_request_type<RequestType>(*slot_ptr->instance); }

        if (slot_ptr) { return resolve_scoped_with_slot<RequestType, T>(binding_ptr, slot_ptr); }

        if constexpr (std::same_as<ParentContainer, void>) { return resolve_scoped_in_root<RequestType, T>(); }
        else { return resolve_scoped_in_child<RequestType, T>(); }
    }

    template <typename RequestType, typename T, typename BindingPtr, typename SlotPtr>
    auto resolve_scoped_with_slot(BindingPtr binding_ptr, SlotPtr slot_ptr)
    {
        slot_ptr->instance = std::make_shared<T>(binding_ptr->binding.provider(*this));
        return adapt_to_request_type<RequestType>(*slot_ptr->instance);
    }

    template <typename RequestType, typename T>
    auto resolve_scoped_in_root()
    {
        root_promotion_provider<T, decltype(*this)> provider{this};
        auto instance = get_promoted_singleton<T, decltype(provider)>(provider);
        return adapt_to_request_type<RequestType>(*instance);
    }

    template <typename RequestType, typename T>
    auto resolve_scoped_in_child()
    {
        if (auto cached = find_in_cache_chain<T>()) { return adapt_to_request_type<RequestType>(*cached); }

        auto instance = cache_.template get_or_create<T>([&]() { return create_as_transient<T>(); });

        return adapt_to_request_type<RequestType>(*instance);
    }

    template <typename From>
    auto find_in_chain()
    {
        if constexpr (has_binding<From, decltype(bindings_)>())
        {
            auto& decorated = find_binding<From>(bindings_);
            return std::make_tuple(
                &decorated, get_slot_ptr(decorated), typename decltype(decorated)::binding_type::resolved_scope{}
            );
        }
        else if constexpr (!std::same_as<ParentContainer, void>)
        {
            if (parent_) { return parent_->template find_in_chain<From>(); }
        }

        using no_binding_ptr = std::remove_pointer_t<
            decltype(&std::declval<resolved_binding_t<
                         binding_t<From, From, providers::ctor_invoker_t, scopes::transient_t>,
                         root_container_tag_t>>())>*;
        using no_slot_ptr = decltype(get_slot_ptr(
            std::declval<resolved_binding_t<
                binding_t<From, From, providers::ctor_invoker_t, scopes::transient_t>, root_container_tag_t>>()
        ));
        return std::make_tuple(
            static_cast<no_binding_ptr>(nullptr), static_cast<no_slot_ptr>(nullptr), scopes::transient_t{}
        );
    }

    template <typename T>
    std::shared_ptr<T> find_in_cache_chain()
    {
        if constexpr (!std::same_as<ParentContainer, void>)
        {
            if (auto cached = cache_.template get<T>()) { return cached; }
        }

        if constexpr (!std::same_as<ParentContainer, void>)
        {
            if (parent_) { return parent_->template find_in_cache_chain<T>(); }
        }

        return nullptr;
    }

    template <typename Decorated>
    auto get_slot_ptr(Decorated& d)
    {
        if constexpr (requires { d.slot; }) { return &d.slot; }
        else { return static_cast<decltype(&d.slot)>(nullptr); }
    }

    template <typename From, typename Tuple>
    static constexpr bool has_binding();

    template <typename From, typename Tuple>
    static constexpr auto& find_binding(Tuple& t);

    template <typename RequestType, typename Instance>
    static auto adapt_to_request_type(Instance&& instance)
    {
        using value_type = typename request_traits_t<RequestType>::value_type;

        if constexpr (std::same_as<RequestType, value_type>)
        {
            // T - return by value
            return std::forward<Instance>(instance);
        }
        else if constexpr (std::same_as<RequestType, value_type&>)
        {
            // T& - return lvalue reference
            return static_cast<value_type&>(instance);
        }
        else if constexpr (std::same_as<RequestType, value_type&&>)
        {
            // T&& - return rvalue reference
            return std::move(instance);
        }
        else if constexpr (std::same_as<RequestType, value_type*>)
        {
            // T* - return pointer
            return &instance;
        }
        else if constexpr (std::same_as<RequestType, std::unique_ptr<value_type>>)
        {
            // unique_ptr<T> - wrap in unique_ptr
            return std::make_unique<value_type>(std::forward<Instance>(instance));
        }
        else if constexpr (std::same_as<RequestType, std::shared_ptr<value_type>>)
        {
            // shared_ptr<T> - if instance is already shared_ptr, return it; otherwise wrap
            if constexpr (std::same_as<std::remove_cvref_t<Instance>, std::shared_ptr<value_type>>) { return instance; }
            else { return std::make_shared<value_type>(std::forward<Instance>(instance)); }
        }
        else if constexpr (std::same_as<RequestType, std::weak_ptr<value_type>>)
        {
            // weak_ptr<T> - instance must already be shared_ptr
            static_assert(
                std::same_as<std::remove_cvref_t<Instance>, std::shared_ptr<value_type>>,
                "weak_ptr can only be created from shared_ptr"
            );
            return std::weak_ptr<value_type>(instance);
        }
    }

    template <typename T, typename Container>
    static T auto_wire(Container& c);
};

} // namespace dink
