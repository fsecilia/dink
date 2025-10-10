/*!
    \file
    Copyright (C) 2025 Frank Secilia
*/

#pragma once

#include <dink/lib.hpp>
#include <dink/bindings.hpp>
#include <dink/instance_cache.hpp>
#include <dink/lifecycle.hpp>
#include <dink/providers.hpp>
#include <dink/request_traits.hpp>
#include <dink/type_list.hpp>
#include <memory>
#include <tuple>
#include <utility>

namespace dink {

namespace container::scope {

struct global_t
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

} // namespace container::scope

namespace detail {

template <typename>
struct is_binding_f : std::false_type
{};

template <typename from_p, typename to_p, typename lifecycle_p, typename provider_p>
struct is_binding_f<binding_t<from_p, to_p, lifecycle_p, provider_p>> : std::true_type
{};

template <typename from_p, typename to_p, typename provider_p>
struct is_binding_f<binding_dst_t<from_p, to_p, provider_p>> : std::true_type
{};

} // namespace detail

template <typename T> concept is_binding = detail::is_binding_f<std::decay_t<T>>::value;

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

template <typename scope_t, typename binding_locator_t, typename instance_creator_t>
class container_t;

// --- Concept to identify a container (avoids ambiguity)
namespace detail {

template <typename>
struct is_container_f : std::false_type
{};

template <typename scope_t, typename binding_locator_t, typename instance_creator_t>
struct is_container_f<container_t<scope_t, binding_locator_t, instance_creator_t>> : std::true_type
{};

} // namespace detail

template <typename T> concept is_container = detail::is_container_f<std::decay_t<T>>::value;

// Creates an instance from a provider, handling lifecycle and caching.
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
            // creator - check effective lifecycle
            using binding_lifecycle_t = typename binding_t::lifecycle_type;
            using effective_lifecycle_t = effective_lifecycle_t<binding_lifecycle_t, request_t>;
            return execute_provider<request_t, dependency_chain_t, effective_lifecycle_t>(binding.provider, container);
        }
    }

    // create instance from default provider
    template <typename request_t, typename dependency_chain_t, typename container_p>
    auto create_from_default_provider(container_p& container) -> returned_t<request_t>
    {
        using resolved_t = resolved_t<request_t>;

        providers::creator_t<resolved_t> default_provider;
        using effective_lifecycle_t = effective_lifecycle_t<lifecycle::transient_t, request_t>;
        return execute_provider<request_t, dependency_chain_t, effective_lifecycle_t>(default_provider, container);
    }

private:
    template <
        typename request_t, typename dependency_chain_t, typename lifecycle_t, typename provider_t,
        typename container_p>
    auto execute_provider(provider_t& provider, container_p& container) -> returned_t<request_t>
    {
        using provided_t = typename provider_t::provided_t;

        // --- BRANCH ON REQUEST TYPE ---
        if constexpr (detail::is_shared_ptr_v<request_t> || detail::is_weak_ptr_v<request_t>)
        {
            // --- LOGIC FOR SHARED POINTERS (P4, P5, P7) ---
            if constexpr (std::same_as<lifecycle_t, lifecycle::singleton_t>)
            {
                // This is a singleton request, so use the scope's shared_ptr resolver.
                // This correctly handles caching the canonical shared_ptr.
                return as_requested<request_t>(
                    container.template resolve_shared_ptr<provided_t, dependency_chain_t>(provider, container)
                );
            }
            else // transient lifecycle
            {
                // create a new transient shared_ptr.
                return as_requested<request_t>(
                    std::shared_ptr<provided_t>{new provided_t{provider.template create<dependency_chain_t>(container)}}
                );
            }
        }
        else
        {
            static_assert(std::same_as<lifecycle_t, effective_lifecycle_t<lifecycle_t, request_t>>);

            // --- EXISTING LOGIC FOR OTHER TYPES (P1, P2, P3, P6) ---

            if constexpr (std::same_as<lifecycle_t, lifecycle::singleton_t>)
            {
                // Resolve through scope (caches automatically)
                return as_requested<request_t>(
                    container.template resolve_singleton<provided_t, dependency_chain_t>(provider, container)
                );
            }
            else // It's a transient lifecycle
            {
                // Create without caching
                return as_requested<request_t>(provider.template create<dependency_chain_t>(container));
            }
        }
    }
};

/// \brief A metafunction to create a locator type from a tuple of bindings.
template <typename tuple_t>
struct locator_from_tuple_f;

