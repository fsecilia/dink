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

class cache_entry_t
{
public:
    //! true when entry is populated
    auto has_value() const noexcept -> bool { return nullptr != instance_; }

    /*!
        gets reference to the cached instance by casting to its known, concrete type

        \tparam value_t concrete type as originally emplaced
        \return reference to cached instance

        \pre has_value()

        \pre value_t must be the same value type used to emplace the entry originally
        \warning This function results in undefined behavior if \p value_t is not the exact, literal type used to
        `emplace()` the instance.
    */
    template <typename value_t>
    auto get_as() const noexcept -> value_t&
    {
        assert(has_value());
        return *static_cast<value_t*>(std::to_address(instance_));
    }

    /*!
        creates new instance of value_t in entry

        \pre value for key must not already exist
    */
    template <typename value_t, typename... args_t>
    auto emplace(args_t&&... args) -> value_t&
    {
        assert(!has_value());
        instance_ = instance_ptr_t{new value_t{std::forward<args_t>(args)...}, &typed_dtor<value_t>};
        return get_as<value_t>();
    }

    cache_entry_t() noexcept = default;

private:
    template <typename value_t>
    static auto typed_dtor(void* instance) noexcept -> void
    {
        delete static_cast<value_t*>(instance);
    }

    static auto default_dtor(void*) noexcept -> void {}

    using instance_ptr_t = std::unique_ptr<void, decltype(&default_dtor)>;
    instance_ptr_t instance_{nullptr, &default_dtor};
};

// ---------------------------------------------------------------------------------------------------------------------

template <typename entry_t>
class instance_cache_t
{
public:
    /*!
        retrieves reference to cache entry for given key 
        
        Performs a map lookup. If no entry exists for the key, a new, empty entry is created and returned.
        
        \tparam key_t type used to look up the entry; may be an interface instead of the value type 
        \returns reference to the cache entry
    */
    template <typename key_t>
    auto locate() -> entry_t&
    {
        return entries_by_type_[std::type_index{typeid(key_t)}];
    }

private:
    std::unordered_map<std::type_index, entry_t> entries_by_type_;
};

} // namespace dink
