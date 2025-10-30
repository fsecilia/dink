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
  static constexpr auto expected_from = FromType{3};

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

// ----------------------------------------------------------------------------
// defaults::BindingLocator
// ----------------------------------------------------------------------------

struct DefaultsFallbackBindingFactoryTest {
  struct FromType {};

  using Sut = defaults::FallbackBindingFactory;
  static constexpr Sut sut{};

  constexpr DefaultsFallbackBindingFactoryTest() {
    static constexpr auto binding = sut.template create<FromType>();
    using Binding = decltype(binding);
    static_assert(std::same_as<FromType, Binding::FromType>);
    static_assert(std::same_as<scope::Transient, Binding::ScopeType>);
    static_assert(
        std::same_as<provider::Ctor<FromType>, Binding::ProviderType>);
  }
};
[[maybe_unused]] constexpr auto defaults_fallback_binding_factory_test =
    DefaultsFallbackBindingFactoryTest{};

}  // namespace
}  // namespace dink
