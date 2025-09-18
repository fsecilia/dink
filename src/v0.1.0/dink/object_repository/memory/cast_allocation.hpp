/*!
    \file
    Copyright (C) 2025 Frank Secilia
*/

#pragma once

#include <dink/lib.hpp>
#include <memory>
#include <utility>

namespace dink {

namespace detail {

//! cpo for cast_allocation
template <typename dst_t, typename dst_deleter_t>
struct allocation_caster_t
{
    template <typename allocation_deleter_t, typename... dst_ctor_args_t>
    [[nodiscard]] auto operator()(
        std::unique_ptr<void, allocation_deleter_t>&& allocation, dst_ctor_args_t&&... ctor_args
    ) const -> std::unique_ptr<dst_t, dst_deleter_t>
    {
        if (!allocation) return nullptr;

        // perfect forward ctor args to placement new
        auto* node = new (allocation.get()) dst_t{std::forward<dst_ctor_args_t>(ctor_args)...};

        // transfer ownership, wrapping the allocation's original deleter
        auto result = std::unique_ptr<dst_t, dst_deleter_t>{node, dst_deleter_t{std::move(allocation.get_deleter())}};
        allocation.release();

        return result;
    }
};

} // namespace detail

//! constructs a specific type in an untyped allocation, then transfers ownership and the original deleter
template <typename dst_t, typename dst_deleter_t>
inline constexpr auto cast_allocation = detail::allocation_caster_t<dst_t, dst_deleter_t>{};

} // namespace dink
