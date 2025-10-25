// \file
// Copyright (c) 2025 Frank Secilia
// SPDX-License-Identifier: MIT

#include "scope.hpp"
#include <dink/test.hpp>

namespace dink::scope {
namespace {

struct ScopeTest : Test {
  struct Container {};
  Container container;

  struct Requested {
    Container* container;
  };

  // returns given container
  struct Provider {
    using Provided = Requested;
    template <typename Requested>
    auto create(Container& container) noexcept -> Requested {
      if constexpr (SharedPtr<Requested>) {
        return std::make_shared<
            typename std::pointer_traits<Requested>::element_type>(&container);
      } else if constexpr (UniquePtr<Requested>) {
        return std::make_unique<
            typename std::pointer_traits<Requested>::element_type>(&container);
      } else {
        return Requested{&container};
      }
    }
  };
  Provider provider;
};

// ----------------------------------------------------------------------------
// Transient
// ----------------------------------------------------------------------------

struct ScopeTestTransient : ScopeTest {
  using Sut = Transient;
  Sut sut;
};

TEST_F(ScopeTestTransient, resolves_value) {
  const auto result = sut.resolve<Requested>(container, provider);
  ASSERT_EQ(&container, result.container);
}

TEST_F(ScopeTestTransient, resolves_const_value) {
  const auto result = sut.resolve<const Requested>(container, provider);
  ASSERT_EQ(&container, result.container);
}

TEST_F(ScopeTestTransient, resolves_shared_ptr) {
  const auto result =
      sut.resolve<std::shared_ptr<Requested>>(container, provider);
  ASSERT_EQ(&container, result->container);
}

TEST_F(ScopeTestTransient, resolves_shared_ptr_to_const) {
  const auto result =
      sut.resolve<std::shared_ptr<const Requested>>(container, provider);
  ASSERT_EQ(&container, result->container);
}

TEST_F(ScopeTestTransient, resolves_unique_ptr) {
  const auto result =
      sut.resolve<std::unique_ptr<Requested>>(container, provider);
  ASSERT_EQ(&container, result->container);
}

TEST_F(ScopeTestTransient, resolves_unique_ptr_to_const) {
  const auto result =
      sut.resolve<std::unique_ptr<const Requested>>(container, provider);
  ASSERT_EQ(&container, result->container);
}

TEST_F(ScopeTestTransient,
       repeated_create_value_calls_return_different_instances) {
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

TEST_F(ScopeTestSingleton, resolves_reference) {
  const auto& result = sut.resolve<Requested&>(container, provider);
  ASSERT_EQ(&container, result.container);
}

TEST_F(ScopeTestSingleton, resolves_reference_to_const) {
  const auto& result = sut.resolve<const Requested&>(container, provider);
  ASSERT_EQ(&container, result.container);
}

TEST_F(ScopeTestSingleton, resolves_pointer) {
  const auto* result = sut.resolve<Requested*>(container, provider);
  ASSERT_EQ(&container, result->container);
}

TEST_F(ScopeTestSingleton, resolves_pointer_to_const) {
  const auto* result = sut.resolve<const Requested*>(container, provider);
  ASSERT_EQ(&container, result->container);
}

TEST_F(ScopeTestSingleton, resolves_shared_ptr) {
  const auto result =
      sut.resolve<std::shared_ptr<Requested>>(container, provider);
  ASSERT_EQ(&container, result->container);
}

TEST_F(ScopeTestSingleton, resolves_shared_ptr_to_const) {
  const auto result =
      sut.resolve<std::shared_ptr<const Requested>>(container, provider);
  ASSERT_EQ(&container, result->container);
}

TEST_F(ScopeTestSingleton, resolves_weak_ptr) {
  const auto result =
      sut.resolve<std::weak_ptr<Requested>>(container, provider);
  ASSERT_EQ(&container, result.lock()->container);
}

TEST_F(ScopeTestSingleton, resolves_weak_ptr_to_const) {
  const auto result =
      sut.resolve<std::weak_ptr<const Requested>>(container, provider);
  ASSERT_EQ(&container, result.lock()->container);
}

TEST_F(ScopeTestSingleton, repeated_create_calls_return_same_instance) {
  const auto& result1 = sut.resolve<Requested&>(container, provider);
  const auto& result2 = sut.resolve<Requested&>(container, provider);
  ASSERT_EQ(&result1, &result2);
}

}  // namespace
}  // namespace dink::scope
