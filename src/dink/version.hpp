// \file
// Copyright (c) 2025 Frank Secilia
// SPDX-License-Identifier: MIT

#pragma once

#include <dink/lib.hpp>
#include <string>

namespace dink {

//! Lightweight semver encapsulation.
class Version {
 public:
  constexpr auto major() const noexcept -> int_t { return major_; }
  constexpr auto major(int_t major) noexcept -> void { major_ = major; }
  constexpr auto minor() const noexcept -> int_t { return minor_; }
  constexpr auto minor(int_t minor) noexcept -> void { minor_ = minor; }
  constexpr auto patch() const noexcept -> int_t { return patch_; }
  constexpr auto patch(int_t patch) noexcept -> void { patch_ = patch; }

  constexpr auto to_string() const noexcept -> std::string {
    return std::to_string(major_) + "." + std::to_string(minor_) + "." +
           std::to_string(patch_);
  }

  constexpr auto operator<=>(const Version& src) const noexcept
      -> auto = default;

  constexpr Version(int_t major, int_t minor, int_t patch) noexcept
      : major_{major}, minor_{minor}, patch_{patch} {}

 private:
  int_t major_{};
  int_t minor_{};
  int_t patch_{};
};

//! Returns version as configured in build.
constexpr auto version() noexcept -> Version {
  return Version{dink_version_major, dink_version_minor, dink_version_patch};
}

}  // namespace dink
