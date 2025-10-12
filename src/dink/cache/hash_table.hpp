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

class hash_table_t
{
public:
    template <typename instance_t, typename factory_t>
    auto get_or_create(factory_t&& factory) -> std::shared_ptr<instance_t>
    {
        auto& instance = map_[typeid(instance_t)];
        if (!instance) instance = std::make_shared<instance_t>(std::forward<factory_t>(factory)());
        return std::static_pointer_cast<instance_t>(instance);
    }

    template <typename instance_t>
    auto get() const noexcept -> std::shared_ptr<instance_t>
    {
        auto const result = map_.find(typeid(instance_t));
        return result == map_.end() ? nullptr : std::static_pointer_cast<instance_t>(result->second);
    }

private:
    std::unordered_map<std::type_index, std::shared_ptr<void>> map_;
};

} // namespace dink::cache
