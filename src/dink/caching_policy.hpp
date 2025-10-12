/*!
    \file
    Copyright (C) 2025 Frank Secilia
*/

#pragma once

#include <dink/lib.hpp>
#include <dink/cache/hash_table.hpp>
#include <dink/cache/type_indexed.hpp>
#include <dink/not_found.hpp>
#include <dink/request_traits.hpp>
#include <memory>
#include <type_traits>

namespace dink::caching_policy {

/*!
    caches instances type-indexed storage

    This policy caches its instances and canonical shared pointers using type-indexed storage. These are based on
    Meyers singletons, so it has O(1) lookups with less overhead than a hash table. The cost is that instances it
    caches live until the end of the application, which means they technically outlive the cache. The cache expects to
    have a lifetime similar to the whole application, though, so this lifetime extension is pragmatic.
*/
template <typename cache_t = cache::type_indexed_t<>>
class type_indexed_t
{
public:
    //! resolves a reference to a cached instance
    template <typename instance_t, typename dependency_chain_t, typename provider_t, typename container_t>
    auto resolve(provider_t& provider, container_t& container) -> instance_t&
    {
        return cache_.template get_or_create<instance_t>([&]() {
            return provider.template create<dependency_chain_t>(container);
        });
    }

    //! resolves a reference to a shared_ptr to a cached instance
    template <typename instance_t, typename dependency_chain_t, typename provider_t, typename container_t>
    auto resolve_shared(provider_t& provider, container_t& container) -> std::shared_ptr<instance_t>&
    {
        // the instance is owned by the Meyers, so this shared_ptr to it has a no-op deleter
        return cache_.template get_or_create<std::shared_ptr<instance_t>>([&]() {
            return std::shared_ptr<instance_t>{
                &resolve<instance_t, dependency_chain_t>(provider, container), [](auto&&) {}
            };
        });
    }

    //! finds a pointer to a cached instance, or nullptr
    template <typename instance_t>
    auto find() -> instance_t*
    {
        return cache_.template get<instance_t>();
    }

    //! finds the canonical shared pointer to a cached instance, or nullptr
    template <typename instance_t>
    auto find_shared() -> std::shared_ptr<instance_t>
    {
        auto result = cache_.template get<std::shared_ptr<instance_t>>();
        return result ? *result : nullptr;
    }

    explicit type_indexed_t(cache_t cache) : cache_{std::move(cache)} {}
    type_indexed_t() = default;

private:
    cache_t cache_{};
};

/*!
    caches instances in a hash table
    
    This policy caches its instances and canonical shared_ptrs in a hash table, mapping from type_index to
    shared_ptr<void>.
*/
template <typename cache_t = cache::hash_table_t>
class hash_table_t
{
public:
    template <typename instance_t, typename dependency_chain_t, typename provider_t, typename container_t>
    auto resolve(provider_t& provider, container_t& container) -> instance_t&
    {
        return *cache_.template get_or_create<instance_t>([&]() {
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
    auto find() -> resolved_t*
    {
        return std::to_address(cache_.template get<resolved_t>());
    }

    template <typename resolved_t>
    auto find_shared() -> std::shared_ptr<resolved_t>
    {
        return find<resolved_t>();
    }

    explicit hash_table_t(cache_t cache) noexcept : cache_{std::move(cache)} {}
    hash_table_t() = default;

private:
    cache_t cache_;
};

template <typename>
struct is_caching_policy_f : std::false_type
{};

template <typename cache_t>
struct is_caching_policy_f<type_indexed_t<cache_t>> : std::true_type
{};

template <typename cache_t>
struct is_caching_policy_f<hash_table_t<cache_t>> : std::true_type
{};

template <typename value_t> concept is_caching_policy = is_caching_policy_f<std::remove_cvref_t<value_t>>::value;

} // namespace dink::caching_policy
