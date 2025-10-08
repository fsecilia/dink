/*!
    \file
    Copyright (C) 2025 Frank Secilia
*/

#pragma once

#include <dink/lib.hpp>
#include <dink/bindings.hpp>
#include <dink/canonical.hpp>
#include <dink/instance_cache.hpp>
#include <dink/providers.hpp>
#include <dink/request_traits.hpp>
#include <dink/scopes.hpp>
#include <dink/type_list.hpp>
#include <memory>
#include <tuple>
#include <utility>

namespace dink {

// Forward declaration
template <typename policy_t, typename... bindings_t>
class container_t;

namespace policies {

// Nesting policies
namespace nesting {

struct no_parent_t
{};

template <typename parent_t>
struct child_t
{
    parent_t* parent;
    explicit child_t(parent_t& parent) : parent{&parent} {}
};

} // namespace nesting

// Scope resolution policies
namespace scope_resolution {

// Root container - uses Meyers singletons
struct static_t
{
    template <typename instance_t, typename dependency_chain_t, typename provider_t, typename container_t>
    auto resolve_singleton(provider_t& provider, container_t& container) -> instance_t&
    {
        static auto instance = provider.template create<dependency_chain_t>(container);
        return instance;
    }

    template <typename canonical_request_t>
    auto find_in_local_cache() -> canonical_request_t*
    {
        return nullptr;
    }
};

// Child container - uses instance cache
struct instance_cache_t
{
    dink::instance_cache_t cache;

    template <typename instance_t, typename dependency_chain_t, typename provider_t, typename container_t>
    auto resolve_singleton(provider_t& provider, container_t& container) -> std::shared_ptr<instance_t>
    {
        return cache.template get_or_create<instance_t>([&]() {
            return provider.template create<dependency_chain_t>(container);
        });
    }

    template <typename canonical_request_t>
    auto find_in_local_cache() -> std::shared_ptr<canonical_request_t>
    {
        return cache.template get<canonical_request_t>();
    }
};

} // namespace scope_resolution

} // namespace policies

// Container policy types
struct root_container_policy_t
{
    using nesting_t = policies::nesting::no_parent_t;
    using scope_resolution_t = policies::scope_resolution::static_t;
};

template <typename parent_t>
struct child_container_policy_t
{
    using nesting_t = policies::nesting::child_t<parent_t>;
    using scope_resolution_t = policies::scope_resolution::instance_cache_t;
};

// Main container implementation
template <typename policy_t, typename... bindings_t>
class container_t : private policy_t::nesting_t, private policy_t::scope_resolution_t
{
public:
    using nesting_policy_t = typename policy_t::nesting_t;
    using scope_policy_t = typename policy_t::scope_resolution_t;

    // Root constructor
    template <typename... binding_configs_t>
    requires std::same_as<nesting_policy_t, policies::nesting::no_parent_t>
    explicit container_t(binding_configs_t&&... configs)
        : bindings_{resolve_bindings(std::forward<binding_configs_t>(configs)...)}
    {}

    // Child constructor
    template <typename parent_t, typename... binding_configs_t>
    requires(!std::same_as<nesting_policy_t, policies::nesting::no_parent_t>)
    explicit container_t(parent_t& parent, binding_configs_t&&... configs)
        : nesting_policy_t(parent), bindings_{resolve_bindings(std::forward<binding_configs_t>(configs)...)}
    {}

    // Main resolution entry point
    template <typename request_t, typename dependency_chain_t = type_list_t<>>
    auto resolve() -> request_t
    {
        using canonical_request_t = canonical_t<request_t>;

        // Step 1: Check local cache
        if (auto cached = scope_policy_t::template find_in_local_cache<canonical_request_t>())
        {
            return as_requested<request_t>(std::move(cached));
        }

        // Step 2: Check local bindings
        if (auto binding = find_local_binding<canonical_request_t>())
        {
            if constexpr (!std::same_as<std::remove_pointer_t<decltype(binding)>, binding_not_found_t>)
            {
                return create_from_binding<request_t, dependency_chain_t>(*binding);
            }
        }

        // Step 3: Delegate to parent if exists
        if constexpr (!std::same_as<nesting_policy_t, policies::nesting::no_parent_t>)
        {
            return resolve_from_parent<request_t, dependency_chain_t>();
        }
        else
        {
            // Step 4: No cache, no binding, no parent - use default provider
            return create_from_default_provider<request_t, dependency_chain_t>();
        }
    }

private:
    struct binding_not_found_t
    {};

