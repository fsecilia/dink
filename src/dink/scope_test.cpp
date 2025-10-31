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

  // returns given container through requested
  template <typename Constructed>
  struct Provider {
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

// ----------------------------------------------------------------------------
// Instance
// ----------------------------------------------------------------------------

struct ScopeTestInstance : ScopeTest {
  struct ExternalRequested : Requested {
    int value = 42;
  };

  ExternalRequested external_instance{Requested{&container}};

  template <typename Constructed>
  struct ExternalProvider {
    using Provided = Constructed;
    Constructed* instance_ptr;

    template <typename Requested>
    auto create(Container& /*container*/) noexcept -> Requested {
      if constexpr (std::is_lvalue_reference_v<Requested>) {
        return *instance_ptr;
      } else {
        return *instance_ptr;  // Value - copy
      }
    }
  };

  using Sut = Instance;
  Sut sut{};

  using Provider = ExternalProvider<ExternalRequested>;
  Provider provider{&external_instance};
};

TEST_F(ScopeTestInstance, resolves_value_copy) {
  const auto result = sut.resolve<ExternalRequested>(container, provider);

  ASSERT_EQ(&container, result.container);
  ASSERT_EQ(42, result.value);
  ASSERT_NE(&external_instance, &result);  // Different address - it's a copy
}

TEST_F(ScopeTestInstance, resolves_const_value_copy) {
  const auto result = sut.resolve<const ExternalRequested>(container, provider);

  ASSERT_EQ(&container, result.container);
  ASSERT_EQ(42, result.value);
}

TEST_F(ScopeTestInstance, resolves_mutable_reference) {
  auto& result = sut.resolve<ExternalRequested&>(container, provider);

  ASSERT_EQ(&container, result.container);
  ASSERT_EQ(&external_instance, &result);

  // Verify it's truly the external instance by mutating
  result.value = 99;
  ASSERT_EQ(99, external_instance.value);
}

TEST_F(ScopeTestInstance, resolves_const_reference) {
  const auto& result =
      sut.resolve<const ExternalRequested&>(container, provider);

  ASSERT_EQ(&container, result.container);
  ASSERT_EQ(&external_instance, &result);
}

TEST_F(ScopeTestInstance, resolves_mutable_pointer) {
  auto* result = sut.resolve<ExternalRequested*>(container, provider);

  ASSERT_EQ(&container, result->container);
  ASSERT_EQ(&external_instance, result);

  // Verify it's truly the external instance by mutating
  result->value = 88;
  ASSERT_EQ(88, external_instance.value);
}

TEST_F(ScopeTestInstance, resolves_const_pointer) {
  const auto* result =
      sut.resolve<const ExternalRequested*>(container, provider);

  ASSERT_EQ(&container, result->container);
  ASSERT_EQ(&external_instance, result);
}

TEST_F(ScopeTestInstance, same_reference_across_multiple_resolves) {
  auto& result1 = sut.resolve<ExternalRequested&>(container, provider);
  auto& result2 = sut.resolve<ExternalRequested&>(container, provider);

  ASSERT_EQ(&result1, &result2);
  ASSERT_EQ(&external_instance, &result1);
}

TEST_F(ScopeTestInstance, same_pointer_across_multiple_resolves) {
  auto* result1 = sut.resolve<ExternalRequested*>(container, provider);
  auto* result2 = sut.resolve<ExternalRequested*>(container, provider);

  ASSERT_EQ(result1, result2);
  ASSERT_EQ(&external_instance, result1);
}

TEST_F(ScopeTestInstance, reference_and_pointer_point_to_same_external) {
  auto& ref = sut.resolve<ExternalRequested&>(container, provider);
  auto* ptr = sut.resolve<ExternalRequested*>(container, provider);

  ASSERT_EQ(&ref, ptr);
  ASSERT_EQ(&external_instance, &ref);
}

TEST_F(ScopeTestInstance,
       const_and_non_const_references_point_to_same_external) {
  auto& mutable_ref = sut.resolve<ExternalRequested&>(container, provider);
  const auto& const_ref =
      sut.resolve<const ExternalRequested&>(container, provider);

  ASSERT_EQ(&mutable_ref, &const_ref);
  ASSERT_EQ(&external_instance, &mutable_ref);
}

TEST_F(ScopeTestInstance, const_and_non_const_pointers_point_to_same_external) {
  auto* mutable_ptr = sut.resolve<ExternalRequested*>(container, provider);
  const auto* const_ptr =
      sut.resolve<const ExternalRequested*>(container, provider);

  ASSERT_EQ(mutable_ptr, const_ptr);
  ASSERT_EQ(&external_instance, mutable_ptr);
}

TEST_F(ScopeTestInstance, value_copy_is_independent) {
  auto value_copy = sut.resolve<ExternalRequested>(container, provider);

  // Mutate the copy
  value_copy.value = 111;

  // External instance should be unchanged
  ASSERT_EQ(42, external_instance.value);
  ASSERT_EQ(111, value_copy.value);
}

TEST_F(ScopeTestInstance, mutations_through_reference_affect_external) {
  auto& ref = sut.resolve<ExternalRequested&>(container, provider);

  ref.value = 77;

  ASSERT_EQ(77, external_instance.value);
}

TEST_F(ScopeTestInstance, mutations_through_pointer_affect_external) {
  auto* ptr = sut.resolve<ExternalRequested*>(container, provider);

  ptr->value = 66;

  ASSERT_EQ(66, external_instance.value);
}

TEST_F(ScopeTestInstance, multiple_value_copies_are_independent) {
  auto copy1 = sut.resolve<ExternalRequested>(container, provider);
  auto copy2 = sut.resolve<ExternalRequested>(container, provider);

  copy1.value = 100;
  copy2.value = 200;

  ASSERT_EQ(100, copy1.value);
  ASSERT_EQ(200, copy2.value);
  ASSERT_EQ(42, external_instance.value);  // Original unchanged
}

// Test with different external instance types
struct ScopeTestInstanceDifferentProviders : ScopeTest {
  struct External1 : Requested {
    int id = 1;
  };

  struct External2 : Requested {
    int id = 2;
  };

  External1 external1{Requested{&container}};
  External2 external2{Requested{&container}};

  template <typename Constructed>
  struct ExternalProvider {
    using Provided = Constructed;
    Constructed* instance_ptr;

    template <typename Requested>
    auto create(Container& /*container*/) noexcept -> Requested {
      if constexpr (std::is_lvalue_reference_v<Requested>) {
        return *instance_ptr;
      } else {
        return *instance_ptr;
      }
    }
  };
};

TEST_F(ScopeTestInstanceDifferentProviders,
       different_scopes_for_different_types) {
  using Scope = Instance;
  using Provider1 = ExternalProvider<External1>;
  using Provider2 = ExternalProvider<External2>;

  auto scope1 = Scope{};
  auto scope2 = Scope{};
  auto provider1 = Provider1{&external1};
  auto provider2 = Provider2{&external2};

  auto& ref1 = scope1.resolve<External1&>(container, provider1);
  auto& ref2 = scope2.resolve<External2&>(container, provider2);

  ASSERT_EQ(&external1, &ref1);
  ASSERT_EQ(&external2, &ref2);
  ASSERT_NE(static_cast<void*>(&ref1), static_cast<void*>(&ref2));
}

}  // namespace
}  // namespace dink::scope
