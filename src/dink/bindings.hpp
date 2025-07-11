/*!
    \file
    Copyright (C) 2025 Frank Secilia
*/

#pragma once

#include <dink/lib.hpp>
#include <cassert>
#include <optional>

namespace dink::bindings {

template <typename resolved_t>
class transient_t
{
public:
    auto bind(resolved_t resolved) -> void { resolved_ = std::move(resolved); }

    constexpr auto unbind() noexcept -> void { resolved_.reset(); }
    constexpr auto is_bound() const noexcept -> bool { return resolved_.has_value(); }

    constexpr auto bound() const noexcept -> resolved_t
    {
        assert(is_bound());
        return *resolved_;
    }

private:
    std::optional<resolved_t> resolved_{};
};

template <typename resolved_t>
class shared_t
{
public:
    auto bind(resolved_t& resolved) -> void { resolved_ = &resolved; }

    constexpr auto unbind() noexcept -> void { resolved_ = nullptr; }
    constexpr auto is_bound() const noexcept -> bool { return resolved_; }

    constexpr auto bound() const noexcept -> resolved_t const&
    {
        assert(is_bound());
        return *resolved_;
    }

    constexpr auto bound() noexcept -> resolved_t&
    {
        assert(is_bound());
        return *resolved_;
    }

private:
    resolved_t* resolved_{};
};

} // namespace dink::bindings
