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

class instance_cache
{
public:
    template <typename T>
    auto get() const noexcept -> std::shared_ptr<T>
    {
        auto const it = map_.find(typeid(T));
        return it != map_.end() ? std::static_pointer_cast<T>(it->second) : nullptr;
    }

    template <typename T>
    auto get_or_create(auto&& creator) -> std::shared_ptr<T>
    {
        auto& instance = map_[typeid(T)];
        if (!instance) instance = std::make_shared<T>(creator());
        return instance;
    }

private:
    std::unordered_map<std::type_index, std::shared_ptr<void>> map_;
};

} // namespace dink
