// \file
// Copyright (c) 2025 Frank Secilia
// SPDX-License-Identifier: MIT

#include "dispatcher.hpp"
#include <dink/test.hpp>

namespace dink {
namespace {

// ----------------------------------------------------------------------------
// defaults::BindingLocator
// ----------------------------------------------------------------------------

struct DefaultsBindingLocatorTest {
  struct FromType {
    int_t id{};
  };
  static constexpr auto expected_from = FromType{5};

  struct Config {
    template <typename FromType>
    constexpr auto find_binding() const noexcept -> FromType {
      return expected_from;
    }
  };
  static constexpr Config config{};

  using Sut = defaults::BindingLocator;
  static constexpr Sut sut{};

  constexpr DefaultsBindingLocatorTest() {
    static_assert(expected_from.id == sut.template find<FromType>(config).id);
  }
};
[[maybe_unused]] constexpr auto defaults_bindings_locator_test =
    DefaultsBindingLocatorTest{};

}  // namespace
}  // namespace dink
