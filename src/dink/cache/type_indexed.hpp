/*!
    \file
    Copyright (C) 2025 Frank Secilia
*/

#pragma once

#include <dink/lib.hpp>
#include <dink/double_checked_storage.hpp>
#include <utility>

namespace dink::cache {

/*!
    caches instances using type-indexed storage

    This caches its instances and canonical shared pointers using type-indexed storage. These are based on Meyers
    singletons, so it has O(1) lookups with less overhead than a hash table. The cost is that instances it caches live
    until the end of the application, which means they technically outlive the cache. The cache expects to have a
    lifetime similar to the whole application, though, so this lifetime extension is pragmatic.
*/
template <template <typename> class storage_t = double_checked_storage_t>
class type_indexed_t
{
public:
    //! returns cached storage or creates it using factory
    template <typename instance_t, typename factory_t>
    auto get_or_create_instance(factory_t&& factory) -> instance_t&
    {
        return storage<instance_t>().get_or_create(std::forward<factory_t>(factory));
    }

    //! returns cached storage or nullptr
    template <typename instance_t>
    auto get_instance() noexcept -> instance_t*
    {
        return storage<instance_t>().get();
    }

    //! returns canonical shared_ptr or creates it
    template <typename instance_t, typename factory_t>
    auto get_or_create_shared(factory_t&& factory) -> std::shared_ptr<instance_t>
    {
        return get_or_create_instance<std::shared_ptr<instance_t>>([&]() {
            // initialize with instance and wrap in a non-owning ptr
            return std::shared_ptr<instance_t>(
                &get_or_create_instance<instance_t>(std::forward<factory_t>(factory)), [](auto*) {}
            );
        });
    }

    //! returns canonical shared_ptr or nullptr
    template <typename instance_t>
    auto get_shared() noexcept -> std::shared_ptr<instance_t>
    {
        auto const result = get_instance<std::shared_ptr<instance_t>>();
        return result ? *result : nullptr;
    }

private:
    //! Meyers singleton to store the actual static storage
    template <typename instance_t>
    static auto storage() -> storage_t<instance_t>&
    {
        static storage_t<instance_t> instance;
        return instance;
    }
};

} // namespace dink::cache
