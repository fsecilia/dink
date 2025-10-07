/*!
    \file
    Copyright (C) 2025 Frank Secilia
*/

#pragma once

#include <dink/lib.hpp>
#include <dink/bindings.hpp>
#include <concepts>
#include <memory>
#include <optional>

namespace dink {

template <typename value_t>
struct singleton_t
{
    static std::optional<value_t> value;

    template <typename provider_t, typename container_t>
    auto get_or_create(provider_t
};

template <typename instance_t, typename provider_t, typename root_container_t>
auto get_or_create_singleton(provider_t& provider, root_container_t& root_continer) -> instance_t&
{
    static auto instance = provider.template operator()<instance_t>(root_container);
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
struct binding_t
{
    using provider_t = binding_config_t::provider_t;

    binding_config_t config;
};

// Specialization for bindings using static storage (singleton and root-scoped)
template <typename binding_config_t, typename container_tag_t>
requires uses_static_storage_v<binding_config_t, container_tag_t>
struct binding_t<binding_config_t, container_tag_t>
{
    using provider_t = binding_config_t::provider_t;

    binding_config_t config;

    template <typename root_container_t>
    auto get_or_create(root_container_t& root_container) -> binding_config_t::to_t&
    {
        return get_or_create_singleton<binding_config_t::to_t, provider_t>(config.provider, root_container);
    }
};

// Specialization for scoped scope in child - has local slot for zero-overhead lookups
template <typename binding_config_t>
requires std::same_as<typename binding_config_t::resolved_scope_t, scopes::scoped_t>
struct binding_t<binding_config_t, child_container_tag_t>
{
    using provider_t = binding_config_t::provider_t;
    binding_config_t config;
    child_slot_t<typename binding_config_t::to_t> slot;
};

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
    -> binding_t<finalized_binding_t, container_tag_t>
{
    return {std::move(finalized_binding)};
}

// two-phase transform: builder -> finalized -> resolved
template <typename container_tag_t, typename element_t, typename container_t>
auto resolve_binding(element_t&& element, container_t& container) -> auto
{
    // complete partial bindings into finalized bindings
    auto finalized = finalize_binding(std::forward<element_t>(element));

    // add scope infrastructure (binding_config_t -> binding_t)
    return add_scope_infrastructure<container_tag_t>(std::move(finalized));
}

template <typename container_tag_t, typename... bindings_t, typename container_t>
auto resolve_bindings(container_t& container, bindings_t&&... bindings)
{
    return std::make_tuple(resolve_binding<container_tag_t>(std::forward<bindings_t>(bindings), container)...);
}

} // namespace dink
