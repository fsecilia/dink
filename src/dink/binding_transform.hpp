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

template <typename instance_t, typename binding_t>
auto get_singleton(binding_t& binding) -> instance_t&
{
    static auto instance = binding.provider.template operator()<instance_t>();
    return instance;
}

struct root_container_tag_t;
struct child_container_tag_t;

template <typename instance_t>
struct child_slot_t
{
    std::shared_ptr<instance_t> instance;
};

//! primary template - transient scope, no slot
template <typename binding_p, typename container_tag_t>
struct resolved_binding_t
{
    using binding_t = binding_p;
    binding_t binding;
};

// specialization for singleton scope - uses Meyers singleton
template <typename binding_p, typename container_tag_t>
requires std::same_as<typename binding_p::resolved_scope, scopes::singleton_t>
struct resolved_binding_t<binding_p, container_tag_t>
{
    using binding_t = binding_p;
    using to_t = typename binding_t::to_t;

    binding_t binding;

    auto get_or_create() -> to_t& { return get_singleton<to_t, binding_t>(binding); }
};

// specialization for scoped scope in root - acts like singleton
template <typename binding_p>
requires std::same_as<typename binding_p::resolved_scope, scopes::scoped_t>
struct resolved_binding_t<binding_p, root_container_tag_t>
{
    using binding_t = binding_p;
    using to_t = typename binding_t::to_t;

    binding_t binding;

    auto get_or_create() -> to_t& { return get_singleton<to_t, binding_t>(binding); }
};

// specialization for scoped scope in child - has local slot to store instance for zero-overhead lookups
template <typename binding_p>
requires std::same_as<typename binding_p::resolved_scope, scopes::scoped_t>
struct resolved_binding_t<binding_p, child_container_tag_t>
{
    using binding_t = binding_p;
    using to_t = typename binding_t::to_t;

    binding_t binding;
    child_slot_t<to_t> slot;
};

template <typename element_t>
auto finalize_binding(element_t&& element) noexcept -> auto
{
    if constexpr (is_binding_builder_v<std::remove_cvref_t<element_t>>)
    {
        using builder_t = std::remove_cvref_t<element_t>;
        return binding_t<
            typename builder_t::from_type, typename builder_t::to_type, typename builder_t::provider_type,
            scopes::default_t>{std::move(element.provider())};
    }
    else { return std::forward<element_t>(element); }
}

template <typename container_tag_t, typename prev_resolved_t, typename container_t>
auto bind_to_container(container_t& container, prev_resolved_t&& resolved_binding) -> auto
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
            bound_provider_t<prev_provider_t, container_t>{std::move(resolved_binding.binding.provider), &container}
        }};
    }
    else { return std::forward<prev_resolved_t>(resolved_binding); }
}

template <typename container_tag_t, typename element_t, typename container_t>
auto resolve_binding(element_t&& element, container_t& container) -> auto
{
    // transform 1: finalize partial bindings (binding_target -> binding)
    auto finalized = finalize_binding(std::forward<element_t>(element));

    // transform 2: add scope infrastructure (binding -> resolved_binding)
    auto resolved_binding = resolved_binding_t<decltype(finalized), container_tag_t>{std::move(finalized)};

    // transform 3: bind provider to container_t (if needed for singleton/root-scoped)
    return bind_to_container<container_tag_t>(std::move(resolved_binding), container);
}

template <typename container_tag_t, typename... bindings_t, typename container_t>
auto resolve_bindings(bindings_t&&... bindings, container_t& container)
{
    return std::make_tuple(resolve_binding<container_tag_t>(std::forward<bindings_t>(bindings))..., container);
}

} // namespace dink
