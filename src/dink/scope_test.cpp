// \file
// Copyright (c) 2025 Frank Secilia
// SPDX-License-Identifier: MIT

#include "scope.hpp"
#include <dink/test.hpp>

namespace dink::scope {
namespace {

// ----------------------------------------------------------------------------
// Transient
// ----------------------------------------------------------------------------

struct ScopeTransientTest : Test {
  using Sut = Transient;

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
  Sut sut;
};

TEST_F(ScopeTransientTest, create) {
  const auto result = sut.create(container, provider);
  ASSERT_EQ(&container, result.container);
}

}  // namespace
}  // namespace dink::scope
