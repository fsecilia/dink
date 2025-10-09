/*!
    \file
    Copyright (C) 2025 Frank Secilia
*/

#pragma once

#include <dink/lib.hpp>
#include <dink/bindings.hpp>
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
    template <typename instance_t, typename bound_initializer_t>
    static auto untyped_initializer(void* bound_initializer) -> instance_t
    {
        return (*static_cast<bound_initializer_t*>(bound_initializer))();
    }

    template <typename instance_t>
    static auto singleton_storage(instance_t (*untyped_initializer)(void*), void* bound_initializer) -> instance_t&
    {
        static auto singleton = untyped_initializer(bound_initializer);
        return singleton;
    }

    template <typename instance_t, typename bound_initializer_t>
    static auto singleton_storage(bound_initializer_t&& bound_initializer) -> instance_t&
    {
        return singleton_storage<instance_t>(&untyped_initializer<instance_t, bound_initializer_t>, &bound_initializer);
    }

    template <typename instance_t, typename dependency_chain_t, typename provider_t, typename container_t>
    auto resolve_singleton(provider_t& provider, container_t& container) -> instance_t&
    {
        return singleton_storage<instance_t>([&]() { return provider.template create<dependency_chain_t>(container); });
    }

    template <typename instance_t, typename dependency_chain_t, typename provider_t, typename container_t>
    auto resolve_shared_ptr(provider_t& provider, container_t& container) -> std::shared_ptr<instance_t>&
    {
        static auto shared = std::shared_ptr<instance_t>{
            &resolve_singleton<instance_t, dependency_chain_t>(provider, container), [](auto&&) {}
        };
        return shared;
    }

    template <typename instance_t>
    auto find_in_local_cache() -> instance_t*
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

    template <typename instance_t, typename dependency_chain_t, typename provider_t, typename container_t>
    auto resolve_shared_ptr(provider_t& provider, container_t& container) -> std::shared_ptr<instance_t>
    {
        // the cache already stores shared_ptrs, so just return that directly
        return resolve_singleton<instance_t, dependency_chain_t>(provider, container);
    }

    template <typename resolved_t>
    auto find_in_local_cache() -> std::shared_ptr<resolved_t>
    {
        return cache.template get<resolved_t>();
    }
};

} // namespace container::strategies

namespace detail {

template <typename>
struct is_binding_config_f : std::false_type
{};

template <typename from_p, typename to_p, typename scope_p, typename provider_p>
struct is_binding_config_f<binding_config_t<from_p, to_p, scope_p, provider_p>> : std::true_type
{};

template <typename from_p, typename to_p, typename provider_p>
struct is_binding_config_f<binding_dst_t<from_p, to_p, provider_p>> : std::true_type
{};

} // namespace detail

template <typename T> concept is_binding_config = detail::is_binding_config_f<std::decay_t<T>>::value;

struct binding_not_found_t
{};

// Manages finding a binding for a given type
template <typename... bindings_t>
class binding_locator_t
{
public:
    explicit binding_locator_t(std::tuple<bindings_t...> bindings) : bindings_{std::move(bindings)} {}

    template <typename... args_t>
    explicit binding_locator_t(args_t&&... args) : bindings_{std::forward<args_t>(args)...}
    {}

    template <typename resolved_t>
    auto find_binding() -> auto
    {
        constexpr auto index = binding_index_v<resolved_t>;
        if constexpr (index != static_cast<std::size_t>(-1)) { return &std::get<index>(bindings_); }
        else { return static_cast<binding_not_found_t*>(nullptr); }
    }

private:
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

    std::tuple<bindings_t...> bindings_;
};

template <typename... bindings_t>
binding_locator_t(bindings_t&&...) -> binding_locator_t<std::decay_t<bindings_t>...>;

template <typename strategy_t, typename... bindings_t>
class container_t;

// --- Concept to identify a container (avoids ambiguity)
namespace detail {

template <typename>
struct is_container_f : std::false_type
{};

template <typename strategy_p, typename... bindings_p>
struct is_container_f<container_t<strategy_p, bindings_p...>> : std::true_type
{};

} // namespace detail

// Creates an instance from a provider, handling scope and caching.
class instance_creator_t
{
public:
    // create instance from a binding
    template <typename request_t, typename dependency_chain_t, typename binding_t, typename container_p>
    auto create_from_binding(binding_t& binding, container_p& container) -> returned_t<request_t>
    {
        if constexpr (providers::is_accessor<typename binding_t::provider_type>)
        {
            // accessor - just get the instance
            return as_requested<request_t>(binding.provider.get());
        }
        else
        {
            // creator - check effective scope
            using binding_scope_t = typename binding_t::scope_type;
            using effective_scope_t = effective_scope_t<binding_scope_t, request_t>;
            return execute_provider<request_t, dependency_chain_t, effective_scope_t>(binding.provider, container);
        }
    }

