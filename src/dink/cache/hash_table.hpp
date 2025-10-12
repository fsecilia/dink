/*!
    \file
    Copyright (C) 2025 Frank Secilia
*/

#pragma once

#include <dink/lib.hpp>
#include <cassert>
#include <memory>
#include <typeindex>
#include <unordered_map>

namespace dink::cache {

/*!
    caches instances in a hash table
    
    This caches its instances and canonical shared_ptrs in a hash table, mapping from type_index to shared_ptr<void>.
*/
class hash_table_t
{
public:
    //! returns cached storage or creates it using factory
    template <typename instance_t, typename factory_t>
    auto get_or_create_instance(factory_t&& factory) -> instance_t&
    {
        return *get_or_create_shared<instance_t>(std::forward<factory_t>(factory));
    }

    //! returns cached storage or nullptr
    template <typename instance_t>
    auto get_instance() const noexcept -> instance_t*
    {
        auto result = get_shared<instance_t>();
        return result ? result.get() : nullptr;
    }

    //! returns canonical shared_ptr or creates it
    template <typename instance_t, typename factory_t>
    auto get_or_create_shared(factory_t&& factory) -> std::shared_ptr<instance_t>
    {
        auto& instance = map_[typeid(instance_t)];
        if (!instance) instance = std::make_shared<instance_t>(std::forward<factory_t>(factory)());
        return std::static_pointer_cast<instance_t>(instance);
    }

    //! returns canonical shared_ptr or nullptr
    template <typename instance_t>
    auto get_shared() const noexcept -> std::shared_ptr<instance_t>
    {
        auto const result = map_.find(typeid(instance_t));
        return result == map_.end() ? nullptr : std::static_pointer_cast<instance_t>(result->second);
    }

private:
    std::unordered_map<std::type_index, std::shared_ptr<void>> map_;
};

} // namespace dink::cache
