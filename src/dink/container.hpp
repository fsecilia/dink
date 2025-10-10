/*!
    \file
    Copyright (C) 2025 Frank Secilia
*/

#pragma once

#include <dink/lib.hpp>
#include <dink/bindings.hpp>
#include <dink/config.hpp>
#include <dink/instance_cache.hpp>
#include <dink/lifecycle.hpp>
#include <dink/providers.hpp>
#include <dink/request_traits.hpp>
#include <dink/resolver.hpp>
#include <dink/type_list.hpp>
#include <atomic>
#include <memory>
#include <mutex>
#include <utility>

namespace dink {

//! initializes an instance using the double-checked locking pattern
template <typename instance_t>
struct double_checked_storage_t
{
public:
    template <typename factory_t>
    auto get_or_create(factory_t&& factory) -> instance_t&
    {
        // try loading outside of lock
        auto* result = atomic.load(std::memory_order_acquire);
        if (result != nullptr) return *result;

        std::lock_guard lock(creation_mutex);

        // try loading from inside of lock
        result = atomic.load(std::memory_order_relaxed);
        if (result != nullptr) return *result;

        // couldn't get, so create
        return create(std::forward<factory_t>(factory));
    }

    auto get_if_initialized() const noexcept -> instance_t* { return atomic.load(std::memory_order_acquire); }

private:
    // store instance in a union so we can control when ctor and dtor are called without using heap
    union union_t
    {
        union_t() {}
        ~union_t() {}
        instance_t instance;
    };
    union_t storage;

    // track instance in a unique_ptr with custom deleter for union
    std::unique_ptr<instance_t, void (*)(instance_t*)> instance{nullptr, [](instance_t* p) { p->~instance_t(); }};

    // instance is replicated into an atomic when valid
    std::atomic<instance_t*> atomic{nullptr};

    // creation uses the double-checked locking pattern
    std::mutex creation_mutex;

    //! creates instance under lock, replicates to atomic_instance
    template <typename factory_t>
    auto create(factory_t&& factory) -> instance_t&
    {
        // find where to put instance
        auto* storage_ptr = &storage.instance;

        // call ctor
        new (storage_ptr) instance_t(factory());

        // capture instance
        instance.reset(storage_ptr);

        // replicate into atomic
        atomic.store(storage_ptr, std::memory_order_release);

        return *instance;
    }
};

//! process-wide accessible type-indexed instance; this is singleton
template <typename instance_t, typename storage_t = double_checked_storage_t<instance_t>>
class type_indexed_storage_t
{
public:
    template <typename factory_t>
    static auto get_or_create(factory_t&& factory) -> instance_t&
    {
        return instance().get_or_create(std::forward<factory_t>(factory));
    }

    static auto get_if_initialized() noexcept -> instance_t* { return instance().get_if_initialized(); }

private:
    static auto instance() -> double_checked_storage_t<instance_t>&
    {
        static storage_t instance;
        return instance;
    }
};

namespace container::scope {

struct global_t
{
public:
    template <typename instance_t, typename dependency_chain_t, typename provider_t, typename container_t>
    auto resolve_singleton(provider_t& provider, container_t& container) -> instance_t&
    {
        return type_indexed_storage_t<instance_t>::get_or_create([&]() {
            return provider.template create<dependency_chain_t>(container);
        });
    }

    template <typename instance_t, typename dependency_chain_t, typename provider_t, typename container_t>
    auto resolve_shared_ptr(provider_t& provider, container_t& container) -> std::shared_ptr<instance_t>&
    {
        // wrap in a shared_ptr with no-op deleter
        static auto shared = std::shared_ptr<instance_t>{
            &resolve_singleton<instance_t, dependency_chain_t>(provider, container), [](auto&&) {}
        };
        return shared;
    }

