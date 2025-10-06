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

struct no_cache_t;
struct root_container_tag_t;
struct child_container_tag_t;

template <typename instance_t>
struct child_slot_t
{
    std::shared_ptr<instance_t> instance;
};

//! primary template - transient scope, no slot
template <typename binding_p, typename ContainerTag>
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

} // namespace dink
