/*!
    \file
    Copyright (C) 2025 Frank Secilia
*/

#pragma once

#include <dink/lib.hpp>
#include <cassert>
#include <cstddef>
#include <utility>
#include <variant>

namespace dink::thresholding {

//! delegates to a large or small object reservation
template <typename policy_t>
class reservation_t
{
public:
    using small_object_reservation_t = policy_t::small_object_policy_t::reservation_t;
    using large_object_reservation_t = policy_t::large_object_policy_t::reservation_t;

    auto allocation() const noexcept -> void*
    {
        return std::visit([](auto& strat) noexcept { return strat.allocation(); }, strat_);
    }

    auto commit() noexcept -> void
    {
        std::visit([](auto& strat) noexcept { strat.commit(); }, strat_);
    }

    reservation_t(small_object_reservation_t small_object_reservation) noexcept
        : strat_{std::move(small_object_reservation)}
    {}

    reservation_t(large_object_reservation_t large_object_reservation) noexcept
        : strat_{std::move(large_object_reservation)}
    {}

private:
    std::variant<small_object_reservation_t, large_object_reservation_t> strat_;
};

//! dispatches to small or large object allocator based on requested allocation size
template <typename policy_t>
class allocator_t
{
public:
    using small_object_allocator_t = policy_t::small_object_policy_t::allocator_t;
    using large_object_allocator_t = policy_t::large_object_policy_t::allocator_t;
    using reservation_t = policy_t::reservation_t;

    auto threshold() const noexcept -> std::size_t { return small_object_allocator_.max_allocation_size(); }

    auto reserve(std::size_t size, std::align_val_t align_val) -> reservation_t
    {
        auto const total_allocation_size = size + static_cast<std::size_t>(align_val) - 1;
        if (total_allocation_size <= small_object_allocator_.max_allocation_size())
        {
            return reservation_t{small_object_allocator_.reserve(size, align_val)};
        }

        return reservation_t{large_object_allocator_.reserve(size, align_val)};
    }

    allocator_t(
        small_object_allocator_t small_object_allocator, large_object_allocator_t large_object_allocator
    ) noexcept
        : small_object_allocator_{std::move(small_object_allocator)},
          large_object_allocator_{std::move(large_object_allocator)}
    {}

private:
    small_object_allocator_t small_object_allocator_;
    large_object_allocator_t large_object_allocator_;
};

} // namespace dink::thresholding