    // Check if we have a binding for this type locally
    template <typename canonical_request_t, std::size_t I = 0>
    auto find_local_binding() -> auto
    {
        if constexpr (I >= sizeof...(bindings_t)) { return static_cast<binding_not_found_t*>(nullptr); }
        else if constexpr (std::same_as<
                               canonical_request_t, typename std::tuple_element_t<I, decltype(bindings_)>::from_type>)
        {
            return &std::get<I>(bindings_);
        }
        else { return find_local_binding<canonical_request_t, I + 1>(); }
    }

    // Create instance from a binding
    template <typename request_t, typename dependency_chain_t, typename binding_t>
    auto create_from_binding(binding_t& binding) -> request_t
    {
        using resolved_request_t = resolved_t<request_t>;

        if constexpr (providers::is_accessor<typename binding_t::provider_type>)
        {
            // Accessor - just get the instance
            return as_requested<request_t>(binding.provider.get());
        }
        else
        {
            // Creator - check effective scope
            using binding_scope_t = typename binding_t::scope_type;
            using effective_scope_request_t = effective_scope_t<binding_scope_t, request_t>;

            if constexpr (std::same_as<effective_scope_request_t, scopes::transient_t>)
            {
                // Transient - create without caching
                return as_requested<request_t>(binding.provider.template create<dependency_chain_t>(*this));
            }
            else // singleton
            {
                // Singleton - resolve through scope policy (caches automatically)
                return as_requested<
                    request_t>(scope_policy_t::template resolve_singleton<resolved_request_t, dependency_chain_t>(
                    binding.provider, *this
                ));
            }
        }
    }

    // Create instance from default provider
    template <typename request_t, typename dependency_chain_t>
    auto create_from_default_provider() -> request_t
    {
        using resolved_request_t = resolved_t<request_t>;

        providers::creator_t<resolved_request_t> default_provider;

        // Determine effective scope (default binding is transient, but request might force singleton)
        using effective_scope_request_t = effective_scope_t<scopes::transient_t, request_t>;

        if constexpr (std::same_as<effective_scope_request_t, scopes::transient_t>)
        {
            // Transient - create without caching
            return as_requested<request_t>(default_provider.template create<dependency_chain_t>(*this));
        }
        else
        {
            // Singleton - resolve through scope policy
            return as_requested<
                request_t>(scope_policy_t::template resolve_singleton<resolved_request_t, dependency_chain_t>(
                default_provider, *this
            ));
        }
    }

    // Delegate resolution to parent container
    template <typename request_t, typename dependency_chain_t>
    requires(!std::same_as<nesting_policy_t, policies::nesting::no_parent_t>)
    auto resolve_from_parent() -> request_t
    {
        return nesting_policy_t::parent->template resolve<request_t, dependency_chain_t>();
    }

    std::tuple<bindings_t...> bindings_;
};

// Deduction guides
template <typename... binding_configs_t>
container_t(binding_configs_t&&...)
    -> container_t<root_container_policy_t, decltype(resolve_binding(std::declval<binding_configs_t>()))...>;

template <typename parent_t, typename... binding_configs_t>
container_t(parent_t&, binding_configs_t&&...)
    -> container_t<child_container_policy_t<parent_t>, decltype(resolve_binding(std::declval<binding_configs_t>()))...>;

// Type aliases
template <typename... bindings_t>
using root_container_t = container_t<root_container_policy_t, bindings_t...>;

template <typename parent_t, typename... bindings_t>
using child_container_t = container_t<child_container_policy_t<parent_t>, bindings_t...>;

} // namespace dink
