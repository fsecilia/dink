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
  template <typename Constructed>
  struct Provider {
    using Provided = Constructed;
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
};

// ----------------------------------------------------------------------------
// Transient
// ----------------------------------------------------------------------------

struct ScopeTestTransient : ScopeTest {
  using Sut = Transient;
  Sut sut{};

  using Provider = Provider<Requested>;
  Provider provider;
};

TEST_F(ScopeTestTransient, resolves_value) {
  const auto result = sut.resolve<Requested>(container, provider);
  ASSERT_EQ(&container, result.container);
}

TEST_F(ScopeTestTransient, resolves_const_value) {
  const auto result = sut.resolve<const Requested>(container, provider);
  ASSERT_EQ(&container, result.container);
}

TEST_F(ScopeTestTransient, resolves_rvalue_reference) {
  const auto&& result = sut.resolve<Requested&&>(container, provider);
  ASSERT_EQ(&container, result.container);
}

TEST_F(ScopeTestTransient, resolves_rvalue_reference_to_const_value) {
  const auto&& result = sut.resolve<const Requested&&>(container, provider);
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

TEST_F(ScopeTestTransient, resolves_value_per_request) {
  const auto& result1 = sut.resolve<Requested>(container, provider);
  const auto& result2 = sut.resolve<Requested>(container, provider);
  ASSERT_NE(&result1, &result2);
}

TEST_F(ScopeTestTransient, resolves_const_value_per_request) {
  const auto& result1 = sut.resolve<const Requested>(container, provider);
  const auto& result2 = sut.resolve<const Requested>(container, provider);
  ASSERT_NE(&result1, &result2);
}

TEST_F(ScopeTestTransient, resolves_rvalue_reference_per_request) {
  const auto& result1 = sut.resolve<Requested&&>(container, provider);
  const auto& result2 = sut.resolve<Requested&&>(container, provider);
  ASSERT_NE(&result1, &result2);
}

TEST_F(ScopeTestTransient, resolves_rvalue_reference_to_const_per_request) {
  const auto& result1 = sut.resolve<const Requested&&>(container, provider);
  const auto& result2 = sut.resolve<const Requested&&>(container, provider);
  ASSERT_NE(&result1, &result2);
}

TEST_F(ScopeTestTransient, resolves_shared_ptr_per_request) {
  const auto result1 =
      sut.resolve<std::shared_ptr<Requested>>(container, provider);
  const auto result2 =
      sut.resolve<std::shared_ptr<Requested>>(container, provider);
  ASSERT_NE(result1, result2);
}

TEST_F(ScopeTestTransient, resolves_shared_ptr_to_const_per_request) {
  const auto result1 =
      sut.resolve<std::shared_ptr<const Requested>>(container, provider);
  const auto result2 =
      sut.resolve<std::shared_ptr<const Requested>>(container, provider);
  ASSERT_NE(result1, result2);
}

TEST_F(ScopeTestTransient, resolves_unique_ptr_per_request) {
  const auto result1 =
      sut.resolve<std::unique_ptr<Requested>>(container, provider);
  const auto result2 =
      sut.resolve<std::unique_ptr<Requested>>(container, provider);
  ASSERT_NE(result1, result2);
}

TEST_F(ScopeTestTransient, resolves_unique_ptr_to_const_per_request) {
  const auto result1 =
      sut.resolve<std::unique_ptr<const Requested>>(container, provider);
  const auto result2 =
      sut.resolve<std::unique_ptr<const Requested>>(container, provider);
  ASSERT_NE(result1, result2);
}

// ----------------------------------------------------------------------------
// Singleton
// ----------------------------------------------------------------------------

struct ScopeTestSingleton : ScopeTest {
  using Sut = Singleton;
  Sut sut{};
};

TEST_F(ScopeTestSingleton, resolves_reference) {
  struct UniqueRequested : Requested {};
  using UniqueProvider = Provider<UniqueRequested>;
  auto provider = UniqueProvider{};

  const auto& result = sut.resolve<UniqueRequested&>(container, provider);
  ASSERT_EQ(&container, result.container);
}

TEST_F(ScopeTestSingleton, resolves_reference_to_const) {
  struct UniqueRequested : Requested {};
  using UniqueProvider = Provider<UniqueRequested>;
  auto provider = UniqueProvider{};

  const auto& result = sut.resolve<const UniqueRequested&>(container, provider);
  ASSERT_EQ(&container, result.container);
}

TEST_F(ScopeTestSingleton, resolves_pointer) {
  struct UniqueRequested : Requested {};
  using UniqueProvider = Provider<UniqueRequested>;
  auto provider = UniqueProvider{};

  const auto* result = sut.resolve<UniqueRequested*>(container, provider);
  ASSERT_EQ(&container, result->container);
}

TEST_F(ScopeTestSingleton, resolves_pointer_to_const) {
  struct UniqueRequested : Requested {};
  using UniqueProvider = Provider<UniqueRequested>;
  auto provider = UniqueProvider{};

  const auto* result = sut.resolve<const UniqueRequested*>(container, provider);
  ASSERT_EQ(&container, result->container);
}

TEST_F(ScopeTestSingleton, resolves_same_reference_per_provider) {
  struct UniqueRequested : Requested {};
  using UniqueProvider = Provider<UniqueRequested>;
  auto provider = UniqueProvider{};

  const auto& result1 = sut.resolve<UniqueRequested&>(container, provider);
  const auto& result2 = sut.resolve<UniqueRequested&>(container, provider);
  ASSERT_EQ(&result1, &result2);
}

TEST_F(ScopeTestSingleton, resolves_same_reference_to_const_per_provider) {
  struct UniqueRequested : Requested {};
  using UniqueProvider = Provider<UniqueRequested>;
  auto provider = UniqueProvider{};

  const auto& result1 =
      sut.resolve<const UniqueRequested&>(container, provider);
  const auto& result2 =
      sut.resolve<const UniqueRequested&>(container, provider);
  ASSERT_EQ(&result1, &result2);
}

TEST_F(ScopeTestSingleton, resolves_same_pointer_per_provider) {
  struct UniqueRequested : Requested {};
  using UniqueProvider = Provider<UniqueRequested>;
  auto provider = UniqueProvider{};

  const auto result1 = sut.resolve<UniqueRequested*>(container, provider);
  const auto result2 = sut.resolve<UniqueRequested*>(container, provider);
  ASSERT_EQ(result1, result2);
}

TEST_F(ScopeTestSingleton, resolves_same_pointer_to_const_per_provider) {
  struct UniqueRequested : Requested {};
  using UniqueProvider = Provider<UniqueRequested>;
  auto provider = UniqueProvider{};

  const auto result1 = sut.resolve<const UniqueRequested*>(container, provider);
  const auto result2 = sut.resolve<const UniqueRequested*>(container, provider);
  ASSERT_EQ(result1, result2);
}

TEST_F(ScopeTestSingleton, resolves_same_reference_to_const_and_non_const) {
  struct UniqueRequested : Requested {};
  using UniqueProvider = Provider<UniqueRequested>;
  auto provider = UniqueProvider{};

  auto& reference = sut.resolve<UniqueRequested&>(container, provider);
  const auto& reference_to_const =
      sut.resolve<const UniqueRequested&>(container, provider);

  EXPECT_EQ(&reference, &reference_to_const);
}

TEST_F(ScopeTestSingleton,
       resolves_different_references_for_different_providers) {
  struct UniqueRequested : Requested {};
  using UniqueProvider = Provider<UniqueRequested>;
  auto provider = UniqueProvider{};

  const auto& result = sut.resolve<UniqueRequested&>(container, provider);

  struct OtherProvider : Provider<UniqueRequested> {};
  auto other_provider = OtherProvider{};
  auto other_sut = Singleton{};
  const auto& other_result =
      other_sut.resolve<UniqueRequested&>(container, other_provider);

  ASSERT_NE(&result, &other_result);
}

struct ScopeTestSingletonCounts : ScopeTest {
  struct UniqueRequested : Requested {};

  struct CountingProvider : Provider<UniqueRequested> {
    int_t& call_count;
    using Provided = UniqueRequested;

    template <typename Requested>
    auto create(Container& container) noexcept -> Requested {
      ++call_count;
      return Provider::template create<Requested>(container);
    }
  };

  using Sut = Singleton;

  int_t call_count = 0;
  CountingProvider counting_provider{.call_count = call_count};

  Sut sut{};
};

TEST_F(ScopeTestSingletonCounts, calls_provider_only_once) {
  sut.resolve<Requested&>(container, counting_provider);
  sut.resolve<Requested&>(container, counting_provider);
  sut.resolve<Requested*>(container, counting_provider);

  EXPECT_EQ(1, call_count);
}

}  // namespace
}  // namespace dink::scope
