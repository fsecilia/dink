/*!
    \file
    Copyright (C) 2025 Frank Secilia
*/

#pragma once

#include <dink/lib.hpp>
#include <dink/instance_cache.hpp>
#include <dink/not_found.hpp>
#include <dink/request_traits.hpp>
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

namespace scope {

struct global_t
{
    // A separate, type-indexed storage for the canonical shared_ptrs.
    template <typename instance_t>
    using shared_cache_t = type_indexed_storage_t<std::shared_ptr<instance_t>>;

public:
    template <typename instance_t, typename dependency_chain_t, typename provider_t, typename container_t>
    auto resolve(provider_t& provider, container_t& container) -> instance_t&
    {
        return type_indexed_storage_t<instance_t>::get_or_create([&]() {
            return provider.template create<dependency_chain_t>(container);
        });
    }

    template <typename instance_t, typename dependency_chain_t, typename provider_t, typename container_t>
    auto resolve_shared(provider_t& provider, container_t& container) -> std::shared_ptr<instance_t>&
    {
        return shared_cache_t<instance_t>::get_or_create([&]() {
            // Create a shared_ptr with a no-op deleter pointing to the raw singleton.
            return std::shared_ptr<instance_t>{
                &this->resolve<instance_t, dependency_chain_t>(provider, container), [](auto&&) {}
            };
        });
    }

    template <typename instance_t>
    auto find() -> instance_t*
    {
        return type_indexed_storage_t<instance_t>::get_if_initialized();
    }

    template <typename instance_t>
    auto find_shared() -> std::shared_ptr<instance_t>*
    {
        return shared_cache_t<instance_t>::get_if_initialized();
    }

    template <typename request_t, typename dependency_chain_t>
    auto delegate_to_parent()
    {
        // signal there is no parent to delegate to
        return not_found;
    }
};

template <typename parent_t>
struct nested_t
{
    parent_t* parent;
    explicit nested_t(parent_t& parent) : parent{&parent} {}

    dink::instance_cache_t cache;

    template <typename instance_t, typename dependency_chain_t, typename provider_t, typename container_t>
    auto resolve(provider_t& provider, container_t& container) -> instance_t&
    {
        return *cache.template get_or_create<instance_t>([&]() {
            return provider.template create<dependency_chain_t>(container);
        });
    }

    template <typename instance_t, typename dependency_chain_t, typename provider_t, typename container_t>
    auto resolve_shared(provider_t& provider, container_t& container) -> std::shared_ptr<instance_t>
    {
        // the cache already stores shared_ptrs, so just return that directly
        return resolve<instance_t, dependency_chain_t>(provider, container);
    }

    template <typename resolved_t>
    auto find() -> std::shared_ptr<resolved_t>
    {
        return cache.template get<resolved_t>();
    }

    template <typename resolved_t>
    auto find_shared() -> std::shared_ptr<resolved_t>
    {
        return find<resolved_t>();
    }

    template <typename request_t, typename dependency_chain_t>
    auto delegate_to_parent() -> decltype(auto)
    {
        // delegate remaining resolution the parent
        return parent->template resolve<request_t, dependency_chain_t>();
    }
};

} // namespace scope

template <typename>
struct is_scope_f : std::false_type
{};

template <>
struct is_scope_f<scope::global_t> : std::true_type
{};

template <typename T>
struct is_scope_f<scope::nested_t<T>> : std::true_type
{};

template <typename T> concept is_scope = is_scope_f<std::decay_t<T>>::value;

} // namespace dink
