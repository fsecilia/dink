// \file
// Copyright (c) 2025 Frank Secilia
// SPDX-License-Identifier: MIT

#include "scope.hpp"
#include <dink/test.hpp>

namespace dink::scope {
namespace {

#if 0
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

TEST_F(ScopeTestSingleton, resolves_same_reference_per_provider) {
  const auto& result1 = sut.resolve<Requested&>(container, provider);
  const auto& result2 = sut.resolve<Requested&>(container, provider);
  ASSERT_EQ(&result1, &result2);
}

TEST_F(ScopeTestSingleton, resolves_same_reference_to_const_per_provider) {
  const auto& result1 = sut.resolve<const Requested&>(container, provider);
  const auto& result2 = sut.resolve<const Requested&>(container, provider);
  ASSERT_EQ(&result1, &result2);
}

TEST_F(ScopeTestSingleton, resolves_same_pointer_per_provider) {
  const auto result1 = sut.resolve<Requested*>(container, provider);
  const auto result2 = sut.resolve<Requested*>(container, provider);
  ASSERT_EQ(result1, result2);
}

TEST_F(ScopeTestSingleton, resolves_same_pointer_to_const_per_provider) {
  const auto result1 = sut.resolve<const Requested*>(container, provider);
  const auto result2 = sut.resolve<const Requested*>(container, provider);
  ASSERT_EQ(result1, result2);
}

TEST_F(ScopeTestSingleton, resolves_same_shared_ptr_per_provider) {
  const auto result1 =
      sut.resolve<std::shared_ptr<Requested>>(container, provider);
  const auto result2 =
      sut.resolve<std::shared_ptr<Requested>>(container, provider);
  ASSERT_EQ(result1, result2);
}

TEST_F(ScopeTestSingleton, resolves_same_shared_ptr_to_const_per_provider) {
  const auto result1 =
      sut.resolve<std::shared_ptr<const Requested>>(container, provider);
  const auto result2 =
      sut.resolve<std::shared_ptr<const Requested>>(container, provider);
  ASSERT_EQ(result1, result2);
}

TEST_F(ScopeTestSingleton, resolves_same_weak_ptr_per_provider) {
  const auto result1 =
      sut.resolve<std::weak_ptr<Requested>>(container, provider);
  const auto result2 =
      sut.resolve<std::weak_ptr<Requested>>(container, provider);
  ASSERT_EQ(result1.lock(), result2.lock());
}

TEST_F(ScopeTestSingleton, resolves_same_weak_ptr_to_const_per_provider) {
  const auto result1 =
      sut.resolve<std::weak_ptr<const Requested>>(container, provider);
  const auto result2 =
      sut.resolve<std::weak_ptr<const Requested>>(container, provider);
  ASSERT_EQ(result1.lock(), result2.lock());
}

TEST_F(ScopeTestSingleton, resolves_same_reference_to_const_and_non_const) {
  auto& reference = sut.resolve<Requested&>(container, provider);
  const auto& reference_to_const =
      sut.resolve<const Requested&>(container, provider);

  EXPECT_EQ(&reference, &reference_to_const);
}

TEST_F(ScopeTestSingleton,
       resolves_different_references_for_different_providers) {
  const auto& result = sut.resolve<Requested&>(container, provider);

  struct OtherProvider : Provider {};
  auto other_provider = OtherProvider{};
  const auto& other_result = sut.resolve<Requested&>(container, other_provider);

  ASSERT_NE(&result, &other_result);
}

TEST_F(ScopeTestSingleton, shared_ptrs_share_control_block) {
  const auto result1 =
      sut.resolve<std::shared_ptr<Requested>>(container, provider);
  const auto result2 =
      sut.resolve<std::shared_ptr<Requested>>(container, provider);

  EXPECT_EQ(result1.use_count(), result2.use_count());
  EXPECT_EQ(3, result1.use_count());  // Both locals alive, plus canonical
}

TEST_F(ScopeTestSingleton, reference_and_shared_ptr_point_to_same_instance) {
  auto& reference = sut.resolve<Requested&>(container, provider);
  const auto shared =
      sut.resolve<std::shared_ptr<Requested>>(container, provider);

  EXPECT_EQ(&reference, shared.get());
}

TEST_F(ScopeTestSingleton, weak_ptr_does_not_expire_while_singleton_alive) {
  const auto weak = sut.resolve<std::weak_ptr<Requested>>(container, provider);

  // Even with no shared_ptr in scope, weak_ptr should not expire
  // because it tracks the canonical shared_ptr which aliases the static
  EXPECT_FALSE(weak.expired());

  auto shared = weak.lock();
  EXPECT_NE(nullptr, shared);
}

TEST_F(ScopeTestSingleton, weak_ptr_expires_with_canonical_shared_ptr) {
  // resolve reference directly to canonical shared_ptr
  auto& canonical_shared_ptr =
      sut.resolve<std::shared_ptr<Requested>&>(container, provider);
  const auto weak = sut.resolve<std::weak_ptr<Requested>>(container, provider);

  EXPECT_FALSE(weak.expired());
  canonical_shared_ptr.reset();
  EXPECT_TRUE(weak.expired());
}

struct ScopeTestSingletonCounts : ScopeTestSingleton {
  struct CountingProvider : Provider {
    int_t call_count = 0;
    using Provided = Requested;

    template <typename Requested>
    auto create(Container& container) noexcept -> Requested {
      ++call_count;
      return Provider::template create<Requested>(container);
    }
  };
};

TEST_F(ScopeTestSingletonCounts, calls_provider_only_once) {
  auto counting_provider = CountingProvider{};

  sut.resolve<Requested&>(container, counting_provider);
  sut.resolve<Requested&>(container, counting_provider);
  sut.resolve<Requested*>(container, counting_provider);
  sut.resolve<std::shared_ptr<Requested>&>(container, counting_provider);

  EXPECT_EQ(1, counting_provider.call_count);
}
#endif

}  // namespace
}  // namespace dink::scope
