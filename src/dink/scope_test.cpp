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

  struct Resolved {
    Container* container;
  };

  // Returns given container through member of Requested.
  template <typename Constructed>
  struct TransientProvider {
    using Provided = Constructed;

    template <typename Requested>
    auto create(Container& container) noexcept
        -> std::remove_reference_t<Requested> {
      if constexpr (meta::IsSharedPtr<Requested>) {
        return std::make_shared<
            typename std::pointer_traits<Requested>::element_type>(&container);
      } else if constexpr (meta::IsUniquePtr<Requested>) {
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

  using Provider = TransientProvider<Resolved>;
  Provider provider;
};

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

TEST_F(ScopeTestTransient, resolves_rvalue_reference_to_const_value) {
  const auto&& result = sut.resolve<const Resolved&&>(container, provider);
  ASSERT_EQ(&container, result.container);
}

TEST_F(ScopeTestTransient, resolves_shared_ptr) {
  const auto result =
      sut.resolve<std::shared_ptr<Resolved>>(container, provider);
  ASSERT_EQ(&container, result->container);
}

TEST_F(ScopeTestTransient, resolves_shared_ptr_to_const) {
  const auto result =
      sut.resolve<std::shared_ptr<const Resolved>>(container, provider);
  ASSERT_EQ(&container, result->container);
}

TEST_F(ScopeTestTransient, resolves_unique_ptr) {
  const auto result =
      sut.resolve<std::unique_ptr<Resolved>>(container, provider);
  ASSERT_EQ(&container, result->container);
}

TEST_F(ScopeTestTransient, resolves_unique_ptr_to_const) {
  const auto result =
      sut.resolve<std::unique_ptr<const Resolved>>(container, provider);
  ASSERT_EQ(&container, result->container);
}

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

TEST_F(ScopeTestTransient, resolves_rvalue_reference_to_const_per_request) {
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

TEST_F(ScopeTestTransient, resolves_shared_ptr_to_const_per_request) {
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

TEST_F(ScopeTestTransient, resolves_unique_ptr_to_const_per_request) {
  const auto result1 =
      sut.resolve<std::unique_ptr<const Resolved>>(container, provider);
  const auto result2 =
      sut.resolve<std::unique_ptr<const Resolved>>(container, provider);
  ASSERT_NE(result1, result2);
}

// ----------------------------------------------------------------------------
// Singleton
// ----------------------------------------------------------------------------
// Each test case needs its own local, unique provider to prevent leaking
// cached instances between cases.

struct ScopeTestSingleton : ScopeTest {
  using Sut = Singleton;
  Sut sut{};
};

TEST_F(ScopeTestSingleton, resolves_reference) {
  struct UniqueProvider : TransientProvider<Resolved> {};
  auto provider = UniqueProvider{};

  const auto& result = sut.resolve<Resolved&>(container, provider);
  ASSERT_EQ(&container, result.container);
}

TEST_F(ScopeTestSingleton, resolves_pointer) {
  struct UniqueProvider : TransientProvider<Resolved> {};
  auto provider = UniqueProvider{};

  const auto* result = sut.resolve<Resolved*>(container, provider);
  ASSERT_EQ(&container, result->container);
}

TEST_F(ScopeTestSingleton, resolves_reference_to_const) {
  struct UniqueProvider : TransientProvider<Resolved> {};
  auto provider = UniqueProvider{};

  const auto& result = sut.resolve<const Resolved&>(container, provider);
  ASSERT_EQ(&container, result.container);
}

TEST_F(ScopeTestSingleton, resolves_pointer_to_const) {
  struct UniqueProvider : TransientProvider<Resolved> {};
  auto provider = UniqueProvider{};

  const auto* result = sut.resolve<const Resolved*>(container, provider);
  ASSERT_EQ(&container, result->container);
}

TEST_F(ScopeTestSingleton, resolves_same_reference_per_provider) {
  struct UniqueProvider : TransientProvider<Resolved> {};
  auto provider = UniqueProvider{};

  const auto& result1 = sut.resolve<Resolved&>(container, provider);
  const auto& result2 = sut.resolve<Resolved&>(container, provider);
  ASSERT_EQ(&result1, &result2);
}

TEST_F(ScopeTestSingleton, resolves_same_pointer_per_provider) {
  struct UniqueProvider : TransientProvider<Resolved> {};
  auto provider = UniqueProvider{};

  const auto result1 = sut.resolve<Resolved*>(container, provider);
  const auto result2 = sut.resolve<Resolved*>(container, provider);
  ASSERT_EQ(result1, result2);
}

TEST_F(ScopeTestSingleton, resolves_same_reference_to_const_per_provider) {
  struct UniqueProvider : TransientProvider<Resolved> {};
  auto provider = UniqueProvider{};

  const auto& result1 = sut.resolve<const Resolved&>(container, provider);
  const auto& result2 = sut.resolve<const Resolved&>(container, provider);
  ASSERT_EQ(&result1, &result2);
}

TEST_F(ScopeTestSingleton, resolves_same_pointer_to_const_per_provider) {
  struct UniqueProvider : TransientProvider<Resolved> {};
  auto provider = UniqueProvider{};

  const auto result1 = sut.resolve<const Resolved*>(container, provider);
  const auto result2 = sut.resolve<const Resolved*>(container, provider);
  ASSERT_EQ(result1, result2);
}

TEST_F(ScopeTestSingleton, resolves_same_reference_to_const_and_non_const) {
  struct UniqueProvider : TransientProvider<Resolved> {};
  auto provider = UniqueProvider{};

  auto& reference = sut.resolve<Resolved&>(container, provider);
  const auto& reference_to_const =
      sut.resolve<const Resolved&>(container, provider);

  EXPECT_EQ(&reference, &reference_to_const);
}

TEST_F(ScopeTestSingleton, resolves_same_pointer_to_const_and_non_const) {
  struct UniqueProvider : TransientProvider<Resolved> {};
  auto provider = UniqueProvider{};

  const auto pointer = sut.resolve<Resolved*>(container, provider);
  const auto pointer_to_const =
      sut.resolve<const Resolved*>(container, provider);

  EXPECT_EQ(pointer, pointer_to_const);
}

TEST_F(ScopeTestSingleton,
       resolves_different_references_for_different_providers) {
  struct UniqueProvider : TransientProvider<Resolved> {};
  auto provider = UniqueProvider{};

  const auto& result = sut.resolve<Resolved&>(container, provider);

  struct OtherProvider : TransientProvider<Resolved> {};
  auto other_provider = OtherProvider{};
  auto other_sut = Singleton{};
  const auto& other_result =
      other_sut.resolve<Resolved&>(container, other_provider);

  ASSERT_NE(&result, &other_result);
}

TEST_F(ScopeTestSingleton,
       resolves_different_pointers_for_different_providers) {
  struct UniqueProvider : TransientProvider<Resolved> {};
  auto provider = UniqueProvider{};

  const auto result = sut.resolve<Resolved*>(container, provider);

  struct OtherProvider : TransientProvider<Resolved> {};
  auto other_provider = OtherProvider{};
  auto other_sut = Singleton{};
  const auto other_result =
      other_sut.resolve<Resolved*>(container, other_provider);

  ASSERT_NE(result, other_result);
}

// Construction Counts
// ----------------------------------------------------------------------------

struct ScopeTestSingletonConstructionCounts : ScopeTest {
  struct Requested : Resolved {};

  struct CountingProvider : TransientProvider<Requested> {
    int_t& num_calls;
    using Provided = Requested;

    template <typename Requested>
    auto create(Container& container) noexcept -> Requested {
      ++num_calls;
      return TransientProvider::template create<Requested>(container);
    }
  };

  using Sut = Singleton;

  int_t num_provider_calls = 0;
  CountingProvider counting_provider{.num_calls = num_provider_calls};

  Sut sut{};
};

TEST_F(ScopeTestSingletonConstructionCounts, calls_provider_only_once) {
  sut.resolve<Resolved&>(container, counting_provider);
  sut.resolve<Resolved&>(container, counting_provider);
  sut.resolve<Resolved*>(container, counting_provider);

  EXPECT_EQ(1, num_provider_calls);
}

// ----------------------------------------------------------------------------
// Instance
// ----------------------------------------------------------------------------

struct ScopeTestInstance : ScopeTest {
  static constexpr auto initial_value = int_t{15132};
  static constexpr auto modified_value = int_t{7486};
  struct Requested : Resolved {
    int value = initial_value;
  };

  Requested instance{Resolved{&container}};

  // Returns provided verbatim.
  template <typename Instance>
  struct ReferenceProvider {
    using Provided = Instance;

    Provided& provided;

    template <typename>
    auto create(Container&) noexcept -> Provided& {
      return provided;
    }
  };
  ReferenceProvider<Requested> provider{instance};

  using Sut = Instance;
  Sut sut{};
};

TEST_F(ScopeTestInstance, resolves_value_copy) {
  const auto result = sut.resolve<Requested>(container, provider);

  ASSERT_EQ(&container, result.container);
  ASSERT_EQ(initial_value, result.value);
  ASSERT_NE(&instance, &result);
}

TEST_F(ScopeTestInstance, resolves_mutable_reference) {
  auto& result = sut.resolve<Requested&>(container, provider);

  ASSERT_EQ(&container, result.container);
  ASSERT_EQ(&instance, &result);

  // Verify it's the external instance by mutating.
  result.value = modified_value;
  ASSERT_EQ(modified_value, instance.value);
}

TEST_F(ScopeTestInstance, resolves_mutable_pointer) {
  auto* result = sut.resolve<Requested*>(container, provider);

  ASSERT_EQ(&container, result->container);
  ASSERT_EQ(&instance, result);

  // Verify it's the external instance by mutating.
  result->value = modified_value;
  ASSERT_EQ(modified_value, instance.value);
}

TEST_F(ScopeTestInstance, resolves_const_value_copy) {
  const auto result = sut.resolve<const Requested>(container, provider);

  ASSERT_EQ(&container, result.container);
  ASSERT_EQ(initial_value, result.value);
}

TEST_F(ScopeTestInstance, resolves_const_reference) {
  const auto& result = sut.resolve<const Requested&>(container, provider);

  ASSERT_EQ(&container, result.container);
  ASSERT_EQ(&instance, &result);
}

TEST_F(ScopeTestInstance, resolves_const_pointer) {
  const auto* result = sut.resolve<const Requested*>(container, provider);

  ASSERT_EQ(&container, result->container);
  ASSERT_EQ(&instance, result);
}

TEST_F(ScopeTestInstance, same_reference_across_multiple_resolves) {
  auto& result1 = sut.resolve<Requested&>(container, provider);
  auto& result2 = sut.resolve<Requested&>(container, provider);

  ASSERT_EQ(&result1, &result2);
  ASSERT_EQ(&instance, &result1);
}

TEST_F(ScopeTestInstance, same_pointer_across_multiple_resolves) {
  auto* result1 = sut.resolve<Requested*>(container, provider);
  auto* result2 = sut.resolve<Requested*>(container, provider);

  ASSERT_EQ(result1, result2);
  ASSERT_EQ(&instance, result1);
}

TEST_F(ScopeTestInstance, reference_and_pointer_point_to_same_external) {
  auto& ref = sut.resolve<Requested&>(container, provider);
  auto* ptr = sut.resolve<Requested*>(container, provider);

  ASSERT_EQ(&ref, ptr);
  ASSERT_EQ(&instance, &ref);
}

TEST_F(ScopeTestInstance,
       const_and_non_const_references_point_to_same_external) {
  auto& mutable_ref = sut.resolve<Requested&>(container, provider);
  const auto& const_ref = sut.resolve<const Requested&>(container, provider);

  ASSERT_EQ(&mutable_ref, &const_ref);
  ASSERT_EQ(&instance, &mutable_ref);
}

TEST_F(ScopeTestInstance, const_and_non_const_pointers_point_to_same_external) {
  auto* mutable_ptr = sut.resolve<Requested*>(container, provider);
  const auto* const_ptr = sut.resolve<const Requested*>(container, provider);

  ASSERT_EQ(mutable_ptr, const_ptr);
  ASSERT_EQ(&instance, mutable_ptr);
}

TEST_F(ScopeTestInstance, value_copy_is_independent) {
  auto value_copy = sut.resolve<Requested>(container, provider);

  // Mutate the copy
  value_copy.value = modified_value;

  // External instance should be unchanged
  ASSERT_EQ(initial_value, instance.value);
  ASSERT_EQ(modified_value, value_copy.value);
}

TEST_F(ScopeTestInstance, mutations_through_reference_affect_external) {
  auto& ref = sut.resolve<Requested&>(container, provider);

  ref.value = modified_value;

  ASSERT_EQ(modified_value, instance.value);
}

TEST_F(ScopeTestInstance, mutations_through_pointer_affect_external) {
  auto* ptr = sut.resolve<Requested*>(container, provider);

  ptr->value = modified_value;

  ASSERT_EQ(modified_value, instance.value);
}

TEST_F(ScopeTestInstance, mutations_through_external_affect_reference) {
  auto& ref = sut.resolve<Requested&>(container, provider);

  instance.value = modified_value;

  ASSERT_EQ(modified_value, ref.value);
}

TEST_F(ScopeTestInstance, mutations_through_external_affect_pointer) {
  auto* ptr = sut.resolve<Requested*>(container, provider);

  instance.value = modified_value;

  ASSERT_EQ(modified_value, ptr->value);
}

TEST_F(ScopeTestInstance, multiple_value_copies_are_independent) {
  auto copy1 = sut.resolve<Requested>(container, provider);
  auto copy2 = sut.resolve<Requested>(container, provider);

  const auto modified_value1 = modified_value;
  const auto modified_value2 = modified_value * 2;
  copy1.value = modified_value1;
  copy2.value = modified_value2;

  ASSERT_EQ(modified_value1, copy1.value);
  ASSERT_EQ(modified_value2, copy2.value);
  ASSERT_EQ(initial_value, instance.value);  // Original unchanged
}

// Instance with Different Scopes and Providers
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
       values_from_same_scope_are_independent) {
  auto& ref1 = scope1.resolve<External1&>(container, provider1);
  auto& ref2 = scope1.resolve<External2&>(container, provider2);

  ASSERT_EQ(&external1, &ref1);
  ASSERT_EQ(&external2, &ref2);
  ASSERT_NE(static_cast<void*>(&ref1), static_cast<void*>(&ref2));
}

TEST_F(ScopeTestInstanceDifferentSources,
       values_from_different_scopes_are_independent) {
  auto& ref1 = scope1.resolve<External1&>(container, provider1);
  auto& ref2 = scope2.resolve<External2&>(container, provider2);

  ASSERT_EQ(&external1, &ref1);
  ASSERT_EQ(&external2, &ref2);
  ASSERT_NE(static_cast<void*>(&ref1), static_cast<void*>(&ref2));
}

}  // namespace
}  // namespace dink::scope
