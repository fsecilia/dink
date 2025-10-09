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

namespace container::strategies {

struct root_t
{
    template <typename instance_t, typename dependency_chain_t, typename provider_t, typename container_t>
    auto resolve_singleton(provider_t& provider, container_t& container) -> instance_t*
    {
        static auto instance = provider.template create<dependency_chain_t>(container);
        return &instance;
    }

    template <typename canonical_request_t>
    auto find_in_local_cache() -> canonical_request_t*
    {
        return nullptr;
    }
};

template <typename parent_t>
struct nested_t
{
    parent_t* parent;
    explicit nested_t(parent_t& parent) : parent{&parent} {}

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

} // namespace container::strategies

template <typename strategy_t, typename... bindings_t>
class container_t : private strategy_t
{
public:
    inline static constexpr auto is_root = std::same_as<strategy_t, container::strategies::root_t>;

    // root constructor
    template <typename... binding_configs_t>
    requires(is_root)
    explicit container_t(binding_configs_t&&... configs)
        : bindings_{resolve_bindings(std::forward<binding_configs_t>(configs)...)}
    {}

    // nested constructor
    template <typename parent_t, typename... binding_configs_t>
    requires(!is_root)
    explicit container_t(parent_t& parent, binding_configs_t&&... configs)
        : strategy_t{parent}, bindings_{resolve_bindings(std::forward<binding_configs_t>(configs)...)}
    {}

    template <typename request_t, typename dependency_chain_t = type_list_t<>>
    auto resolve() -> returned_t<request_t>
    {
        return try_resolve<request_t, dependency_chain_t>([this]() -> decltype(auto) {
            return create_from_default_provider<request_t, dependency_chain_t>();
        });
    }

private:
    struct binding_not_found_t
    {};

    // Compute binding index at compile time
    template <typename T>
    static constexpr std::size_t compute_binding_index()
    {
        if constexpr (sizeof...(bindings_t) == 0) { return static_cast<std::size_t>(-1); }
        else
        {
            return []<std::size_t... Is>(std::index_sequence<Is...>) consteval {
                constexpr std::size_t not_found = static_cast<std::size_t>(-1);

                // Build array of matches
                constexpr bool matches[]
                    = {std::same_as<T, typename std::tuple_element_t<Is, std::tuple<bindings_t...>>::from_type>...};

                // Find first match
                for (std::size_t i = 0; i < sizeof...(Is); ++i)
                {
                    if (matches[i]) return i;
                }
                return not_found;
            }(std::index_sequence_for<bindings_t...>{});
        }
    }

    template <typename T>
    static constexpr std::size_t binding_index_v = compute_binding_index<T>();

    // Direct lookup using pre-computed index
    template <typename canonical_request_t>
    auto find_local_binding() -> auto
    {
        constexpr auto index = binding_index_v<canonical_request_t>;
        if constexpr (index != static_cast<std::size_t>(-1)) { return &std::get<index>(bindings_); }
        else { return static_cast<binding_not_found_t*>(nullptr); }
    }

    // create instance from a binding
    template <typename request_t, typename dependency_chain_t, typename binding_t>
    auto create_from_binding(binding_t& binding) -> returned_t<request_t>
    {
        using resolved_request_t = resolved_t<request_t>;

        if constexpr (providers::is_accessor<typename binding_t::provider_type>)
        {
            // accessor - just get the instance
            return as_requested<request_t>(binding.provider.get());
        }
        else
        {
            // creator - check effective scope
            using binding_scope_t = typename binding_t::scope_type;
            using effective_scope_request_t = effective_scope_t<binding_scope_t, request_t>;

            if constexpr (std::same_as<effective_scope_request_t, scopes::transient_t>)
            {
                // transient - create without caching
                return as_requested<request_t>(binding.provider.template create<dependency_chain_t>(*this));
            }
            else
            {
                // singleton - resolve through scope strategy (caches automatically)
                return as_requested<
                    request_t>(strategy_t::template resolve_singleton<resolved_request_t, dependency_chain_t>(
                    binding.provider, *this
                ));
            }
        }
    }

    // create instance from default provider
    template <typename request_t, typename dependency_chain_t>
    auto create_from_default_provider() -> returned_t<request_t>
    {
        using resolved_request_t = resolved_t<request_t>;

        providers::creator_t<resolved_request_t> default_provider;

        // determine effective scope (default binding is transient, but request might force singleton)
        using effective_scope_request_t = effective_scope_t<scopes::transient_t, request_t>;

        if constexpr (std::same_as<effective_scope_request_t, scopes::transient_t>)
        {
            // transient - create without caching
            return as_requested<request_t>(default_provider.template create<dependency_chain_t>(*this));
        }
        else
        {
            // singleton - resolve through scope strategy
            return as_requested<request_t>(
                strategy_t::template resolve_singleton<resolved_request_t, dependency_chain_t>(default_provider, *this)
            );
        }
    }

    template <typename request_t, typename dependency_chain_t, typename factory_t>
    requires(!is_root)
    auto resolve_from_parent(factory_t&& factory) -> returned_t<request_t>
    {
        return strategy_t::parent->template try_resolve<request_t, dependency_chain_t>(
            std::forward<factory_t>(factory)
        );
    }

    std::tuple<bindings_t...> bindings_;

public:
    template <typename request_t, typename dependency_chain_t, typename factory_t>
    auto try_resolve(factory_t&& factory) -> returned_t<request_t>
    {
        using canonical_t = canonical_t<request_t>;

        // check local cache for for singleton-scoped requests
        if constexpr (request_traits_f<canonical_t>::transitive_scope == transitive_scope_t::singleton)
        {
            if (auto cached = strategy_t::template find_in_local_cache<canonical_t>())
            {
                return as_requested<request_t>(std::move(cached));
            }
        }

        // check local bindings
        if (auto binding = find_local_binding<canonical_t>())
        {
            using binding_t = std::remove_pointer_t<decltype(binding)>;
            static constexpr auto binding_found = !std::same_as<binding_t, binding_not_found_t>;
            if constexpr (binding_found) return create_from_binding<request_t, dependency_chain_t>(*binding);
        }

        // delegate to parent if exists
        if constexpr (!is_root)
        {
            return resolve_from_parent<request_t, dependency_chain_t>(std::forward<factory_t>(factory));
        }

        // Not found anywhere - invoke the factory from the original caller
        return factory();
    }
};

// deduction guides
template <typename... binding_configs_t>
container_t(binding_configs_t&&...)
    -> container_t<container::strategies::root_t, decltype(resolve_binding(std::declval<binding_configs_t>()))...>;

template <typename parent_t, typename... binding_configs_t>
container_t(parent_t&, binding_configs_t&&...) -> container_t<
    container::strategies::nested_t<parent_t>, decltype(resolve_binding(std::declval<binding_configs_t>()))...>;

// type aliases
template <typename... bindings_t>
using root_container_t = container_t<container::strategies::root_t, bindings_t...>;

template <typename parent_t, typename... bindings_t>
using child_container_t = container_t<container::strategies::nested_t<parent_t>, bindings_t...>;

} // namespace dink
