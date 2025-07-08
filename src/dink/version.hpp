/*!
    \file
    Copyright (C) 2025 Frank Secilia
*/

#pragma once

#include <dink/lib.hpp>
#include <compare>
#include <string>

namespace dink {

class version_t
{
public:
    constexpr auto major() const noexcept -> int_t { return major_; }
    constexpr auto major(int_t major) noexcept -> void { major_ = major; }
    constexpr auto minor() const noexcept -> int_t { return minor_; }
    constexpr auto minor(int_t minor) noexcept -> void { minor_ = minor; }
    constexpr auto patch() const noexcept -> int_t { return patch_; }
    constexpr auto patch(int_t patch) noexcept -> void { patch_ = patch; }

    constexpr auto to_string() const noexcept -> std::string
    {
        return std::to_string(major_) + "." + std::to_string(minor_) + "." + std::to_string(patch_);
    }

    constexpr auto operator<=>(version_t const& src) const noexcept -> std::strong_ordering = default;
    constexpr auto operator==(version_t const& src) const noexcept -> bool = default;

    constexpr version_t(int_t major, int_t minor, int_t patch) noexcept : major_{major}, minor_{minor}, patch_{patch} {}

private:
    int_t major_{};
    int_t minor_{};
    int_t patch_{};
};

} // namespace dink
