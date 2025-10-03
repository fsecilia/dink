/*!
    \file
    Copyright (C) 2025 Frank Secilia
*/

#pragma once

#include <dink/lib.hpp>
#include <any>
#include <cassert>
#include <typeindex>
#include <unordered_map>

namespace dink {

class instance_cache_t
{
public:
    /*!
        finds pointer to const cache entry for given key

        \tparam key_t type used to look up entry; may be an interface instead of value type
        \tparam value_t actual type used to store entry

        \pre value_t must be the same value type used to emplace the entry originally

        \returns pointer to const cache entry if found, nullptr otherwise
    */
    template <typename key_t, typename value_t>
    auto try_find() const noexcept -> value_t const*
    {
        auto const location = entries_by_type_.find(key<key_t>());

        auto const key_found = std::end(entries_by_type_) != location;
        if (!key_found) return nullptr;

        auto* const result = std::any_cast<value_t>(&location->second);

        // if this pops, value_t does not match the type written by emplace
        assert(result != nullptr && "type mismatch for existing key in cache");

        return result;
    }

    /*!
        finds pointer to mutable cache entry for given key
        \copydoc instance_cache_t::try_find() const
        \returns pointer to mutable cache entry if found, nullptr otherwise
    */
    template <typename key_t, typename value_t>
    auto try_find() noexcept -> value_t*
    {
        return const_cast<value_t*>(static_cast<instance_cache_t const&>(*this).try_find<key_t, value_t>());
    }

    /*!
        emplaces typed value in cache entry for given key

        \tparam key_t type used to look up entry; may be an interface instead of value type
        \tparam value_t actual type used to store entry
        \tparam args_t arg types forwarded to value_t's ctor
        \param args args forwarded to value_t's ctor

        \pre value for key must not already exist

        \returns reference to value in cache entry
    */
    template <typename key_t, typename value_t, typename... args_t>
    auto emplace(args_t&&... args) -> value_t&
    {
        // use try_emplace to check for duplicates
        auto const result = entries_by_type_.try_emplace(key<key_t>());

        // if this pops, an attempt was made to overwite an existing key
        assert(result.second && "error: tried to emplace duplicate key");

        // emplace into the std::any and return a reference
        auto& entry = result.first->second;
        entry.template emplace<value_t>(std::forward<args_t>(args)...);
        return std::any_cast<value_t&>(entry);
    }

private:
    template <typename key_t>
    auto key() const noexcept -> std::type_index
    {
        return {typeid(key_t)};
    }

    std::unordered_map<std::type_index, std::any> entries_by_type_;
};

} // namespace dink
