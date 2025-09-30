/*!
    \file
    Copyright (C) 2025 Frank Secilia
*/

#pragma once

#include <dink/lib.hpp>
#include <memory>

namespace dink {

class cache_entry_t
{
public:
    //! true when entry is populated
    auto has_value() const noexcept -> bool { return nullptr != instance_; }

    /*!
        retrieves pointer to the cached instance by casting to its known, concrete type
        
        \tparam value_t concrete type as originally emplaced
        \return pointer to cached instance
        
        \warning This function results in undefined behavior if \p value_t is not the exact, literal type used to 
        `emplace()` the instance. 
    */
    template <typename value_t>
    auto get_as() const noexcept -> value_t*
    {
        return static_cast<value_t*>(std::to_address(instance_));
    }

    //! creates new instance of value_t in entry, destroying and replacing existing
    template <typename value_t, typename... args_t>
    auto emplace(args_t&&... args) -> value_t&
    {
        instance_ = instance_ptr_t{new value_t{std::forward<args_t>(args)...}, &typed_dtor<value_t>};
        return *get_as<value_t>();
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

} // namespace dink
