// \file
// Copyright (c) 2025 Frank Secilia
// SPDX-License-Identifier: MIT

#include "scope.hpp"
#include <dink/test.hpp>

namespace dink::scope {
namespace {

struct ScopeTest : Test {
  static constexpr auto kInitialValue = int_t{15132};  // Arbitrary.
  static constexpr auto kModifiedValue = int_t{7486};  // Arbitrary.

  struct Container {};
  Container container;

  struct Resolved {
    Container* container;
    int_t value = kInitialValue;
  };

  // Returns given container through Requested::container.
  template <typename Constructed>
  struct EchoProvider {
    using Provided = Constructed;

    template <typename Requested>
    auto create(Container& container) noexcept
        -> std::remove_reference_t<Requested> {
      if constexpr (meta::IsSharedPtr<Requested>) {
        return std::make_shared<
            typename std::pointer_traits<Requested>::element_type>(
            &container, kInitialValue);
      } else if constexpr (meta::IsUniquePtr<Requested>) {
        return std::make_unique<
            typename std::pointer_traits<Requested>::element_type>(
            &container, kInitialValue);
      } else {
        return Requested{.container = &container, .value = kInitialValue};
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

  using Provider = EchoProvider<Resolved>;
  Provider provider;
};

// Resolution
// ----------------------------------------------------------------------------

TEST_F(ScopeTestTransient, resolves_value) {
  const auto result = sut.resolve<Resolved>(container, provider);
  ASSERT_EQ(&container, result.container);
}

TEST_F(ScopeTestTransient, resolves_const_value) {
  const auto result = sut.resolve<const Resolved>(container, provider);
  ASSERT_EQ(&container, result.container);
}

TEST_F(ScopeTestTransient, resolves_rvalue_reference) {
  const auto&& result = sut.resolve<Resolved&&>(container, provider);
  ASSERT_EQ(&container, result.container);
}

TEST_F(ScopeTestTransient, resolves_const_rvalue_reference) {
  const auto&& result = sut.resolve<const Resolved&&>(container, provider);
  ASSERT_EQ(&container, result.container);
}

TEST_F(ScopeTestTransient, resolves_shared_ptr) {
  const auto result =
      sut.resolve<std::shared_ptr<Resolved>>(container, provider);
  ASSERT_EQ(&container, result->container);
}

TEST_F(ScopeTestTransient, resolves_const_shared_ptr) {
  const auto result =
      sut.resolve<std::shared_ptr<const Resolved>>(container, provider);
  ASSERT_EQ(&container, result->container);
}

TEST_F(ScopeTestTransient, resolves_unique_ptr) {
  const auto result =
      sut.resolve<std::unique_ptr<Resolved>>(container, provider);
  ASSERT_EQ(&container, result->container);
}

TEST_F(ScopeTestTransient, resolves_const_unique_ptr) {
  const auto result =
      sut.resolve<std::unique_ptr<const Resolved>>(container, provider);
  ASSERT_EQ(&container, result->container);
}

// Uniqueness (Per Request)
// ----------------------------------------------------------------------------

TEST_F(ScopeTestTransient, resolves_value_per_request) {
  const auto& result1 = sut.resolve<Resolved>(container, provider);
  const auto& result2 = sut.resolve<Resolved>(container, provider);
  ASSERT_NE(&result1, &result2);
}

TEST_F(ScopeTestTransient, resolves_const_value_per_request) {
  const auto& result1 = sut.resolve<const Resolved>(container, provider);
  const auto& result2 = sut.resolve<const Resolved>(container, provider);
  ASSERT_NE(&result1, &result2);
}

TEST_F(ScopeTestTransient, resolves_rvalue_reference_per_request) {
  const auto& result1 = sut.resolve<Resolved&&>(container, provider);
  const auto& result2 = sut.resolve<Resolved&&>(container, provider);
  ASSERT_NE(&result1, &result2);
}

TEST_F(ScopeTestTransient, resolves_const_rvalue_reference_per_request) {
  const auto& result1 = sut.resolve<const Resolved&&>(container, provider);
  const auto& result2 = sut.resolve<const Resolved&&>(container, provider);
  ASSERT_NE(&result1, &result2);
}

TEST_F(ScopeTestTransient, resolves_shared_ptr_per_request) {
  const auto result1 =
      sut.resolve<std::shared_ptr<Resolved>>(container, provider);
  const auto result2 =
      sut.resolve<std::shared_ptr<Resolved>>(container, provider);
  ASSERT_NE(result1, result2);
}

TEST_F(ScopeTestTransient, resolves_const_shared_ptr_per_request) {
  const auto result1 =
      sut.resolve<std::shared_ptr<const Resolved>>(container, provider);
  const auto result2 =
      sut.resolve<std::shared_ptr<const Resolved>>(container, provider);
  ASSERT_NE(result1, result2);
}

TEST_F(ScopeTestTransient, resolves_unique_ptr_per_request) {
  const auto result1 =
      sut.resolve<std::unique_ptr<Resolved>>(container, provider);
  const auto result2 =
      sut.resolve<std::unique_ptr<Resolved>>(container, provider);
  ASSERT_NE(result1, result2);
}

TEST_F(ScopeTestTransient, resolves_const_unique_ptr_per_request) {
  const auto result1 =
      sut.resolve<std::unique_ptr<const Resolved>>(container, provider);
  const auto result2 =
      sut.resolve<std::unique_ptr<const Resolved>>(container, provider);
  ASSERT_NE(result1, result2);
}

// ----------------------------------------------------------------------------
// Singleton
// ----------------------------------------------------------------------------

struct ScopeTestSingleton : ScopeTest {
  using Sut = Singleton;
  Sut sut{};

  // Each test case needs its own local, unique provider to prevent leaking
  // cached instances between cases.
  using Provider = EchoProvider<Resolved>;
};

// Resolution
// ----------------------------------------------------------------------------

TEST_F(ScopeTestSingleton, resolves_value) {
  struct UniqueProvider : Provider {};
  auto provider = UniqueProvider{};
  const auto result = sut.resolve<Resolved>(container, provider);
  ASSERT_EQ(&container, result.container);
}

TEST_F(ScopeTestSingleton, resolves_const_value) {
  struct UniqueProvider : Provider {};
  auto provider = UniqueProvider{};
  const auto result = sut.resolve<const Resolved>(container, provider);
  ASSERT_EQ(&container, result.container);
}

TEST_F(ScopeTestSingleton, resolves_rvalue_reference) {
  struct UniqueProvider : Provider {};
  auto provider = UniqueProvider{};
  const auto&& result = sut.resolve<Resolved&&>(container, provider);
  ASSERT_EQ(&container, result.container);
}

TEST_F(ScopeTestSingleton, resolves_const_rvalue_reference) {
  struct UniqueProvider : Provider {};
  auto provider = UniqueProvider{};
  const auto&& result = sut.resolve<const Resolved&&>(container, provider);
  ASSERT_EQ(&container, result.container);
}

TEST_F(ScopeTestSingleton, resolves_unique_ptr) {
  struct UniqueProvider : Provider {};
  auto provider = UniqueProvider{};
  const auto result =
      sut.resolve<std::unique_ptr<Resolved>>(container, provider);
  ASSERT_EQ(&container, result->container);
}

TEST_F(ScopeTestSingleton, resolves_const_unique_ptr) {
  struct UniqueProvider : Provider {};
  auto provider = UniqueProvider{};
  const auto result =
      sut.resolve<std::unique_ptr<const Resolved>>(container, provider);
  ASSERT_EQ(&container, result->container);
}

TEST_F(ScopeTestSingleton, resolves_reference) {
  struct UniqueProvider : Provider {};
  auto provider = UniqueProvider{};
  auto& result = sut.resolve<Resolved&>(container, provider);
  ASSERT_EQ(&container, result.container);
}

TEST_F(ScopeTestSingleton, resolves_const_reference) {
  struct UniqueProvider : Provider {};
  auto provider = UniqueProvider{};
  const auto& result = sut.resolve<const Resolved&>(container, provider);
  ASSERT_EQ(&container, result.container);
}

TEST_F(ScopeTestSingleton, resolves_pointer) {
  struct UniqueProvider : Provider {};
  auto provider = UniqueProvider{};
  auto* result = sut.resolve<Resolved*>(container, provider);
  ASSERT_EQ(&container, result->container);
}

TEST_F(ScopeTestSingleton, resolves_const_pointer) {
  struct UniqueProvider : Provider {};
  auto provider = UniqueProvider{};
  const auto* result = sut.resolve<const Resolved*>(container, provider);
  ASSERT_EQ(&container, result->container);
}

// Uniqueness (Per Provider)
// ----------------------------------------------------------------------------

TEST_F(ScopeTestSingleton, resolves_same_reference_per_provider) {
  struct UniqueProvider : Provider {};
  auto provider = UniqueProvider{};
  const auto& result1 = sut.resolve<Resolved&>(container, provider);
  const auto& result2 = sut.resolve<Resolved&>(container, provider);
  ASSERT_EQ(&result1, &result2);
}

TEST_F(ScopeTestSingleton, resolves_same_const_reference_per_provider) {
  struct UniqueProvider : Provider {};
  auto provider = UniqueProvider{};
  const auto& result1 = sut.resolve<const Resolved&>(container, provider);
  const auto& result2 = sut.resolve<const Resolved&>(container, provider);
  ASSERT_EQ(&result1, &result2);
}

TEST_F(ScopeTestSingleton, resolves_same_pointer_per_provider) {
  struct UniqueProvider : Provider {};
  auto provider = UniqueProvider{};
  const auto result1 = sut.resolve<Resolved*>(container, provider);
  const auto result2 = sut.resolve<Resolved*>(container, provider);
  ASSERT_EQ(result1, result2);
}

TEST_F(ScopeTestSingleton, resolves_same_const_pointer_per_provider) {
  struct UniqueProvider : Provider {};
  auto provider = UniqueProvider{};
  const auto result1 = sut.resolve<const Resolved*>(container, provider);
  const auto result2 = sut.resolve<const Resolved*>(container, provider);
  ASSERT_EQ(result1, result2);
}

TEST_F(ScopeTestSingleton,
       resolves_same_instance_for_const_and_non_const_references) {
  struct UniqueProvider : Provider {};
  auto provider = UniqueProvider{};
  auto& reference = sut.resolve<Resolved&>(container, provider);
  const auto& reference_to_const =
      sut.resolve<const Resolved&>(container, provider);
  EXPECT_EQ(&reference, &reference_to_const);
}

TEST_F(ScopeTestSingleton,
       resolves_same_instance_for_const_and_non_const_pointers) {
  struct UniqueProvider : Provider {};
  auto provider = UniqueProvider{};
  const auto pointer = sut.resolve<Resolved*>(container, provider);
  const auto pointer_to_const =
      sut.resolve<const Resolved*>(container, provider);
  EXPECT_EQ(pointer, pointer_to_const);
}

TEST_F(ScopeTestSingleton, resolves_same_instance_for_reference_and_pointer) {
  struct UniqueProvider : Provider {};
  auto provider = UniqueProvider{};
  auto& ref = sut.resolve<Resolved&>(container, provider);
  auto* ptr = sut.resolve<Resolved*>(container, provider);
  EXPECT_EQ(&ref, ptr);
}

// Mutation & State
// ----------------------------------------------------------------------------

TEST_F(ScopeTestSingleton, mutations_through_reference_are_visible) {
  struct UniqueProvider : Provider {};
  auto provider = UniqueProvider{};

  auto& ref1 = sut.resolve<Resolved&>(container, provider);
  ASSERT_EQ(kInitialValue, ref1.value);

  ref1.value = kModifiedValue;

  auto& ref2 = sut.resolve<Resolved&>(container, provider);
  EXPECT_EQ(kModifiedValue, ref2.value);
}

TEST_F(ScopeTestSingleton, mutations_through_pointer_are_visible) {
  struct UniqueProvider : Provider {};
  auto provider = UniqueProvider{};

  auto* ptr1 = sut.resolve<Resolved*>(container, provider);
  ASSERT_EQ(kInitialValue, ptr1->value);

  ptr1->value = kModifiedValue;

  auto* ptr2 = sut.resolve<Resolved*>(container, provider);
  EXPECT_EQ(kModifiedValue, ptr2->value);
}

// Value & Copy Independence
// ----------------------------------------------------------------------------

TEST_F(ScopeTestSingleton, value_resolves_are_independent_copies_of_instance) {
  struct UniqueProvider : Provider {};
  auto provider = UniqueProvider{};

  auto val1 = sut.resolve<Resolved>(container, provider);
  auto val2 = sut.resolve<Resolved>(container, provider);
  ASSERT_NE(&val1, &val2);

  // Mutate copies
  val1.value = kModifiedValue;
  val2.value = kModifiedValue + 1;

  // Ensure original is unchanged
  auto& ref = sut.resolve<Resolved&>(container, provider);
  EXPECT_EQ(kInitialValue, ref.value);
  EXPECT_EQ(kModifiedValue, val1.value);
  EXPECT_EQ(kModifiedValue + 1, val2.value);
}

TEST_F(ScopeTestSingleton,
       rvalue_reference_resolves_are_independent_copies_of_instance) {
  struct UniqueProvider : Provider {};
  auto provider = UniqueProvider{};

  auto&& val1 = sut.resolve<Resolved&&>(container, provider);
  auto&& val2 = sut.resolve<Resolved&&>(container, provider);
  ASSERT_NE(&val1, &val2);

  // Mutate copies
  val1.value = kModifiedValue;
  val2.value = kModifiedValue + 1;

  // Ensure original is unchanged
  auto& ref = sut.resolve<Resolved&>(container, provider);
  EXPECT_EQ(kInitialValue, ref.value);
  EXPECT_EQ(kModifiedValue, val1.value);
  EXPECT_EQ(kModifiedValue + 1, val2.value);
}

TEST_F(ScopeTestSingleton,
       unique_ptr_resolves_are_independent_copies_of_instance) {
  struct UniqueProvider : Provider {};
  auto provider = UniqueProvider{};

  auto val1 = sut.resolve<std::unique_ptr<Resolved>>(container, provider);
  auto val2 = sut.resolve<std::unique_ptr<Resolved>>(container, provider);
  ASSERT_NE(val1.get(), val2.get());

  // Mutate copies
  val1->value = kModifiedValue;
  val2->value = kModifiedValue + 1;

  // Ensure original is unchanged
  auto& ref = sut.resolve<Resolved&>(container, provider);
  EXPECT_EQ(kInitialValue, ref.value);
  EXPECT_EQ(kModifiedValue, val1->value);
  EXPECT_EQ(kModifiedValue + 1, val2->value);
}

// shared_ptr
// ----------------------------------------------------------------------------

struct ScopeTestSingletonSharedPtr : ScopeTest {
  // This needs to specialize on shared_ptr to have a matching Provided because
  // Singleton::cached_instance() returns that verbatim.
  using Provider = EchoProvider<std::shared_ptr<Resolved>>;

  using Sut = Singleton;
  Sut sut{};
};

TEST_F(ScopeTestSingletonSharedPtr, resolves_shared_ptr) {
  struct UniqueProvider : Provider {};
  auto provider = UniqueProvider{};
  const auto result =
      sut.resolve<std::shared_ptr<Resolved>>(container, provider);
  ASSERT_EQ(&container, result->container);
}

TEST_F(ScopeTestSingletonSharedPtr, resolves_const_shared_ptr) {
  struct UniqueProvider : Provider {};
  auto provider = UniqueProvider{};
  const auto result =
      sut.resolve<std::shared_ptr<const Resolved>>(container, provider);
  ASSERT_EQ(&container, result->container);
}

TEST_F(ScopeTestSingletonSharedPtr, resolves_weak_ptr) {
  struct UniqueProvider : Provider {};
  auto provider = UniqueProvider{};
  const auto result = sut.resolve<std::weak_ptr<Resolved>>(container, provider);
  ASSERT_EQ(&container, result.lock()->container);
}

TEST_F(ScopeTestSingletonSharedPtr, resolves_const_weak_ptr) {
  struct UniqueProvider : Provider {};
  auto provider = UniqueProvider{};
  const auto result =
      sut.resolve<std::weak_ptr<const Resolved>>(container, provider);
  ASSERT_EQ(&container, result.lock()->container);
}

TEST_F(ScopeTestSingletonSharedPtr, resolves_same_shared_ptr_per_provider) {
  struct UniqueProvider : Provider {};
  auto provider = UniqueProvider{};
  const auto result1 =
      sut.resolve<std::shared_ptr<Resolved>>(container, provider);
  const auto result2 =
      sut.resolve<std::shared_ptr<Resolved>>(container, provider);
  ASSERT_EQ(result1, result2);
}

TEST_F(ScopeTestSingletonSharedPtr,
       resolves_same_const_shared_ptr_per_provider) {
  struct UniqueProvider : Provider {};
  auto provider = UniqueProvider{};
  const auto result1 =
      sut.resolve<std::shared_ptr<const Resolved>>(container, provider);
  const auto result2 =
      sut.resolve<std::shared_ptr<const Resolved>>(container, provider);
  ASSERT_EQ(result1, result2);
}

TEST_F(ScopeTestSingletonSharedPtr,
       resolves_same_instance_for_const_and_non_const_shared_ptr) {
  struct UniqueProvider : Provider {};
  auto provider = UniqueProvider{};
  const auto pointer =
      sut.resolve<std::shared_ptr<Resolved>>(container, provider);
  const auto pointer_to_const =
      sut.resolve<std::shared_ptr<const Resolved>>(container, provider);
  EXPECT_EQ(pointer, pointer_to_const);
}

TEST_F(ScopeTestSingletonSharedPtr,
       resolves_same_instance_for_shared_ptr_and_weak_ptr) {
  struct UniqueProvider : Provider {};
  auto provider = UniqueProvider{};
  const auto shared =
      sut.resolve<std::shared_ptr<Resolved>>(container, provider);
  const auto weak = sut.resolve<std::weak_ptr<Resolved>>(container, provider);
  EXPECT_EQ(shared, weak.lock());
}

TEST_F(ScopeTestSingletonSharedPtr,
       shared_ptr_value_resolves_are_copies_of_same_smart_pointer) {
  struct UniqueProvider : Provider {};
  auto provider = UniqueProvider{};

  const auto& val1 =
      sut.resolve<std::shared_ptr<Resolved>>(container, provider);
  const auto& val2 =
      sut.resolve<std::shared_ptr<Resolved>>(container, provider);

  // The shared_ptrs themselves are copies.
  ASSERT_NE(&val1, &val2);

  // The copies point to same instance.
  ASSERT_EQ(val1, val2);
}

TEST_F(ScopeTestSingletonSharedPtr,
       shared_ptr_reference_resolves_are_references_to_same_smart_pointer) {
  struct UniqueProvider : Provider {};
  auto provider = UniqueProvider{};

  const auto& val1 =
      sut.resolve<std::shared_ptr<Resolved>&>(container, provider);
  const auto& val2 =
      sut.resolve<std::shared_ptr<Resolved>&>(container, provider);

  ASSERT_EQ(&val1, &val2);
}

// Provider Interactions
// ----------------------------------------------------------------------------

TEST_F(ScopeTestSingleton,
       resolves_different_instances_for_different_providers) {
  struct UniqueProvider : Provider {};
  auto provider = UniqueProvider{};

  struct OtherProvider : Provider {};
  auto other_provider = OtherProvider{};
  auto other_sut = Singleton{};

  const auto& result = sut.resolve<Resolved&>(container, provider);
  const auto& other_result =
      other_sut.resolve<Resolved&>(container, other_provider);

  ASSERT_NE(&result, &other_result);
}

struct ScopeTestSingletonConstructionCounts : ScopeTest {
  struct CountingProvider : EchoProvider<Resolved> {
    int_t& num_calls;
    using Provided = Resolved;

    template <typename Requested>
    auto create(Container& container) noexcept
        -> std::remove_reference_t<Requested> {
      ++num_calls;
      return EchoProvider::template create<Requested>(container);
    }
  };

  using Sut = Singleton;
  Sut sut{};

  int_t num_provider_calls = 0;
  CountingProvider counting_provider{.num_calls = num_provider_calls};
};

TEST_F(ScopeTestSingletonConstructionCounts, calls_provider_create_only_once) {
  sut.resolve<Resolved&>(container, counting_provider);
  sut.resolve<const Resolved&>(container, counting_provider);
  sut.resolve<Resolved*>(container, counting_provider);
  sut.resolve<const Resolved*>(container, counting_provider);
  sut.resolve<Resolved>(container, counting_provider);
  sut.resolve<const Resolved>(container, counting_provider);
  sut.resolve<Resolved&&>(container, counting_provider);
  sut.resolve<const Resolved&&>(container, counting_provider);
  sut.resolve<std::unique_ptr<Resolved>>(container, counting_provider);
  sut.resolve<std::unique_ptr<const Resolved>>(container, counting_provider);

  EXPECT_EQ(1, num_provider_calls);
}

// ----------------------------------------------------------------------------
// Instance
// ----------------------------------------------------------------------------

struct ScopeTestInstance : ScopeTest {
  // Use a distinct type to ensure it's resolving the external instance.
  Resolved external_instance{
      Resolved{.container = &container, .value = kInitialValue}};

  // Returns provided, verbatim.
  template <typename Instance>
  struct ReferenceProvider {
    using Provided = Instance;
    Provided& provided;
    template <typename>
    auto create(Container&) noexcept -> Provided& {
      return provided;
    }
  };
  ReferenceProvider<Resolved> provider{external_instance};

  using Sut = Instance;
  Sut sut{};
};

// Resolution
// ----------------------------------------------------------------------------

TEST_F(ScopeTestInstance, resolves_value) {
  const auto result = sut.resolve<Resolved>(container, provider);
  ASSERT_EQ(&container, result.container);
  ASSERT_NE(&external_instance, &result);
  ASSERT_EQ(kInitialValue, result.value);
}

TEST_F(ScopeTestInstance, resolves_const_value) {
  const auto result = sut.resolve<const Resolved>(container, provider);
  ASSERT_EQ(&container, result.container);
  ASSERT_EQ(kInitialValue, result.value);
}

TEST_F(ScopeTestInstance, resolves_rvalue_reference) {
  const auto&& result = sut.resolve<Resolved&&>(container, provider);
  ASSERT_EQ(&container, result.container);
  ASSERT_NE(&external_instance, &result);
  ASSERT_EQ(kInitialValue, result.value);
}

TEST_F(ScopeTestInstance, resolves_const_rvalue_reference) {
  const auto&& result = sut.resolve<const Resolved&&>(container, provider);
  ASSERT_EQ(&container, result.container);
  ASSERT_NE(&external_instance, &result);
  ASSERT_EQ(kInitialValue, result.value);
}

TEST_F(ScopeTestInstance, resolves_unique_ptr) {
  const auto result =
      sut.resolve<std::unique_ptr<Resolved>>(container, provider);
  ASSERT_EQ(&container, result->container);
  ASSERT_NE(&external_instance, result.get());
  ASSERT_EQ(kInitialValue, result->value);
}

TEST_F(ScopeTestInstance, resolves_const_unique_ptr) {
  const auto result =
      sut.resolve<std::unique_ptr<const Resolved>>(container, provider);
  ASSERT_EQ(&container, result->container);
  ASSERT_NE(&external_instance, result.get());
  ASSERT_EQ(kInitialValue, result->value);
}

TEST_F(ScopeTestInstance, resolves_reference) {
  auto& result = sut.resolve<Resolved&>(container, provider);
  ASSERT_EQ(&container, result.container);
  ASSERT_EQ(&external_instance, &result);
}

TEST_F(ScopeTestInstance, resolves_const_reference) {
  const auto& result = sut.resolve<const Resolved&>(container, provider);
  ASSERT_EQ(&container, result.container);
  ASSERT_EQ(&external_instance, &result);
}

TEST_F(ScopeTestInstance, resolves_pointer) {
  auto* result = sut.resolve<Resolved*>(container, provider);
  ASSERT_EQ(&container, result->container);
  ASSERT_EQ(&external_instance, result);
}

TEST_F(ScopeTestInstance, resolves_const_pointer) {
  const auto* result = sut.resolve<const Resolved*>(container, provider);
  ASSERT_EQ(&container, result->container);
  ASSERT_EQ(&external_instance, result);
}

// Uniqueness (Always Unique)
// ----------------------------------------------------------------------------

TEST_F(ScopeTestInstance, resolves_same_reference) {
  auto& result1 = sut.resolve<Resolved&>(container, provider);
  auto& result2 = sut.resolve<Resolved&>(container, provider);
  ASSERT_EQ(&result1, &result2);
  ASSERT_EQ(&external_instance, &result1);
}

TEST_F(ScopeTestInstance, resolves_same_const_reference) {
  const auto& result1 = sut.resolve<const Resolved&>(container, provider);
  const auto& result2 = sut.resolve<const Resolved&>(container, provider);
  ASSERT_EQ(&result1, &result2);
  ASSERT_EQ(&external_instance, &result1);
}

TEST_F(ScopeTestInstance, resolves_same_pointer) {
  auto* result1 = sut.resolve<Resolved*>(container, provider);
  auto* result2 = sut.resolve<Resolved*>(container, provider);
  ASSERT_EQ(result1, result2);
  ASSERT_EQ(&external_instance, result1);
}

TEST_F(ScopeTestInstance, resolves_same_const_pointer) {
  const auto* result1 = sut.resolve<const Resolved*>(container, provider);
  const auto* result2 = sut.resolve<const Resolved*>(container, provider);
  ASSERT_EQ(result1, result2);
  ASSERT_EQ(&external_instance, result1);
}

TEST_F(ScopeTestInstance,
       resolves_same_instance_for_const_and_non_const_references) {
  auto& mutable_ref = sut.resolve<Resolved&>(container, provider);
  const auto& const_ref = sut.resolve<const Resolved&>(container, provider);
  ASSERT_EQ(&mutable_ref, &const_ref);
  ASSERT_EQ(&external_instance, &mutable_ref);
}

TEST_F(ScopeTestInstance,
       resolves_same_instance_for_const_and_non_const_pointers) {
  auto* mutable_ptr = sut.resolve<Resolved*>(container, provider);
  const auto* const_ptr = sut.resolve<const Resolved*>(container, provider);
  ASSERT_EQ(mutable_ptr, const_ptr);
  ASSERT_EQ(&external_instance, mutable_ptr);
}

TEST_F(ScopeTestInstance, resolves_same_instance_for_reference_and_pointer) {
  auto& ref = sut.resolve<Resolved&>(container, provider);
  auto* ptr = sut.resolve<Resolved*>(container, provider);
  ASSERT_EQ(&ref, ptr);
  ASSERT_EQ(&external_instance, &ref);
}

// Mutation & State
// ----------------------------------------------------------------------------

TEST_F(ScopeTestInstance, mutations_through_reference_are_visible) {
  auto& ref = sut.resolve<Resolved&>(container, provider);
  ref.value = kModifiedValue;
  EXPECT_EQ(kModifiedValue, external_instance.value);
}

TEST_F(ScopeTestInstance, mutations_through_pointer_are_visible) {
  auto* ptr = sut.resolve<Resolved*>(container, provider);
  ptr->value = kModifiedValue;
  EXPECT_EQ(kModifiedValue, external_instance.value);
}

TEST_F(ScopeTestInstance,
       mutations_to_external_instance_are_visible_in_reference) {
  auto& ref = sut.resolve<Resolved&>(container, provider);
  external_instance.value = kModifiedValue;
  EXPECT_EQ(kModifiedValue, ref.value);
}

TEST_F(ScopeTestInstance,
       mutations_to_external_instance_are_visible_in_pointer) {
  auto* ptr = sut.resolve<Resolved*>(container, provider);
  external_instance.value = kModifiedValue;
  EXPECT_EQ(kModifiedValue, ptr->value);
}

// Value & Copy Independence
// ----------------------------------------------------------------------------

TEST_F(ScopeTestInstance, value_resolves_are_independent_copies_of_instance) {
  [[maybe_unused]] auto value_copy = sut.resolve<Resolved>(container, provider);
  value_copy.value = kModifiedValue;
  EXPECT_EQ(kInitialValue, external_instance.value);
  EXPECT_NE(kModifiedValue, external_instance.value);
}

TEST_F(ScopeTestInstance,
       rvalue_reference_resolves_are_independent_copies_of_instance) {
  auto&& value_copy = sut.resolve<Resolved&&>(container, provider);
  value_copy.value = kModifiedValue;
  EXPECT_EQ(kInitialValue, external_instance.value);
  EXPECT_NE(kModifiedValue, external_instance.value);
}

TEST_F(ScopeTestInstance,
       unique_ptr_resolves_are_independent_copies_of_instance) {
  auto value_copy = sut.resolve<std::unique_ptr<Resolved>>(container, provider);
  value_copy->value = kModifiedValue;
  EXPECT_EQ(kInitialValue, external_instance.value);
  EXPECT_NE(kModifiedValue, external_instance.value);
}

// shared_ptr
// ----------------------------------------------------------------------------

struct ScopeTestInstanceSharedPtr : ScopeTest {
  std::shared_ptr<Resolved> external_instance = std::make_shared<Resolved>(
      Resolved{.container = &container, .value = kInitialValue});

  template <typename Instance>
  struct ReferenceProvider {
    using Provided = Instance;
    Provided& provided;
    template <typename>
    auto create(Container&) noexcept -> Provided& {
      return provided;
    }
  };
  ReferenceProvider<std::shared_ptr<Resolved>> provider{external_instance};

  using Sut = Instance;
  Sut sut{};
};

TEST_F(ScopeTestInstanceSharedPtr, resolves_shared_ptr) {
  const auto result =
      sut.resolve<std::shared_ptr<Resolved>>(container, provider);
  ASSERT_EQ(external_instance, result);
}

TEST_F(ScopeTestInstanceSharedPtr, resolves_const_shared_ptr) {
  const auto result =
      sut.resolve<std::shared_ptr<const Resolved>>(container, provider);
  ASSERT_EQ(external_instance, result);
}

TEST_F(ScopeTestInstanceSharedPtr, resolves_weak_ptr) {
  const auto result = sut.resolve<std::weak_ptr<Resolved>>(container, provider);
  ASSERT_EQ(&container, result.lock()->container);
}

TEST_F(ScopeTestInstanceSharedPtr, resolves_const_weak_ptr) {
  const auto result =
      sut.resolve<std::weak_ptr<const Resolved>>(container, provider);
  ASSERT_EQ(&container, result.lock()->container);
}

TEST_F(ScopeTestInstanceSharedPtr, resolves_same_shared_ptr) {
  const auto result1 =
      sut.resolve<std::shared_ptr<Resolved>>(container, provider);
  const auto result2 =
      sut.resolve<std::shared_ptr<Resolved>>(container, provider);
  ASSERT_EQ(result1, result2);
  ASSERT_EQ(external_instance, result1);
}

TEST_F(ScopeTestInstanceSharedPtr, resolves_same_const_shared_ptr) {
  const auto result1 =
      sut.resolve<std::shared_ptr<const Resolved>>(container, provider);
  const auto result2 =
      sut.resolve<std::shared_ptr<const Resolved>>(container, provider);
  ASSERT_EQ(result1, result2);
  ASSERT_EQ(external_instance, result1);
}

TEST_F(ScopeTestInstanceSharedPtr,
       resolves_same_instance_for_const_and_non_const_shared_ptr) {
  const auto result1 =
      sut.resolve<std::shared_ptr<Resolved>>(container, provider);
  const auto result2 =
      sut.resolve<std::shared_ptr<const Resolved>>(container, provider);
  ASSERT_EQ(result1, result2);
}

TEST_F(ScopeTestInstanceSharedPtr,
       resolves_same_instance_for_shared_ptr_and_weak_ptr) {
  const auto shared =
      sut.resolve<std::shared_ptr<Resolved>>(container, provider);
  const auto weak = sut.resolve<std::weak_ptr<Resolved>>(container, provider);
  EXPECT_EQ(shared, weak.lock());
}

TEST_F(ScopeTestInstanceSharedPtr,
       shared_ptr_value_resolves_are_copies_of_same_smart_pointer) {
  const auto& val1 =
      sut.resolve<std::shared_ptr<Resolved>>(container, provider);
  const auto& val2 =
      sut.resolve<std::shared_ptr<Resolved>>(container, provider);

  // The shared_ptrs themselves are copies.
  ASSERT_NE(&val1, &val2);

  // The copies point to same instance.
  ASSERT_EQ(val1, val2);
}

TEST_F(ScopeTestInstanceSharedPtr,
       shared_ptr_reference_resolves_are_references_to_same_smart_pointer) {
  const auto& val1 =
      sut.resolve<std::shared_ptr<Resolved>&>(container, provider);
  const auto& val2 =
      sut.resolve<std::shared_ptr<Resolved>&>(container, provider);

  ASSERT_EQ(&val1, &val2);
}

// Provider Interactions
// ----------------------------------------------------------------------------

struct ScopeTestInstanceDifferentSources : ScopeTestInstance {
  struct External1 : Resolved {};
  External1 external1{&container};
  ReferenceProvider<External1> provider1{external1};
  Sut scope1{};

  struct External2 : Resolved {};
  External2 external2{&container};
  ReferenceProvider<External2> provider2{external2};
  Sut scope2{};
};

TEST_F(ScopeTestInstanceDifferentSources,
       resolves_different_instances_for_different_providers) {
  auto& ref1 = scope1.resolve<External1&>(container, provider1);
  auto& ref2 = scope1.resolve<External2&>(container, provider2);

  ASSERT_EQ(&external1, &ref1);
  ASSERT_EQ(&external2, &ref2);
  ASSERT_NE(static_cast<void*>(&ref1), static_cast<void*>(&ref2));
}

TEST_F(ScopeTestInstanceDifferentSources,
       resolves_different_instances_for_different_scopes_and_providers) {
  auto& ref1 = scope1.resolve<External1&>(container, provider1);
  auto& ref2 = scope2.resolve<External2&>(container, provider2);

  ASSERT_EQ(&external1, &ref1);
  ASSERT_EQ(&external2, &ref2);
  ASSERT_NE(static_cast<void*>(&ref1), static_cast<void*>(&ref2));
}

}  // namespace
}  // namespace dink::scope
