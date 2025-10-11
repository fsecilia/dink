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

struct global_t
{
    template <typename instance_t>
    using cache_t = type_indexed_storage_t<instance_t>;

    // A separate, type-indexed storage for the canonical shared_ptrs.
    template <typename instance_t>
    using shared_cache_t = type_indexed_storage_t<std::shared_ptr<instance_t>>;

public:
    template <typename instance_t, typename dependency_chain_t, typename provider_t, typename container_t>
    auto resolve(provider_t& provider, container_t& container) -> instance_t&
    {
        return cache_t<instance_t>::get_or_create([&]() {
            return provider.template create<dependency_chain_t>(container);
        });
    }

    template <typename instance_t, typename dependency_chain_t, typename provider_t, typename container_t>
    auto resolve_shared(provider_t& provider, container_t& container) -> std::shared_ptr<instance_t>&
    {
        // use shared_ptr with a no-op deleter pointing to the raw singleton
        return shared_cache_t<instance_t>::get_or_create([&]() {
            return std::shared_ptr<instance_t>{
                &this->resolve<instance_t, dependency_chain_t>(provider, container), [](auto&&) {}
            };
        });
    }

    template <typename instance_t>
    auto find() -> instance_t*
    {
        return cache_t<instance_t>::get_if_initialized();
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

template <typename parent_t>
struct is_scope_f<scope::nested_t<parent_t>> : std::true_type
{};

template <typename value_t> concept is_scope = is_scope_f<std::remove_cvref_t<value_t>>::value;

} // namespace dink