/// \brief Specialization that extracts the binding pack from the tuple.
template <template <typename...> class tuple_p, typename... bindings_t>
struct locator_from_tuple_f<tuple_p<bindings_t...>>
{
    using type = binding_locator_t<bindings_t...>;
};

template <typename scope_t, typename binding_locator_t, typename instance_creator_t>
class container_t : public scope_t
{
    // Helper to get the deduced locator type from a set of configs
    template <typename... bindings_t>
    using deduced_locator_t =
        typename locator_from_tuple_f<decltype(resolve_bindings(std::declval<bindings_t>()...))>::type;

public:
    inline static constexpr auto is_global = std::same_as<scope_t, container::scope::global_t>;

    container_t(scope_t scope, binding_locator_t locator, instance_creator_t instance_creator) noexcept
        : scope_t{std::move(scope)}, binding_locator_{std::move(locator)},
          instance_creator_{std::move(instance_creator)}
    {}

    // global constructor
    template <typename... bindings_t>
    requires((!is_container<bindings_t> && ...) && (is_binding<bindings_t> && ...))
    explicit container_t(bindings_t&&... configs)
        : binding_locator_{resolve_bindings(std::forward<bindings_t>(configs)...)}
    {}

    // nested constructor
    template <typename p_scope_t, typename p_locator_t, typename p_creator_t, typename... bindings_t>
    requires(is_binding<bindings_t> && ...)
    explicit container_t(container_t<p_scope_t, p_locator_t, p_creator_t>& parent, bindings_t&&... configs)
        : scope_t{parent}, binding_locator_{resolve_bindings(std::forward<bindings_t>(configs)...)}
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

        // check local cache for for singleton-lifecycled requests
        if constexpr (request_traits_f<resolved_t>::transitive_lifecycle == transitive_lifecycle_t::singleton)
        {
            if (auto cached = scope_t::template find_in_local_cache<resolved_t>())
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
        if constexpr (!is_global)
        {
            return scope_t::parent->template try_resolve<request_t, dependency_chain_t>(
                std::forward<factory_t>(factory)
            );
        }

        // Not found anywhere - invoke the factory from the original caller
        return factory();
    }

private:
    binding_locator_t binding_locator_{};
    instance_creator_t instance_creator_{};
};

/*
    deduction guides
    
    clang kept matching the global deduction guide for a nested container, so there are 2 clang-20.1-specific workarounds:
        - the global deduction guides must be split into empty and nonempty so we can apply a constrainted to the first_
          parameter
        - the nonempty version must use enable_if_t to remove itself from consideration
    
    When clang fixes this, the empty/nonempty split can be removed, as can the enable_if_t.
*/

//! deduction guide for nonempty global containers
template <
    typename first_binding_p, typename... rest_bindings_p,
    std::enable_if_t<
        !is_container<std::decay_t<first_binding_p>>
            && is_binding<std::decay_t<first_binding_p>>
            && (is_binding<std::decay_t<rest_bindings_p>> && ...),
        int>
    = 0>
container_t(first_binding_p&&, rest_bindings_p&&...) -> container_t<
    container::scope::global_t,
    typename locator_from_tuple_f<
        decltype(resolve_bindings(std::declval<first_binding_p>(), std::declval<rest_bindings_p>()...))>::type,
    instance_creator_t>;

//! deduction guide for empty global containers
container_t() -> container_t<container::scope::global_t, binding_locator_t<>, instance_creator_t>;

//! deduction guide for nested containers
template <typename p_scope_t, typename p_locator_t, typename p_creator_t, typename... bindings_t>
requires(is_binding<bindings_t> && ...)
container_t(container_t<p_scope_t, p_locator_t, p_creator_t>& parent, bindings_t&&...) -> container_t<
    container::scope::nested_t<std::decay_t<decltype(parent)>>,
    typename locator_from_tuple_f<decltype(resolve_bindings(std::declval<bindings_t>()...))>::type, instance_creator_t>;

// type aliases

template <typename... bindings_t>
using global_container_t = container_t<
    container::scope::global_t,
    typename locator_from_tuple_f<decltype(resolve_bindings(std::declval<bindings_t>()...))>::type, instance_creator_t>;

template <typename parent_t, typename... bindings_t>
using child_container_t = container_t<
    container::scope::nested_t<parent_t>,
    typename locator_from_tuple_f<decltype(resolve_bindings(std::declval<bindings_t>()...))>::type,
    dink::instance_creator_t>;

} // namespace dink
