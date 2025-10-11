/*!
    \file
    Copyright (C) 2025 Frank Secilia
*/

#pragma once

#include <dink/lib.hpp>
#include <dink/instance_cache.hpp>
#include <dink/not_found.hpp>
#include <dink/request_traits.hpp>
#include <dink/type_indexed_storage.hpp>
#include <memory>

namespace dink {
namespace scope {

/*!
    scope for the root, global container
    
    This scope has no parent. 
    
    This scope expects to have a lifetime similar to the whole application, so it uses Meyers singletons for cache.This
    gives it O(1) lookups with less overhead than a hash table.
*/
struct global_t
{
private:
    //! type-indexed storage for references
    template <typename instance_t>
    using cache_t = type_indexed_storage_t<instance_t>;

    //! type-indexed storage for canonical shared_ptrs
    template <typename instance_t>
    using shared_cache_t = type_indexed_storage_t<std::shared_ptr<instance_t>>;

public:
    //! resolves a reference to a meyers singleton
    template <typename instance_t, typename dependency_chain_t, typename provider_t, typename container_t>
    auto resolve(provider_t& provider, container_t& container) -> instance_t&
    {
        return cache_t<instance_t>::get_or_create([&]() {
            return provider.template create<dependency_chain_t>(container);
        });
    }

    //! resolves a shared_ptr with a no-op deleter pointing to the singleton instance
    template <typename instance_t, typename dependency_chain_t, typename provider_t, typename container_t>
    auto resolve_shared(provider_t& provider, container_t& container) -> std::shared_ptr<instance_t>&
    {
        return shared_cache_t<instance_t>::get_or_create([&]() {
            return std::shared_ptr<instance_t>{
                &this->resolve<instance_t, dependency_chain_t>(provider, container), [](auto&&) {}
            };
        });
    }

    //! finds a pointer to the singleton instance, or nullptr
    template <typename instance_t>
    auto find() -> instance_t*
    {
        return cache_t<instance_t>::get_if_initialized();
    }

    //! finds the canonical shared pointer to the singleton instance, or nullptr
    template <typename instance_t>
    auto find_shared() -> std::shared_ptr<instance_t>*
    {
        return shared_cache_t<instance_t>::get_if_initialized();
    }

    // signals there is no parent to delegate to
    template <typename request_t, typename dependency_chain_t>
    auto delegate_to_parent() -> auto
    {
        return not_found;
    }
};

/*!
    scope for nested containers
    
    These scopes cache their instances in a hash table, mapping from type_index to shared_ptr<void>. 
*/
template <typename parent_t>
struct nested_t
{
    parent_t* parent;
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

    explicit nested_t(parent_t& parent) : parent{&parent} {}
};

} // namespace scope

template <typename>
struct is_scope_f : std::false_type
{};

template <>
struct is_scope_f<scope::global_t> : std::true_type
{};

template <typename parent_t>
struct is_scope_f<scope::nested_t<parent_t>> : std::true_type
{};

template <typename value_t> concept is_scope = is_scope_f<std::remove_cvref_t<value_t>>::value;

} // namespace dink