    template <typename instance_t>
    auto find_in_local_cache() -> instance_t*
    {
        return type_indexed_storage_t<instance_t>::get_if_initialized();
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

template <typename scope_t, typename config_t, typename resolver_t>
class container_t;

// --- Concept to identify a container (avoids ambiguity)
namespace detail {

template <typename>
struct is_container_f : std::false_type
{};

template <typename scope_t, typename config_t, typename resolver_t>
struct is_container_f<container_t<scope_t, config_t, resolver_t>> : std::true_type
{};

} // namespace detail

template <typename T> concept is_container = detail::is_container_f<std::decay_t<T>>::value;

template <typename scope_t, typename config_t, typename resolver_t>
class container_t : public scope_t
{
    // Helper to get the deduced config type from a set of configs
    template <typename... bindings_t>
    using deduced_config_t =
        typename config_from_tuple_f<decltype(resolve_bindings(std::declval<bindings_t>()...))>::type;

public:
    inline static constexpr auto is_global = std::same_as<scope_t, container::scope::global_t>;

    container_t(scope_t scope, config_t config, resolver_t resolver) noexcept
        : scope_t{std::move(scope)}, config_{std::move(config)}, resolver_{std::move(resolver)}
    {}

    // global constructor
    template <typename... bindings_t>
    requires((!is_container<bindings_t> && ...) && (is_binding<bindings_t> && ...))
    explicit container_t(bindings_t&&... configs) : config_{resolve_bindings(std::forward<bindings_t>(configs)...)}
    {}

    // nested constructor
    template <typename p_scope_t, typename p_config_t, typename p_creator_t, typename... bindings_t>
    requires(is_binding<bindings_t> && ...)
    explicit container_t(container_t<p_scope_t, p_config_t, p_creator_t>& parent, bindings_t&&... configs)
        : scope_t{parent}, config_{resolve_bindings(std::forward<bindings_t>(configs)...)}
    {}

    template <typename request_t, typename dependency_chain_t = type_list_t<>>
    auto resolve() -> returned_t<request_t>
    {
        // run search for instance or bindings up the chain, or use the default provider with this container.
        return try_resolve<request_t, dependency_chain_t>([this]() -> decltype(auto) {
            return resolver_.template create_from_default_provider<request_t, dependency_chain_t>(*this);
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
        if (auto binding = config_.template find_binding<resolved_t>())
        {
            using binding_t = std::remove_pointer_t<decltype(binding)>;
            static constexpr auto binding_found = !std::same_as<binding_t, binding_not_found_t>;
            if constexpr (binding_found)
            {
                return resolver_.template create_from_binding<request_t, dependency_chain_t>(*binding, *this);
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
    config_t config_{};
    resolver_t resolver_{};
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
    typename config_from_tuple_f<
        decltype(resolve_bindings(std::declval<first_binding_p>(), std::declval<rest_bindings_p>()...))>::type,
    resolver_t>;

//! deduction guide for empty global containers
container_t() -> container_t<container::scope::global_t, config_t<>, resolver_t>;

//! deduction guide for nested containers
template <typename p_scope_t, typename p_config_t, typename p_creator_t, typename... bindings_t>
requires(is_binding<bindings_t> && ...)
container_t(container_t<p_scope_t, p_config_t, p_creator_t>& parent, bindings_t&&...) -> container_t<
    container::scope::nested_t<std::decay_t<decltype(parent)>>,
    typename config_from_tuple_f<decltype(resolve_bindings(std::declval<bindings_t>()...))>::type, resolver_t>;

// type aliases

template <typename... bindings_t>
using global_container_t = container_t<
    container::scope::global_t,
    typename config_from_tuple_f<decltype(resolve_bindings(std::declval<bindings_t>()...))>::type, resolver_t>;

template <typename parent_t, typename... bindings_t>
using child_container_t = container_t<
    container::scope::nested_t<parent_t>,
    typename config_from_tuple_f<decltype(resolve_bindings(std::declval<bindings_t>()...))>::type, dink::resolver_t>;

} // namespace dink
