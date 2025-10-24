// \file
// Copyright (c) 2025 Frank Secilia
// SPDX-License-Identifier: MIT

#include "scope.hpp"
#include <dink/test.hpp>

namespace dink::scope {
namespace {

struct ScopeTest : Test {
  struct Container {};

  struct Provided {
    Container* container;
  };

  struct Provider {
    auto provide(Container& container) noexcept -> Provided {
      return {&container};
    }
  };

  Container container;
  Provider provider;
};

// ----------------------------------------------------------------------------
// Transient
// ----------------------------------------------------------------------------

struct ScopeTestTransient : ScopeTest {
  using Sut = Transient;
  Sut sut;
};

TEST_F(ScopeTestTransient, create) {
  const auto result = sut.create(container, provider);
  ASSERT_EQ(&container, result.container);
}

}  // namespace
}  // namespace dink::scope
