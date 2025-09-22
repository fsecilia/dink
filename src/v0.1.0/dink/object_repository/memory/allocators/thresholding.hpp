/*!
    \file
    Copyright (C) 2025 Frank Secilia
*/

#pragma once

#include <dink/lib.hpp>
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

} // namespace dink::thresholding