    // create instance from default provider
    template <typename request_t, typename dependency_chain_t, typename container_p>
    auto create_from_default_provider(container_p& container) -> returned_t<request_t>
    {
        using resolved_t = resolved_t<request_t>;

        providers::creator_t<resolved_t> default_provider;
        using effective_scope_t = effective_scope_t<scopes::transient_t, request_t>;
        return execute_provider<request_t, dependency_chain_t, effective_scope_t>(default_provider, container);
    }

private:
    template <
        typename request_t, typename dependency_chain_t, typename scope_t, typename provider_t, typename container_p>
    auto execute_provider(provider_t& provider, container_p& container) -> returned_t<request_t>
    {
        using provided_t = typename provider_t::provided_t;

        // --- BRANCH ON REQUEST TYPE ---
        if constexpr (detail::is_shared_ptr_v<request_t> || detail::is_weak_ptr_v<request_t>)
        {
            // --- LOGIC FOR SHARED POINTERS (P4, P5, P7) ---
            if constexpr (std::same_as<scope_t, scopes::singleton_t>)
            {
                // This is a singleton request, so use the strategy's shared_ptr resolver.
                // This correctly handles caching the canonical shared_ptr.
                return as_requested<request_t>(
                    container.template resolve_shared_ptr<provided_t, dependency_chain_t>(provider, container)
                );
            }
            else // transient scope
            {
                // create a new transient shared_ptr.
                return as_requested<request_t>(
                    std::shared_ptr<provided_t>{new provided_t{provider.template create<dependency_chain_t>(container)}}
                );
            }
        }
        else
        {
            static_assert(std::same_as<scope_t, effective_scope_t<scope_t, request_t>>);

            // --- EXISTING LOGIC FOR OTHER TYPES (P1, P2, P3, P6) ---

            if constexpr (std::same_as<scope_t, scopes::singleton_t>)
            {
                // Resolve through strategy (caches automatically)
                return as_requested<request_t>(
                    container.template resolve_singleton<provided_t, dependency_chain_t>(provider, container)
                );
            }
            else // It's a transient scope
            {
                // Create without caching
                return as_requested<request_t>(provider.template create<dependency_chain_t>(container));
            }
        }
    }
};

template <typename T> concept is_container = detail::is_container_f<std::decay_t<T>>::value;

template <typename strategy_t, typename... bindings_t>
class container_t : public strategy_t
{
public:
    inline static constexpr auto is_root = std::same_as<strategy_t, container::strategies::root_t>;

    // root constructor
    template <typename... binding_configs_t>
    requires((is_binding_config<binding_configs_t> && ...))
    explicit container_t(binding_configs_t&&... configs)
        : binding_locator_{resolve_bindings(std::forward<binding_configs_t>(configs)...)}
    {}

    // nested constructor
    template <typename parent_t, typename... binding_configs_t>
    requires(is_container<parent_t> && ((is_binding_config<binding_configs_t> && ...)))
    explicit container_t(parent_t& parent, binding_configs_t&&... configs)
        : strategy_t{parent}, binding_locator_{resolve_bindings(std::forward<binding_configs_t>(configs)...)}
    {}

    template <typename request_t, typename dependency_chain_t = type_list_t<>>
    auto resolve() -> returned_t<request_t>
    {
        // run search for instance or bindings up the chain, or use the default provider with this container.
        return try_resolve<request_t, dependency_chain_t>([this]() -> decltype(auto) {
            return instance_creator_.template create_from_default_provider<request_t, dependency_chain_t>(*this);
        });
    }

    template <typename request_t, typename dependency_chain_t, typename factory_t>
    auto try_resolve(factory_t&& factory) -> returned_t<request_t>
    {
        using resolved_t = resolved_t<request_t>;

        // check local cache for for singleton-scoped requests
        if constexpr (request_traits_f<resolved_t>::transitive_scope == transitive_scope_t::singleton)
        {
            if (auto cached = strategy_t::template find_in_local_cache<resolved_t>())
            {
                return as_requested<request_t>(std::move(cached));
            }
        }

        // check local bindings and create if found
        if (auto binding = binding_locator_.template find_binding<resolved_t>())
        {
            using binding_t = std::remove_pointer_t<decltype(binding)>;
            static constexpr auto binding_found = !std::same_as<binding_t, binding_not_found_t>;
            if constexpr (binding_found)
            {
                return instance_creator_.template create_from_binding<request_t, dependency_chain_t>(*binding, *this);
            }
        }

        // delegate to parent if exists
        if constexpr (!is_root)
        {
            return strategy_t::parent->template try_resolve<request_t, dependency_chain_t>(
                std::forward<factory_t>(factory)
            );
        }

        // Not found anywhere - invoke the factory from the original caller
        return factory();
    }

private:
    instance_creator_t instance_creator_{};
    binding_locator_t<bindings_t...> binding_locator_{};
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
