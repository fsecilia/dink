// \file
// Copyright (c) 2025 Frank Secilia
// SPDX-License-Identifier: MIT

#include "scope.hpp"
#include <dink/test.hpp>

namespace dink::scope {
namespace {

struct ScopeTest : Test {
  struct Container {};

  struct Requested {
    Container* container;
  };

  struct Provider {
    using Provided = Requested;
    template <typename Requested>
    auto create(Container& container) noexcept -> Requested {
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

TEST_F(ScopeTestTransient, create_calls_provider_with_container) {
  const auto result = sut.resolve<Requested>(container, provider);
  ASSERT_EQ(&container, result.container);
}

TEST_F(ScopeTestTransient, repeated_create_calls_return_different_instances) {
  const auto& result1 = sut.resolve<Requested>(container, provider);
  const auto& result2 = sut.resolve<Requested>(container, provider);
  ASSERT_NE(&result1, &result2);
}

// ----------------------------------------------------------------------------
// Singleton
// ----------------------------------------------------------------------------

struct ScopeTestSingleton : ScopeTest {
  using Sut = Singleton;
  Sut sut;
};

TEST_F(ScopeTestSingleton, create_calls_provider_with_container) {
  const auto& result = sut.resolve<Requested>(container, provider);
  ASSERT_EQ(&container, result.container);
}

TEST_F(ScopeTestSingleton, repeated_create_calls_return_same_instance) {
  const auto& result1 = sut.resolve<Requested>(container, provider);
  const auto& result2 = sut.resolve<Requested>(container, provider);
  ASSERT_EQ(&result1, &result2);
}

}  // namespace
}  // namespace dink::scope
