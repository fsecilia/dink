/*!
    \file
    Copyright (C) 2025 Frank Secilia
*/

#pragma once

#include <dink/lib.hpp>
#include <dink/bindings.hpp>
#include <concepts>
#include <memory>

namespace dink {

//! closure binding a provider to a specific container instance to produce a parameterless binding
template <typename provider_t, typename container_t>
struct bound_provider_t
{
    provider_t provider;
    container_t* container;

    template <typename instance_t>
    auto operator()() -> instance_t
    {
        return provider.template operator()<instance_t>(*container);
    }
};

// TODO: this should take the provider directly
template <typename instance_t, typename binding_t>
auto get_singleton(binding_t& binding) -> instance_t&
{
    static auto instance = binding.provider.template operator()<instance_t>();
    return instance;
}

struct root_container_tag_t
{};

struct child_container_tag_t
{};

template <typename instance_t>
struct child_slot_t
{
    std::shared_ptr<instance_t> instance;
};

// Helper: does this binding use static (process-wide) storage?
template <typename binding_p, typename container_tag_t>
constexpr bool uses_static_storage_v = std::same_as<typename binding_p::resolved_scope_t, scopes::singleton_t>
    || (std::same_as<typename binding_p::resolved_scope_t, scopes::scoped_t>
        && std::same_as<container_tag_t, root_container_tag_t>);

//! primary template - transient scope, no slot, no caching
template <typename binding_config_t, typename container_tag_t>
struct binding_with_scope_t : binding_config_t
{
};

// Specialization for bindings using static storage (singleton and root-scoped)
template <typename binding_config_t, typename container_tag_t>
requires uses_static_storage_v<binding_config_t, container_tag_t>
struct binding_with_scope_t<binding_config_t, container_tag_t> : binding_config_t
{
    auto get_or_create() -> binding_config_t::to_t&
    {
        return get_singleton<binding_config_t::to_t, binding_config_t>(*this);
    }
};

// Specialization for scoped scope in child - has local slot for zero-overhead lookups
template <typename binding_config_t>
requires std::same_as<typename binding_config_t::resolved_scope_t, scopes::scoped_t>
struct binding_with_scope_t<binding_config_t, child_container_tag_t> : binding_config_t
{
    child_slot_t<typename binding_config_t::to_t> slot;
};

template <typename binding_with_scope_t>
struct binding_t : binding_with_scope_t
{};

template <typename binding_t>
constexpr auto is_binding_builder_v = requires {
    typename binding_t::from_type;
    typename binding_t::to_type;
    typename binding_t::provider_type;
} && !requires { typename binding_t::resolved_scope; };

template <typename element_t>
auto finalize_binding(element_t&& element) noexcept -> auto
{
    if constexpr (is_binding_builder_v<std::remove_cvref_t<element_t>>)
    {
        using builder_t = std::remove_cvref_t<element_t>;
        return binding_config_t<
            typename builder_t::from_type, typename builder_t::to_type, typename builder_t::provider_type,
            scopes::default_t>{std::move(element.provider())};
    }
    else { return std::forward<element_t>(element); }
}

template <typename container_tag_t, typename finalized_binding_t>
auto add_scope_infrastructure(finalized_binding_t&& finalized_binding)
    -> binding_with_scope_t<finalized_binding_t, container_tag_t>
{
    return {std::move(finalized_binding)};
}

// Helper to determine if we need to close over the container
template <typename resolved_scope_t, typename container_tag_t>
constexpr bool needs_container_closure_v = std::same_as<resolved_scope_t, scopes::singleton_t>
    || (std::same_as<resolved_scope_t, scopes::scoped_t> && std::same_as<container_tag_t, root_container_tag_t>);

// Step 3: Close provider over container for singleton/root-scoped bindings
template <typename container_tag_t, typename prev_resolved_t, typename container_t>
auto close_provider_over_container(prev_resolved_t&& resolved_binding, container_t& container) -> auto
{
    using prev_binding_t = typename prev_resolved_t::binding_t;
    using resolved_scope_t = typename prev_binding_t::resolved_scope_t;

    if constexpr (needs_container_closure_v<resolved_scope_t, container_tag_t>)
    {
        using prev_provider_t = typename prev_binding_t::provider_t;

        return binding_t<binding_config_t<
            typename prev_binding_t::from_t, typename prev_binding_t::to_t,
            bound_provider_t<prev_provider_t, container_t>, resolved_scope_t>>{binding_config_t{
            bound_provider_t<prev_provider_t, container_t>{std::move(resolved_binding.binding.provider), &container}
        }};
    }
    else { return std::forward<prev_resolved_t>(resolved_binding); }
}

// Three-phase transform: builder -> finalized -> resolved -> closed
template <typename container_tag_t, typename element_t, typename container_t>
auto resolve_binding(element_t&& element, container_t& container) -> auto
{
    // Phase 1: Complete partial bindings (builder -> binding_config_t)
    auto finalized = finalize_binding(std::forward<element_t>(element));

    // Phase 2: Add scope infrastructure (binding_config_t -> binding_with_scope_t)
    auto with_scope = add_scope_infrastructure<container_tag_t>(std::move(finalized));

    // Phase 3: Close provider over container if needed (for singleton/root-scoped) (binding_with_scope_t -> binding_t)
    return close_provider_over_container<container_tag_t>(std::move(with_scope), container);
}

template <typename container_tag_t, typename... bindings_t, typename container_t>
auto resolve_bindings(container_t& container, bindings_t&&... bindings)
{
    return std::make_tuple(resolve_binding<container_tag_t>(std::forward<bindings_t>(bindings), container)...);
}

} // namespace dink
