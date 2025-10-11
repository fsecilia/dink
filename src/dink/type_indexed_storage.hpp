/*!
    \file
    Copyright (C) 2025 Frank Secilia
*/

#pragma once

#include <dink/lib.hpp>
#include <dink/double_checked_storage.hpp>
#include <utility>

namespace dink {

//! type-indexed instance accessible process-wide; this is a singleton
template <typename instance_t, typename storage_t = double_checked_storage_t<instance_t>>
class type_indexed_storage_t
{
public:
    //! returns cached instance or creates it using factory
    template <typename factory_t>
    static auto get_or_create(factory_t&& factory) -> instance_t&
    {
        return instance().get_or_create(std::forward<factory_t>(factory));
    }

    //! returns cached instance or nullptr
    static auto get_if_initialized() noexcept -> instance_t* { return instance().get_if_initialized(); }

private:
    //! meyers singleton to store the actual static instance
    static auto instance() -> double_checked_storage_t<instance_t>&
    {
        static storage_t instance;
        return instance;
    }
};

} // namespace dink
