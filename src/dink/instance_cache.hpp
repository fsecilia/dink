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

namespace dink {

class instance_cache_t
{
public:
    template <typename instance_t>
    auto get() const noexcept -> std::shared_ptr<instance_t>
    {
        auto const result = map_.find(typeid(instance_t));
        return result == map_.end() ? nullptr : std::static_pointer_cast<instance_t>(result->second);
    }

    template <typename instance_t>
    auto get_or_create(auto&& creator) -> std::shared_ptr<instance_t>
    {
        auto& instance = map_[typeid(instance_t)];
        if (!instance) instance = std::make_shared<instance_t>(creator());
        return std::static_pointer_cast<instance_t>(instance);
    }

private:
    std::unordered_map<std::type_index, std::shared_ptr<void>> map_;
};

} // namespace dink
