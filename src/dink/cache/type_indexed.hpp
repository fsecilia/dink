/*!
    \file
    Copyright (C) 2025 Frank Secilia
*/

#pragma once

#include <dink/lib.hpp>
#include <dink/double_checked_storage.hpp>
#include <utility>

namespace dink::cache {

//! type-indexed instance accessible process-wide
template <template <typename> class storage_t = double_checked_storage_t>
class type_indexed_t
{
public:
    //! returns cached instance or creates it using factory
    template <typename instance_t, typename factory_t>
    auto get_or_create(factory_t&& factory) -> instance_t&
    {
        return instance<instance_t>().get_or_create(std::forward<factory_t>(factory));
    }

    //! returns cached instance or nullptr
    template <typename instance_t>
    auto get() noexcept -> instance_t*
    {
        return instance<instance_t>().get();
    }

private:
    //! Meyers singleton to store the actual static instance
    template <typename instance_t>
    static auto instance() -> storage_t<instance_t>&
    {
        static storage_t<instance_t> instance;
        return instance;
    }
};

} // namespace dink::cache
