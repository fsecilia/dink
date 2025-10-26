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
    auto create(Container& container) noexcept
        -> std::remove_reference_t<Requested> {
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
  using Sut = Transient<Provider>;
  Sut sut{Provider{}};
};

TEST_F(ScopeTestTransient, resolves_value) {
  const auto result = sut.resolve<Requested>(container);
  ASSERT_EQ(&container, result.container);
}

TEST_F(ScopeTestTransient, resolves_const_value) {
  const auto result = sut.resolve<const Requested>(container);
  ASSERT_EQ(&container, result.container);
}

TEST_F(ScopeTestTransient, resolves_rvalue_reference) {
  const auto&& result = sut.resolve<Requested&&>(container);
  ASSERT_EQ(&container, result.container);
}

TEST_F(ScopeTestTransient, resolves_rvalue_reference_to_const_value) {
  const auto&& result = sut.resolve<const Requested&&>(container);
  ASSERT_EQ(&container, result.container);
}

TEST_F(ScopeTestTransient, resolves_shared_ptr) {
  const auto result = sut.resolve<std::shared_ptr<Requested>>(container);
  ASSERT_EQ(&container, result->container);
}

TEST_F(ScopeTestTransient, resolves_shared_ptr_to_const) {
  const auto result = sut.resolve<std::shared_ptr<const Requested>>(container);
  ASSERT_EQ(&container, result->container);
}

TEST_F(ScopeTestTransient, resolves_unique_ptr) {
  const auto result = sut.resolve<std::unique_ptr<Requested>>(container);
  ASSERT_EQ(&container, result->container);
}

TEST_F(ScopeTestTransient, resolves_unique_ptr_to_const) {
  const auto result = sut.resolve<std::unique_ptr<const Requested>>(container);
  ASSERT_EQ(&container, result->container);
}

TEST_F(ScopeTestTransient, resolves_value_per_request) {
  const auto& result1 = sut.resolve<Requested>(container);
  const auto& result2 = sut.resolve<Requested>(container);
  ASSERT_NE(&result1, &result2);
}

TEST_F(ScopeTestTransient, resolves_const_value_per_request) {
  const auto& result1 = sut.resolve<const Requested>(container);
  const auto& result2 = sut.resolve<const Requested>(container);
  ASSERT_NE(&result1, &result2);
}

TEST_F(ScopeTestTransient, resolves_rvalue_reference_per_request) {
  const auto& result1 = sut.resolve<Requested&&>(container);
  const auto& result2 = sut.resolve<Requested&&>(container);
  ASSERT_NE(&result1, &result2);
}

TEST_F(ScopeTestTransient, resolves_rvalue_reference_to_const_per_request) {
  const auto& result1 = sut.resolve<const Requested&&>(container);
  const auto& result2 = sut.resolve<const Requested&&>(container);
  ASSERT_NE(&result1, &result2);
}

TEST_F(ScopeTestTransient, resolves_shared_ptr_per_request) {
  const auto result1 = sut.resolve<std::shared_ptr<Requested>>(container);
  const auto result2 = sut.resolve<std::shared_ptr<Requested>>(container);
  ASSERT_NE(result1, result2);
}

TEST_F(ScopeTestTransient, resolves_shared_ptr_to_const_per_request) {
  const auto result1 = sut.resolve<std::shared_ptr<const Requested>>(container);
  const auto result2 = sut.resolve<std::shared_ptr<const Requested>>(container);
  ASSERT_NE(result1, result2);
}

TEST_F(ScopeTestTransient, resolves_unique_ptr_per_request) {
  const auto result1 = sut.resolve<std::unique_ptr<Requested>>(container);
  const auto result2 = sut.resolve<std::unique_ptr<Requested>>(container);
  ASSERT_NE(result1, result2);
}

TEST_F(ScopeTestTransient, resolves_unique_ptr_to_const_per_request) {
  const auto result1 = sut.resolve<std::unique_ptr<const Requested>>(container);
  const auto result2 = sut.resolve<std::unique_ptr<const Requested>>(container);
  ASSERT_NE(result1, result2);
}

// ----------------------------------------------------------------------------
// Singleton
// ----------------------------------------------------------------------------

struct ScopeTestSingleton : ScopeTest {
  using Sut = Singleton<Provider>;
  Sut sut{Provider{}};
};

TEST_F(ScopeTestSingleton, resolves_reference) {
  const auto& result = sut.resolve<Requested&>(container);
  ASSERT_EQ(&container, result.container);
}

TEST_F(ScopeTestSingleton, resolves_reference_to_const) {
  const auto& result = sut.resolve<const Requested&>(container);
  ASSERT_EQ(&container, result.container);
}

TEST_F(ScopeTestSingleton, resolves_pointer) {
  const auto* result = sut.resolve<Requested*>(container);
  ASSERT_EQ(&container, result->container);
}

TEST_F(ScopeTestSingleton, resolves_pointer_to_const) {
  const auto* result = sut.resolve<const Requested*>(container);
  ASSERT_EQ(&container, result->container);
}

TEST_F(ScopeTestSingleton, resolves_same_reference_per_provider) {
  const auto& result1 = sut.resolve<Requested&>(container);
  const auto& result2 = sut.resolve<Requested&>(container);
  ASSERT_EQ(&result1, &result2);
}

TEST_F(ScopeTestSingleton, resolves_same_reference_to_const_per_provider) {
  const auto& result1 = sut.resolve<const Requested&>(container);
  const auto& result2 = sut.resolve<const Requested&>(container);
  ASSERT_EQ(&result1, &result2);
}

TEST_F(ScopeTestSingleton, resolves_same_pointer_per_provider) {
  const auto result1 = sut.resolve<Requested*>(container);
  const auto result2 = sut.resolve<Requested*>(container);
  ASSERT_EQ(result1, result2);
}

TEST_F(ScopeTestSingleton, resolves_same_pointer_to_const_per_provider) {
  const auto result1 = sut.resolve<const Requested*>(container);
  const auto result2 = sut.resolve<const Requested*>(container);
  ASSERT_EQ(result1, result2);
}

TEST_F(ScopeTestSingleton, resolves_same_reference_to_const_and_non_const) {
  auto& reference = sut.resolve<Requested&>(container);
  const auto& reference_to_const = sut.resolve<const Requested&>(container);

  EXPECT_EQ(&reference, &reference_to_const);
}

TEST_F(ScopeTestSingleton,
       resolves_different_references_for_different_providers) {
  const auto& result = sut.resolve<Requested&>(container);

  struct OtherProvider : Provider {};
  auto other_sut = Singleton<OtherProvider>{OtherProvider{}};
  const auto& other_result = other_sut.resolve<Requested&>(container);

  ASSERT_NE(&result, &other_result);
}

struct ScopeTestSingletonCounts : ScopeTest {
  struct CountingProvider : Provider {
    int_t& call_count;
    using Provided = Requested;

    template <typename Requested>
    auto create(Container& container) noexcept -> Requested {
      ++call_count;
      return Provider::template create<Requested>(container);
    }
  };

  using Sut = Singleton<CountingProvider>;

  int_t call_count = 0;
  Sut sut{CountingProvider{.call_count = call_count}};
};

TEST_F(ScopeTestSingletonCounts, calls_provider_only_once) {
  sut.resolve<Requested&>(container);
  sut.resolve<Requested&>(container);
  sut.resolve<Requested*>(container);

  EXPECT_EQ(1, call_count);
}

}  // namespace
}  // namespace dink::scope
