// \file
// Copyright (c) 2025 Frank Secilia
// SPDX-License-Identifier: MIT

#include "dink/container.hpp"
#include <dink/test.hpp>
#include <dink/binding.hpp>
#include <dink/binding_dsl.hpp>
#include <dink/scope.hpp>

namespace dink::container {
namespace {

struct IntegrationTest : Test {
  static inline const auto kInitialValue = int_t{7793};   // arbitrary
  static inline const auto kModifiedValue = int_t{2145};  // arbitrary

  // Base class for types that need instance counting.
  struct Counted {
    static inline int_t num_instances = 0;
    int_t id;
    Counted() : id{num_instances++} {}
  };

  // Arbitrary type with a known initial value.
  struct Initialized : Counted {
    int_t value = kInitialValue;
    Initialized() = default;
  };

  // Base type for unique, local types used as singleton.
  //
  // This is a base class because types bound singleton and promoted requests
  // must be unique, local to the test, or the cached values will leak between
  // tests.
  struct Singleton : Initialized {};

  // Arbitrary type with given initial value.
  struct ValueInitialized : Counted {
    int_t value;
    explicit ValueInitialized(int_t value = 0) : value{value} {}
  };

  //
  struct Instance : ValueInitialized {
    using ValueInitialized::ValueInitialized;
  };

  // Arbitrary type used as the product of a factory.
  struct Product : ValueInitialized {
    using ValueInitialized::ValueInitialized;
  };

  // Arbitrary dependency passed as a ctor param to other types.
  struct Dependency : Initialized {};

  // Common dependencies.
  struct Dep1 : Initialized {
    Dep1() { value = 1; }
  };
  struct Dep2 : Initialized {
    Dep2() { value = 2; }
  };
  struct Dep3 : Initialized {
    Dep3() { value = 3; }
  };

  // Arbitrary common base class.
  struct IService {
    virtual ~IService() = default;
    virtual auto get_value() const -> int_t = 0;
  };

  IntegrationTest() { Counted::num_instances = 0; }
};

// =============================================================================
// SCOPES - Basic Lifetime Management
// How scope configuration affects when instances are created and cached
// =============================================================================

// ----------------------------------------------------------------------------
// Transient Scope Tests
// ----------------------------------------------------------------------------

struct IntegrationTestTransient : IntegrationTest {};

// Resolution
// ----------------------------------------------------------------------------

TEST_F(IntegrationTestTransient, resolves_const_value) {
  auto sut = Container{bind<Initialized>().in<scope::Transient>()};

  const auto value = sut.template resolve<const Initialized>();
  EXPECT_EQ(kInitialValue, value.value);
}

TEST_F(IntegrationTestTransient, resolves_rvalue_reference) {
  auto sut = Container{bind<Initialized>().in<scope::Transient>()};

  auto&& value = sut.template resolve<Initialized&&>();
  EXPECT_EQ(kInitialValue, value.value);
}

// Uniqueness (Per Request)
// ----------------------------------------------------------------------------

TEST_F(IntegrationTestTransient, creates_new_shared_ptr_per_resolve) {
  auto sut = Container{bind<Initialized>().in<scope::Transient>()};

  auto shared1 = sut.template resolve<std::shared_ptr<Initialized>>();
  auto shared2 = sut.template resolve<std::shared_ptr<Initialized>>();

  EXPECT_NE(shared1.get(), shared2.get());  // Different instances
}

TEST_F(IntegrationTestTransient, creates_new_value_per_resolve) {
  auto sut = Container{bind<Initialized>().in<scope::Transient>()};

  auto value1 = sut.template resolve<Initialized>();
  auto value2 = sut.template resolve<Initialized>();

  EXPECT_EQ(0, value1.id);
  EXPECT_EQ(1, value2.id);
}

TEST_F(IntegrationTestTransient, creates_new_unique_ptr_per_resolve) {
  auto sut = Container{bind<Initialized>().in<scope::Transient>()};

  auto unique1 = sut.template resolve<std::unique_ptr<Initialized>>();
  auto unique2 = sut.template resolve<std::unique_ptr<Initialized>>();

  EXPECT_NE(unique1.get(), unique2.get());
  EXPECT_EQ(kInitialValue, unique1->value);
  EXPECT_EQ(kInitialValue, unique2->value);
}

// ----------------------------------------------------------------------------
// Singleton Scope Tests
// ----------------------------------------------------------------------------

struct IntegrationTestSingleton : IntegrationTest {};

// Resolved Value
// ----------------------------------------------------------------------------

TEST_F(IntegrationTestSingleton, resolves_reference) {
  struct Type : Singleton {};
  auto sut = Container{bind<Type>().in<scope::Singleton>()};

  auto& ref = sut.template resolve<Type&>();

  EXPECT_EQ(kInitialValue, ref.value);
}

TEST_F(IntegrationTestSingleton, resolves_const_reference) {
  struct Type : Singleton {};
  auto sut = Container{bind<Type>().in<scope::Singleton>()};

  const auto& ref = sut.template resolve<const Type&>();

  EXPECT_EQ(kInitialValue, ref.value);
}

TEST_F(IntegrationTestSingleton, resolves_pointer) {
  struct Type : Singleton {};
  auto sut = Container{bind<Type>().in<scope::Singleton>()};

  auto* ptr = sut.template resolve<Type*>();

  EXPECT_EQ(kInitialValue, ptr->value);
}

TEST_F(IntegrationTestSingleton, resolves_const_pointer) {
  struct Type : Singleton {};
  auto sut = Container{bind<Type>().in<scope::Singleton>()};

  const auto* ptr = sut.template resolve<const Type*>();

  EXPECT_EQ(kInitialValue, ptr->value);
}

TEST_F(IntegrationTestSingleton, resolves_shared_pointer) {
  struct Type : Singleton {};
  auto sut = Container{bind<Type>().in<scope::Singleton>()};

  const auto shared_ptr = sut.template resolve<std::shared_ptr<Type>>();

  EXPECT_EQ(kInitialValue, shared_ptr->value);
}

TEST_F(IntegrationTestSingleton, resolves_const_shared_pointer) {
  struct Type : Singleton {};
  auto sut = Container{bind<Type>().in<scope::Singleton>()};

  const auto shared_ptr = sut.template resolve<const std::shared_ptr<Type>>();

  EXPECT_EQ(kInitialValue, shared_ptr->value);
}

TEST_F(IntegrationTestSingleton, resolves_shared_pointer_to_const) {
  struct Type : Singleton {};
  auto sut = Container{bind<Type>().in<scope::Singleton>()};

  const auto shared_ptr = sut.template resolve<std::shared_ptr<const Type>>();

  EXPECT_EQ(kInitialValue, shared_ptr->value);
}

TEST_F(IntegrationTestSingleton, resolves_const_shared_pointer_to_const) {
  struct Type : Singleton {};
  auto sut = Container{bind<Type>().in<scope::Singleton>()};

  const auto shared_ptr =
      sut.template resolve<const std::shared_ptr<const Type>>();

  EXPECT_EQ(kInitialValue, shared_ptr->value);
}

TEST_F(IntegrationTestSingleton, resolves_weak_pointer) {
  struct Type : Singleton {};
  auto sut = Container{bind<Type>().in<scope::Singleton>()};

  const auto weak_ptr = sut.template resolve<std::weak_ptr<Type>>();

  EXPECT_EQ(kInitialValue, weak_ptr.lock()->value);
}

TEST_F(IntegrationTestSingleton, resolves_const_weak_pointer) {
  struct Type : Singleton {};
  auto sut = Container{bind<Type>().in<scope::Singleton>()};

  const auto weak_ptr = sut.template resolve<const std::weak_ptr<Type>>();

  EXPECT_EQ(kInitialValue, weak_ptr.lock()->value);
}

TEST_F(IntegrationTestSingleton, resolves_weak_pointer_to_const) {
  struct Type : Singleton {};
  auto sut = Container{bind<Type>().in<scope::Singleton>()};

  const auto weak_ptr = sut.template resolve<std::weak_ptr<const Type>>();

  EXPECT_EQ(kInitialValue, weak_ptr.lock()->value);
}

TEST_F(IntegrationTestSingleton, resolves_const_weak_pointer_to_const) {
  struct Type : Singleton {};
  auto sut = Container{bind<Type>().in<scope::Singleton>()};

  const auto weak_ptr = sut.template resolve<const std::weak_ptr<const Type>>();

  EXPECT_EQ(kInitialValue, weak_ptr.lock()->value);
}

// Resolved Identity
// ----------------------------------------------------------------------------

TEST_F(IntegrationTestSingleton, const_reference_is_same_as_reference) {
  struct Type : Singleton {};
  auto sut = Container{bind<Type>().in<scope::Singleton>()};

  const auto& ref = sut.template resolve<Type&>();
  const auto& const_ref = sut.template resolve<const Type&>();

  EXPECT_EQ(&ref, &const_ref);
}

TEST_F(IntegrationTestSingleton, pointer_is_same_as_reference) {
  struct Type : Singleton {};
  auto sut = Container{bind<Type>().in<scope::Singleton>()};

  const auto& ref = sut.template resolve<Type&>();
  auto* ptr = sut.template resolve<Type*>();

  EXPECT_EQ(&ref, ptr);
}

TEST_F(IntegrationTestSingleton, const_pointer_is_same_as_reference) {
  struct Type : Singleton {};
  auto sut = Container{bind<Type>().in<scope::Singleton>()};

  const auto& ref = sut.template resolve<Type&>();
  const auto* ptr = sut.template resolve<const Type*>();

  EXPECT_EQ(&ref, ptr);
}

TEST_F(IntegrationTestSingleton, shared_ptr_is_same_as_reference) {
  struct Type : Singleton {};
  auto sut = Container{bind<Type>().in<scope::Singleton>()};

  const auto& ref = sut.template resolve<Type&>();
  const auto shared_ptr = sut.template resolve<std::shared_ptr<Type>>();

  EXPECT_EQ(&ref, shared_ptr.get());
}

TEST_F(IntegrationTestSingleton, const_shared_pointer_is_same_as_reference) {
  struct Type : Singleton {};
  auto sut = Container{bind<Type>().in<scope::Singleton>()};

  const auto& ref = sut.template resolve<Type&>();
  const auto shared_ptr = sut.template resolve<const std::shared_ptr<Type>>();

  EXPECT_EQ(&ref, shared_ptr.get());
}

TEST_F(IntegrationTestSingleton, shared_pointer_to_const_is_same_as_reference) {
  struct Type : Singleton {};
  auto sut = Container{bind<Type>().in<scope::Singleton>()};

  const auto& ref = sut.template resolve<Type&>();
  const auto shared_ptr = sut.template resolve<std::shared_ptr<const Type>>();

  EXPECT_EQ(&ref, shared_ptr.get());
}

TEST_F(IntegrationTestSingleton,
       const_shared_pointer_to_const_is_same_as_reference) {
  struct Type : Singleton {};
  auto sut = Container{bind<Type>().in<scope::Singleton>()};

  const auto& ref = sut.template resolve<Type&>();
  const auto shared_ptr =
      sut.template resolve<const std::shared_ptr<const Type>>();

  EXPECT_EQ(&ref, shared_ptr.get());
}

TEST_F(IntegrationTestSingleton, weak_pointer_is_same_as_reference) {
  struct Type : Singleton {};
  auto sut = Container{bind<Type>().in<scope::Singleton>()};

  const auto& ref = sut.template resolve<Type&>();
  const auto weak_ptr = sut.template resolve<std::weak_ptr<Type>>();

  EXPECT_EQ(&ref, weak_ptr.lock().get());
}

TEST_F(IntegrationTestSingleton, const_weak_pointer_is_same_as_reference) {
  struct Type : Singleton {};
  auto sut = Container{bind<Type>().in<scope::Singleton>()};

  const auto& ref = sut.template resolve<Type&>();
  const auto weak_ptr = sut.template resolve<const std::weak_ptr<Type>>();

  EXPECT_EQ(&ref, weak_ptr.lock().get());
}

TEST_F(IntegrationTestSingleton, weak_pointer_to_const_is_same_as_reference) {
  struct Type : Singleton {};
  auto sut = Container{bind<Type>().in<scope::Singleton>()};

  const auto& ref = sut.template resolve<Type&>();
  const auto weak_ptr = sut.template resolve<std::weak_ptr<const Type>>();

  EXPECT_EQ(&ref, weak_ptr.lock().get());
}

TEST_F(IntegrationTestSingleton,
       const_weak_pointer_to_const_is_same_as_reference) {
  struct Type : Singleton {};
  auto sut = Container{bind<Type>().in<scope::Singleton>()};

  const auto& ref = sut.template resolve<Type&>();
  const auto weak_ptr = sut.template resolve<const std::weak_ptr<const Type>>();

  EXPECT_EQ(&ref, weak_ptr.lock().get());
}

TEST_F(IntegrationTestSingleton, weak_pointer_survives_without_shared) {
  struct Type : Singleton {};
  auto sut = Container{bind<Type>().in<scope::Singleton>()};

  const auto& weak = sut.template resolve<std::weak_ptr<Type>>();

  // Even with no shared_ptr in scope, weak_ptr should not expire because it
  // tracks a cached shared_ptr to the instance.
  EXPECT_FALSE(weak.expired());

  auto shared = weak.lock();
  EXPECT_NE(nullptr, shared);
}

TEST_F(IntegrationTestSingleton, weak_ptr_expires_with_cached_shared_ptr) {
  struct Type : Singleton {};
  auto sut = Container{bind<Type>().in<scope::Singleton>()};

  // Resolve reference directly to cached shared_ptr.
  auto& cached_shared_ptr = sut.template resolve<std::shared_ptr<Type>&>();
  const auto weak = sut.template resolve<std::weak_ptr<Type>>();

  EXPECT_FALSE(weak.expired());
  cached_shared_ptr.reset();
  EXPECT_TRUE(weak.expired());
}

// Resolved Caching (Shared Pointers)
// ----------------------------------------------------------------------------

TEST_F(IntegrationTestSingleton, resolves_same_shared_ptr) {
  struct Type : Singleton {};
  auto sut = Container{bind<Type>().in<scope::Singleton>()};

  const auto& result1 = sut.template resolve<std::shared_ptr<Type>>();
  const auto& result2 = sut.template resolve<std::shared_ptr<Type>>();

  ASSERT_EQ(result1.use_count(), result2.use_count());
  ASSERT_EQ(result1.use_count(), 3);
  ASSERT_EQ(result1.get(), result2.get());
}

TEST_F(IntegrationTestSingleton, resolves_same_const_shared_ptr) {
  struct Type : Singleton {};
  auto sut = Container{bind<Type>().in<scope::Singleton>()};

  const auto& result1 = sut.template resolve<const std::shared_ptr<Type>>();
  const auto& result2 = sut.template resolve<const std::shared_ptr<Type>>();

  ASSERT_EQ(result1.use_count(), result2.use_count());
  ASSERT_EQ(result1.use_count(), 3);
  ASSERT_EQ(result1.get(), result2.get());
}

TEST_F(IntegrationTestSingleton, resolves_same_shared_ptr_to_const) {
  struct Type : Singleton {};
  auto sut = Container{bind<Type>().in<scope::Singleton>()};

  const auto& result1 = sut.template resolve<std::shared_ptr<const Type>>();
  const auto& result2 = sut.template resolve<std::shared_ptr<const Type>>();

  ASSERT_EQ(result1.use_count(), result2.use_count());
  ASSERT_EQ(result1.use_count(), 3);
  ASSERT_EQ(result1.get(), result2.get());
}

TEST_F(IntegrationTestSingleton, resolves_same_const_shared_ptr_to_const) {
  struct Type : Singleton {};
  auto sut = Container{bind<Type>().in<scope::Singleton>()};

  const auto& result1 =
      sut.template resolve<const std::shared_ptr<const Type>>();
  const auto& result2 =
      sut.template resolve<const std::shared_ptr<const Type>>();

  ASSERT_EQ(result1.use_count(), result2.use_count());
  ASSERT_EQ(result1.use_count(), 3);
  ASSERT_EQ(result1.get(), result2.get());
}

TEST_F(IntegrationTestSingleton, resolves_same_weak_ptr) {
  struct Type : Singleton {};
  auto sut = Container{bind<Type>().in<scope::Singleton>()};

  const auto& result1 = sut.template resolve<std::weak_ptr<Type>>();
  const auto& result2 = sut.template resolve<std::weak_ptr<Type>>();

  ASSERT_EQ(result1.use_count(), result2.use_count());
  ASSERT_EQ(result1.use_count(), 1);
  ASSERT_EQ(result1.lock(), result2.lock());
}

TEST_F(IntegrationTestSingleton, resolves_same_const_weak_ptr) {
  struct Type : Singleton {};
  auto sut = Container{bind<Type>().in<scope::Singleton>()};

  const auto& result1 = sut.template resolve<const std::weak_ptr<Type>>();
  const auto& result2 = sut.template resolve<const std::weak_ptr<Type>>();

  ASSERT_EQ(result1.use_count(), result2.use_count());
  ASSERT_EQ(result1.use_count(), 1);
  ASSERT_EQ(result1.lock(), result2.lock());
}

TEST_F(IntegrationTestSingleton, resolves_same_weak_ptr_to_const) {
  struct Type : Singleton {};
  auto sut = Container{bind<Type>().in<scope::Singleton>()};

  const auto& result1 = sut.template resolve<std::weak_ptr<const Type>>();
  const auto& result2 = sut.template resolve<std::weak_ptr<const Type>>();

  ASSERT_EQ(result1.use_count(), result2.use_count());
  ASSERT_EQ(result1.use_count(), 1);
  ASSERT_EQ(result1.lock(), result2.lock());
}

TEST_F(IntegrationTestSingleton, resolves_same_const_weak_ptr_to_const) {
  struct Type : Singleton {};
  auto sut = Container{bind<Type>().in<scope::Singleton>()};

  const auto& result1 = sut.template resolve<const std::weak_ptr<const Type>>();
  const auto& result2 = sut.template resolve<const std::weak_ptr<const Type>>();

  ASSERT_EQ(result1.use_count(), result2.use_count());
  ASSERT_EQ(result1.use_count(), 1);
  ASSERT_EQ(result1.lock(), result2.lock());
}

TEST_F(IntegrationTestSingleton, resolves_same_reference_to_shared_ptr) {
  struct Type : Singleton {};
  auto sut = Container{bind<Type>().in<scope::Singleton>()};

  const auto& result1 = sut.template resolve<std::shared_ptr<Type>&>();
  const auto& result2 = sut.template resolve<std::shared_ptr<Type>&>();

  ASSERT_EQ(&result1, &result2);
  ASSERT_EQ(result1.use_count(), result2.use_count());
  ASSERT_EQ(result1.use_count(), 1);
}

TEST_F(IntegrationTestSingleton, resolves_same_reference_to_const_shared_ptr) {
  struct Type : Singleton {};
  auto sut = Container{bind<Type>().in<scope::Singleton>()};

  const auto& result1 = sut.template resolve<const std::shared_ptr<Type>&>();
  const auto& result2 = sut.template resolve<const std::shared_ptr<Type>&>();

  ASSERT_EQ(&result1, &result2);
  ASSERT_EQ(result1.use_count(), result2.use_count());
  ASSERT_EQ(result1.use_count(), 1);
}

// Mutation & State
// ----------------------------------------------------------------------------

TEST_F(IntegrationTestSingleton, mutations_through_reference_are_visible) {
  struct Type : Singleton {};
  auto sut = Container{bind<Type>().in<scope::Singleton>()};

  auto& ref1 = sut.template resolve<Type&>();
  ASSERT_EQ(kInitialValue, ref1.value);

  ref1.value = kModifiedValue;

  auto& ref2 = sut.template resolve<Type&>();
  EXPECT_EQ(kModifiedValue, ref2.value);
}

TEST_F(IntegrationTestSingleton, mutations_through_pointer_are_visible) {
  struct Type : Singleton {};
  auto sut = Container{bind<Type>().in<scope::Singleton>()};

  auto* ptr1 = sut.template resolve<Type*>();
  ASSERT_EQ(kInitialValue, ptr1->value);

  ptr1->value = kModifiedValue;

  auto* ptr2 = sut.template resolve<Type*>();
  EXPECT_EQ(kModifiedValue, ptr2->value);
}

// Value & Copy Independence
// ----------------------------------------------------------------------------

TEST_F(IntegrationTestSingleton,
       value_resolves_independent_copies_of_instance) {
  struct Type : Singleton {};
  auto sut = Container{bind<Type>().in<scope::Singleton>()};

  auto val1 = sut.template resolve<Type>();
  auto val2 = sut.template resolve<Type>();
  ASSERT_NE(&val1, &val2);

  // Mutate copies
  val1.value = kModifiedValue;
  val2.value = kModifiedValue + 1;

  // Ensure original is unchanged
  auto& ref = sut.template resolve<Type&>();
  EXPECT_EQ(kInitialValue, ref.value);
  EXPECT_EQ(kModifiedValue, val1.value);
  EXPECT_EQ(kModifiedValue + 1, val2.value);
}

TEST_F(IntegrationTestSingleton,
       rvalue_reference_resolves_independent_copies_of_instance) {
  struct Type : Singleton {};
  auto sut = Container{bind<Type>().in<scope::Singleton>()};

  auto&& val1 = sut.template resolve<Type&&>();
  auto&& val2 = sut.template resolve<Type&&>();
  ASSERT_NE(&val1, &val2);

  // Mutate copies
  val1.value = kModifiedValue;
  val2.value = kModifiedValue + 1;

  // Ensure original is unchanged
  auto& ref = sut.template resolve<Type&>();
  EXPECT_EQ(kInitialValue, ref.value);
  EXPECT_EQ(kModifiedValue, val1.value);
  EXPECT_EQ(kModifiedValue + 1, val2.value);
}

TEST_F(IntegrationTestSingleton,
       unique_ptr_resolves_independent_copies_of_instance) {
  struct Type : Singleton {};
  auto sut = Container{bind<Type>().in<scope::Singleton>()};

  auto val1 = sut.template resolve<std::unique_ptr<Type>>();
  auto val2 = sut.template resolve<std::unique_ptr<Type>>();
  ASSERT_NE(val1.get(), val2.get());

  // Mutate copies
  val1->value = kModifiedValue;
  val2->value = kModifiedValue + 1;

  // Ensure original is unchanged
  auto& ref = sut.template resolve<Type&>();
  EXPECT_EQ(kInitialValue, ref.value);
  EXPECT_EQ(kModifiedValue, val1->value);
  EXPECT_EQ(kModifiedValue + 1, val2->value);
}

// Multiple Bindings
// ----------------------------------------------------------------------------

TEST_F(IntegrationTestSingleton, multiple_singleton_types) {
  struct Type1 : Singleton {};
  struct Type2 : Singleton {};

  auto sut = Container{bind<Type1>().in<scope::Singleton>(),
                       bind<Type2>().in<scope::Singleton>()};

  auto shared1 = sut.template resolve<std::shared_ptr<Type1>>();
  auto shared2 = sut.template resolve<std::shared_ptr<Type2>>();

  EXPECT_NE(shared1.get(), nullptr);
  EXPECT_NE(shared2.get(), nullptr);
}

// ----------------------------------------------------------------------------
// Instance Scope Tests (External References)
// ----------------------------------------------------------------------------

struct IntegrationTestInstance : IntegrationTest {
  using Instance = Initialized;
  Instance external;

  using Sut = Container<
      Config<Binding<Instance, scope::Instance, provider::External<Instance>>>>;
  Sut sut = Container{bind<Instance>().to(external)};
};

// Resolution
// ----------------------------------------------------------------------------

TEST_F(IntegrationTestInstance, resolves_mutable_reference) {
  auto& ref = sut.template resolve<Instance&>();
  EXPECT_EQ(&external, &ref);
}

TEST_F(IntegrationTestInstance, resolves_const_reference) {
  auto sut = Container{bind<Instance>().to(external)};

  const auto& ref = sut.template resolve<const Instance&>();

  EXPECT_EQ(&external, &ref);
}

TEST_F(IntegrationTestInstance, resolves_mutable_pointer) {
  auto sut = Container{bind<Instance>().to(external)};

  auto* ptr = sut.template resolve<Instance*>();

  EXPECT_EQ(&external, ptr);
}

TEST_F(IntegrationTestInstance, resolves_const_pointer) {
  auto sut = Container{bind<Instance>().to(external)};

  const auto* ptr = sut.template resolve<const Instance*>();

  EXPECT_EQ(&external, ptr);
}

TEST_F(IntegrationTestInstance, shared_ptr_wraps_external_instance) {
  // shared_ptr should wrap the external instance.
  auto shared = sut.template resolve<std::shared_ptr<Instance>>();
  EXPECT_EQ(&external, shared.get());
}

// Shared Pointer Caching
// ----------------------------------------------------------------------------

TEST_F(IntegrationTestInstance, shared_ptr_aliases_same_instance) {
  auto shared1 = sut.template resolve<std::shared_ptr<Instance>>();
  auto shared2 = sut.template resolve<std::shared_ptr<Instance>>();

  EXPECT_EQ(shared1.get(), shared2.get());
  EXPECT_EQ(&external, shared1.get());
}

TEST_F(IntegrationTestInstance, weak_ptr_observes_external_instance) {
  const auto weak = sut.template resolve<std::weak_ptr<Instance>>();

  EXPECT_FALSE(weak.expired());

  auto shared = weak.lock();
  EXPECT_NE(nullptr, shared);
}

TEST_F(IntegrationTestInstance, weak_ptr_expires_with_cached_shared_ptr) {
  // Resolve reference directly to cached shared_ptr.
  auto& cached_shared_ptr = sut.template resolve<std::shared_ptr<Instance>&>();
  const auto weak = sut.template resolve<std::weak_ptr<Instance>>();

  EXPECT_FALSE(weak.expired());
  cached_shared_ptr.reset();
  EXPECT_TRUE(weak.expired());
}

// Mutation & State
// ----------------------------------------------------------------------------

TEST_F(IntegrationTestInstance, mutations_through_reference_are_visible) {
  auto& ref = sut.template resolve<Instance&>();
  ref.value = kModifiedValue;
  EXPECT_EQ(kModifiedValue, external.value);
}

TEST_F(IntegrationTestInstance, mutations_through_pointer_are_visible) {
  auto* ptr = sut.template resolve<Instance*>();
  ptr->value = kModifiedValue;
  EXPECT_EQ(kModifiedValue, external.value);
}

TEST_F(IntegrationTestInstance,
       mutations_to_external_instance_are_visible_in_reference) {
  auto& ref = sut.template resolve<Instance&>();
  external.value = kModifiedValue;
  EXPECT_EQ(kModifiedValue, ref.value);
}

TEST_F(IntegrationTestInstance,
       mutations_to_external_instance_are_visible_in_pointer) {
  auto* ptr = sut.template resolve<Instance*>();
  external.value = kModifiedValue;
  EXPECT_EQ(kModifiedValue, ptr->value);
}

// Value & Copy Independence
// ----------------------------------------------------------------------------

TEST_F(IntegrationTestInstance, resolves_value_copy_of_external) {
  external.value = kModifiedValue;

  auto copy = sut.template resolve<Instance>();
  EXPECT_EQ(kModifiedValue, copy.value);

  // Verify it's a copy, not the original.
  copy.value *= 2;
  EXPECT_EQ(kModifiedValue, external.value);
}

TEST_F(IntegrationTestInstance,
       value_resolves_are_independent_copies_of_instance) {
  auto copy1 = sut.template resolve<Instance>();
  auto copy2 = sut.template resolve<Instance>();

  // Copies are independent.
  copy1.value = kModifiedValue;
  copy2.value *= 2;
  EXPECT_EQ(kModifiedValue, copy1.value);
  EXPECT_EQ(kInitialValue * 2, copy2.value);
  EXPECT_EQ(kInitialValue, external.value);
}

TEST_F(IntegrationTestInstance,
       rvalue_reference_resolves_are_independent_copies_of_instance) {
  auto&& value_copy = sut.template resolve<Instance&&>();
  value_copy.value = kModifiedValue;
  EXPECT_EQ(kInitialValue, external.value);
  EXPECT_NE(kModifiedValue, external.value);
}

TEST_F(IntegrationTestInstance,
       unique_ptr_resolves_are_independent_copies_of_instance) {
  auto value_copy = sut.template resolve<std::unique_ptr<Instance>>();
  value_copy->value = kModifiedValue;
  EXPECT_EQ(kInitialValue, external.value);
  EXPECT_NE(kModifiedValue, external.value);
}

// =============================================================================
// PROMOTION - Transient Becomes Singleton-Like
// When reference-requesting resolution types promote transients to singletons
// =============================================================================

// ----------------------------------------------------------------------------
// Promotion Tests
// ----------------------------------------------------------------------------

struct IntegrationTestPromotion : IntegrationTest {};

TEST_F(IntegrationTestPromotion, values_not_promoted) {
  using Type = Initialized;
  auto sut = Container{bind<Type>().in<scope::Transient>()};

  auto val1 = sut.template resolve<Type>();
  auto val2 = sut.template resolve<Type>();

  EXPECT_EQ(0, val1.id);
  EXPECT_EQ(1, val2.id);
  EXPECT_EQ(2, Counted::num_instances);
}

TEST_F(IntegrationTestPromotion, rvalue_references_not_promoted) {
  using Type = Initialized;
  auto sut = Container{bind<Type>().in<scope::Transient>()};

  auto val1 = sut.template resolve<Type&&>();
  auto val2 = sut.template resolve<Type&&>();

  EXPECT_EQ(0, val1.id);
  EXPECT_EQ(1, val2.id);
  EXPECT_EQ(2, Counted::num_instances);
}

TEST_F(IntegrationTestPromotion, references_are_promoted) {
  struct Type : Singleton {};
  auto sut = Container{bind<Type>().in<scope::Transient>()};

  auto& ref1 = sut.template resolve<Type&>();
  auto& ref2 = sut.template resolve<Type&>();

  EXPECT_EQ(&ref1, &ref2);
  EXPECT_EQ(0, ref1.id);
  EXPECT_EQ(1, Counted::num_instances);
}

TEST_F(IntegrationTestPromotion, references_to_const_are_promoted) {
  struct Type : Singleton {};
  auto sut = Container{bind<Type>().in<scope::Transient>()};

  const auto& ref1 = sut.template resolve<const Type&>();
  const auto& ref2 = sut.template resolve<const Type&>();

  EXPECT_EQ(&ref1, &ref2);
  EXPECT_EQ(0, ref1.id);
  EXPECT_EQ(1, Counted::num_instances);
}

TEST_F(IntegrationTestPromotion, pointers_are_promoted) {
  struct Type : Singleton {};
  auto sut = Container{bind<Type>().in<scope::Transient>()};

  auto* ptr1 = sut.template resolve<Type*>();
  auto* ptr2 = sut.template resolve<Type*>();

  EXPECT_EQ(ptr1, ptr2);
  EXPECT_EQ(0, ptr1->id);
  EXPECT_EQ(1, Counted::num_instances);
}

TEST_F(IntegrationTestPromotion, pointers_to_const_are_promoted) {
  struct Type : Singleton {};
  auto sut = Container{bind<Type>().in<scope::Transient>()};

  const auto* ptr1 = sut.template resolve<const Type*>();
  const auto* ptr2 = sut.template resolve<const Type*>();

  EXPECT_EQ(ptr1, ptr2);
  EXPECT_EQ(0, ptr1->id);
  EXPECT_EQ(1, Counted::num_instances);
}

TEST_F(IntegrationTestPromotion, shared_ptrs_not_promoted) {
  using Type = Initialized;
  auto sut = Container{bind<Type>().in<scope::Transient>()};

  auto shared1 = sut.template resolve<std::shared_ptr<Type>>();
  auto shared2 = sut.template resolve<std::shared_ptr<Type>>();

  EXPECT_NE(shared1.get(), shared2.get());
  EXPECT_EQ(0, shared1->id);
  EXPECT_EQ(1, shared2->id);
  EXPECT_EQ(2, Counted::num_instances);
}

TEST_F(IntegrationTestPromotion, weak_ptrs_are_promoted) {
  struct Type : Singleton {};
  auto sut = Container{bind<Type>().in<scope::Transient>()};

  auto weak1 = sut.template resolve<std::weak_ptr<Type>>();
  auto weak2 = sut.template resolve<std::weak_ptr<Type>>();

  EXPECT_FALSE(weak1.expired());
  EXPECT_EQ(weak1.lock(), weak2.lock());
  EXPECT_EQ(0, weak1.lock()->id);
  EXPECT_EQ(1, Counted::num_instances);
}

TEST_F(IntegrationTestPromotion, unique_ptrs_not_promoted) {
  using Type = Initialized;
  auto sut = Container{bind<Type>().in<scope::Transient>()};

  auto unique1 = sut.template resolve<std::unique_ptr<Type>>();
  auto unique2 = sut.template resolve<std::unique_ptr<Type>>();

  EXPECT_NE(unique1.get(), unique2.get());
  EXPECT_EQ(0, unique1->id);
  EXPECT_EQ(1, unique2->id);
  EXPECT_EQ(2, Counted::num_instances);
}

TEST_F(IntegrationTestPromotion, multiple_promotions_different_requests) {
  struct Type : Singleton {};
  auto sut = Container{bind<Type>().in<scope::Transient>()};

  auto& ref = sut.template resolve<Type&>();
  auto* ptr = sut.template resolve<Type*>();
  auto weak = sut.template resolve<std::weak_ptr<Type>>();

  EXPECT_EQ(&ref, ptr);
  EXPECT_EQ(ptr, weak.lock().get());
  EXPECT_EQ(0, ref.id);
  EXPECT_EQ(1, Counted::num_instances);
}

TEST_F(IntegrationTestPromotion, promotion_with_dependencies) {
  struct Dependency : Counted {};
  struct Service : Counted {
    Dependency* dep;
    explicit Service(Dependency& d) : dep{&d} {}
  };

  auto sut = Container{bind<Dependency>().in<scope::Transient>(),
                       bind<Service>().in<scope::Transient>()};

  auto& service = sut.template resolve<Service&>();
  auto& service2 = sut.template resolve<Service&>();

  EXPECT_EQ(0, service.dep->id);
  EXPECT_EQ(1, service.id);

  EXPECT_EQ(&service, &service2);
  EXPECT_EQ(service.dep, service2.dep);

  EXPECT_EQ(2, Counted::num_instances);  // 1 Service + 1 Dependency
}

TEST_F(IntegrationTestPromotion, unbound_type_reference_is_promoted) {
  struct Type : Counted {};

  auto sut = Container{};

  auto& ref1 = sut.template resolve<Type&>();
  auto& ref2 = sut.template resolve<Type&>();

  EXPECT_EQ(&ref1, &ref2);
  EXPECT_EQ(0, ref1.id);
  EXPECT_EQ(1, Counted::num_instances);
}

// =============================================================================
// VALUE RESOLUTION FROM REFERENCE SCOPES
// Requesting values from singleton/promoted types yields copies
// =============================================================================

// ----------------------------------------------------------------------------
// Value Resolution Tests
// ----------------------------------------------------------------------------

struct IntegrationTestValueResolution : IntegrationTest {};

TEST_F(IntegrationTestValueResolution, values_are_copies_of_singleton) {
  struct Type : Singleton {};
  auto sut = Container{bind<Type>().in<scope::Singleton>()};

  // Values return copies of the singleton.
  auto val1 = sut.template resolve<Type>();
  auto val2 = sut.template resolve<Type>();

  EXPECT_NE(&val1, &val2);
  EXPECT_EQ(0, val1.id);
  EXPECT_EQ(0, val2.id);
  EXPECT_EQ(1, Counted::num_instances);
}

TEST_F(IntegrationTestValueResolution,
       rvalue_references_are_copies_of_singleton) {
  struct Type : Singleton {};
  auto sut = Container{bind<Type>().in<scope::Singleton>()};

  auto&& rval1 = sut.template resolve<Type&&>();
  auto&& rval2 = sut.template resolve<Type&&>();

  EXPECT_NE(&rval1, &rval2);
  EXPECT_EQ(0, rval1.id);
  EXPECT_EQ(0, rval2.id);
  EXPECT_EQ(1, Counted::num_instances);
}

TEST_F(IntegrationTestValueResolution, unique_ptrs_are_copies_of_singleton) {
  struct Type : Singleton {};
  auto sut = Container{bind<Type>().in<scope::Singleton>()};

  auto unique1 = sut.template resolve<std::unique_ptr<Type>>();
  auto unique2 = sut.template resolve<std::unique_ptr<Type>>();

  EXPECT_NE(unique1.get(), unique2.get());
  EXPECT_EQ(0, unique1->id);
  EXPECT_EQ(0, unique2->id);
  EXPECT_EQ(1, Counted::num_instances);
}

TEST_F(IntegrationTestValueResolution, references_not_copied) {
  struct Type : Singleton {};
  auto sut = Container{bind<Type>().in<scope::Singleton>()};

  // References should still be singleton
  auto& ref1 = sut.template resolve<Type&>();
  auto& ref2 = sut.template resolve<Type&>();

  EXPECT_EQ(&ref1, &ref2);
  EXPECT_EQ(0, ref1.id);
  EXPECT_EQ(1, Counted::num_instances);
}

TEST_F(IntegrationTestValueResolution, pointers_not_copied) {
  struct Type : Singleton {};
  auto sut = Container{bind<Type>().in<scope::Singleton>()};

  // Pointers should still be singleton
  auto* ptr1 = sut.template resolve<Type*>();
  auto* ptr2 = sut.template resolve<Type*>();

  EXPECT_EQ(ptr1, ptr2);
  EXPECT_EQ(0, ptr1->id);
  EXPECT_EQ(1, Counted::num_instances);
}

TEST_F(IntegrationTestValueResolution, shared_ptr_not_copied) {
  struct Type : Singleton {};
  auto sut = Container{bind<Type>().in<scope::Singleton>()};

  auto shared1 = sut.template resolve<std::shared_ptr<Type>>();
  auto shared2 = sut.template resolve<std::shared_ptr<Type>>();
  auto& ref = sut.template resolve<Type&>();

  EXPECT_EQ(shared1.get(), shared2.get());
  EXPECT_EQ(&ref, shared1.get());
  EXPECT_EQ(0, ref.id);
  EXPECT_EQ(1, Counted::num_instances);
}

TEST_F(IntegrationTestValueResolution,
       singleton_shared_ptr_wraps_singleton_reference) {
  struct Type : Singleton {};
  auto sut = Container{bind<Type>().in<scope::Singleton>()};

  // Modify the singleton.
  auto& singleton = sut.template resolve<Type&>();
  singleton.value = kModifiedValue;

  // shared_ptr should wrap the singleton, showing the modified value.
  auto shared = sut.template resolve<std::shared_ptr<Type>>();
  EXPECT_EQ(kModifiedValue, shared->value);
  EXPECT_EQ(&singleton, shared.get());

  // Values are copies of the singleton with the modified value.
  auto val = sut.template resolve<Type>();
  EXPECT_EQ(kModifiedValue, val.value);  // Copy of modified singleton
  EXPECT_NE(&singleton, &val);           // But different address

  EXPECT_EQ(1, Counted::num_instances);  // Only 1 singleton instance
}

TEST_F(IntegrationTestValueResolution,
       value_copies_reflect_singleton_state_not_fresh_instances) {
  struct Type : Singleton {};
  auto sut = Container{bind<Type>().in<scope::Singleton>()};

  // Get singleton reference and modify it.
  auto& singleton = sut.template resolve<Type&>();
  singleton.value = kModifiedValue;

  // Values are copies of the singleton. This creates copies of the
  // modified singleton, not fresh instances from the provider.
  auto val1 = sut.template resolve<Type>();
  auto val2 = sut.template resolve<Type>();

  // Copies of modified singleton, not default values from provider.
  EXPECT_EQ(kModifiedValue, val1.value);
  EXPECT_EQ(kModifiedValue, val2.value);

  // Copies are independent from each other and from singleton.
  EXPECT_NE(&singleton, &val1);
  EXPECT_NE(&singleton, &val2);
  EXPECT_NE(&val1, &val2);

  // Singleton itself is unchanged.
  EXPECT_EQ(kModifiedValue, singleton.value);
}

TEST_F(IntegrationTestValueResolution, value_resolution_with_dependencies) {
  struct DependencyType : Singleton {};
  struct ServiceType : Singleton {
    DependencyType dep;
    explicit ServiceType(DependencyType d) : dep{d} {}
  };

  auto sut = Container{bind<DependencyType>().in<scope::Singleton>(),
                       bind<ServiceType>().in<scope::Singleton>()};

  // Service value is a copy of the Service singleton.
  // The Service singleton contains a copy of the Dependency singleton.
  // Each value resolution creates independent copies.
  const auto& service1 = sut.template resolve<ServiceType>();
  const auto& service2 = sut.template resolve<ServiceType>();

  EXPECT_NE(&service1, &service2);          // Independent copies
  EXPECT_NE(&service1.dep, &service2.dep);  // Each copy has its own dep copy

  // Singletons created: Dependency, then Service.
  EXPECT_EQ(0, service1.dep.id);  // Copy of Dependency singleton
  EXPECT_EQ(1, service1.id);      // Copy of Service singleton

  // Both values are copies of the same singletons.
  EXPECT_EQ(0, service2.dep.id);         // Copy of same Dependency singleton
  EXPECT_EQ(1, service2.id);             // Copy of same Service singleton
  EXPECT_EQ(2, Counted::num_instances);  // 1 Service + 1 Dependency
}

// =============================================================================
// PROVIDERS - How Instances Are Created
// Constructor, factory, and external instance providers
// =============================================================================

// ----------------------------------------------------------------------------
// Factory Provider Tests
// ----------------------------------------------------------------------------

struct IntegrationTestFactoryProvider : IntegrationTest {
  struct Factory {
    auto operator()() const -> Product { return Product{kInitialValue}; }
  };
  Factory factory;
};

// Resolution
// ----------------------------------------------------------------------------

TEST_F(IntegrationTestFactoryProvider, resolves_with_factory) {
  auto sut = Container{bind<Product>().via(factory)};

  auto value = sut.template resolve<Product>();
  EXPECT_EQ(kInitialValue, value.value);
}

TEST_F(IntegrationTestFactoryProvider, factory_with_parameters_from_container) {
  struct ProductWithDep {
    int_t combined_value;
    explicit ProductWithDep(Dependency dep) : combined_value{dep.value * 2} {}
  };

  auto factory = [](Dependency dep) { return ProductWithDep{dep}; };

  auto sut = Container{bind<Dependency>(), bind<ProductWithDep>().via(factory)};

  auto product = sut.template resolve<ProductWithDep>();
  EXPECT_EQ(kInitialValue * 2, product.combined_value);
}

// Scope Interaction
// ----------------------------------------------------------------------------

TEST_F(IntegrationTestFactoryProvider, factory_with_singleton_scope) {
  auto sut = Container{bind<Product>().via(factory).in<scope::Singleton>()};

  auto& ref1 = sut.template resolve<Product&>();
  auto& ref2 = sut.template resolve<Product&>();

  EXPECT_EQ(&ref1, &ref2);
  EXPECT_EQ(0, ref1.id);
  EXPECT_EQ(1, Counted::num_instances);
}

TEST_F(IntegrationTestFactoryProvider, factory_with_transient_scope) {
  auto sut = Container{bind<Product>().via(factory).in<scope::Transient>()};

  auto value1 = sut.template resolve<Product>();
  auto value2 = sut.template resolve<Product>();

  EXPECT_EQ(0, value1.id);
  EXPECT_EQ(1, value2.id);
  EXPECT_EQ(2, Counted::num_instances);
}

TEST_F(IntegrationTestFactoryProvider,
       factory_with_transient_scope_and_promoted_ref) {
  auto sut = Container{bind<Product>().via(factory)};

  auto value = sut.template resolve<Product>();
  auto& ref = sut.template resolve<Product&>();

  EXPECT_EQ(kInitialValue, value.value);
  EXPECT_EQ(kInitialValue, ref.value);
  EXPECT_NE(value.id, ref.id);
  EXPECT_EQ(0, value.id);
  EXPECT_EQ(1, ref.id);
  EXPECT_EQ(2, Counted::num_instances);
}

// =============================================================================
// DEPENDENCY INJECTION
// Automatic resolution of constructor dependencies
// =============================================================================

// ----------------------------------------------------------------------------
// Dependency Injection Tests
// ----------------------------------------------------------------------------

struct IntegrationTestDependencyInjection : IntegrationTest {};

// Simple Injection
// ----------------------------------------------------------------------------

TEST_F(IntegrationTestDependencyInjection, resolves_single_dependency) {
  struct Service {
    int_t result;
    explicit Service(Dependency dep) : result{dep.value * 2} {}
  };

  auto sut = Container{bind<Dependency>(), bind<Service>()};

  auto service = sut.template resolve<Service>();
  EXPECT_EQ(kInitialValue * 2, service.result);
}

TEST_F(IntegrationTestDependencyInjection, resolves_multiple_dependencies) {
  struct Service {
    int_t sum;
    Service(Dep1 d1, Dep2 d2) : sum{d1.value + d2.value} {}
  };

  auto sut = Container{bind<Dep1>(), bind<Dep2>(), bind<Service>()};

  auto service = sut.template resolve<Service>();
  EXPECT_EQ(3, service.sum);  // 1 + 2
}

TEST_F(IntegrationTestDependencyInjection, resolves_dependency_chain) {
  struct Dep1 {
    int_t value = 3;
    Dep1() = default;
  };

  struct Dep2 {
    int_t value;
    explicit Dep2(Dep1 d1) : value{d1.value * 5} {}
  };

  struct Service {
    int_t value;
    explicit Service(Dep2 d2) : value{d2.value * 7} {}
  };

  auto sut = Container{bind<Dep1>(), bind<Dep2>(), bind<Service>()};

  auto service = sut.template resolve<Service>();
  EXPECT_EQ(105, service.value);  // 3 * 5 * 7
}

// Value Category Injection
// ----------------------------------------------------------------------------

TEST_F(IntegrationTestDependencyInjection, resolves_dependency_as_reference) {
  struct Service {
    Dependency* dep_ptr;
    explicit Service(Dependency& dep) : dep_ptr{&dep} {}
  };

  auto sut =
      Container{bind<Dependency>().in<scope::Singleton>(), bind<Service>()};

  auto service = sut.template resolve<Service>();
  auto& dep = sut.template resolve<Dependency&>();

  EXPECT_EQ(&dep, service.dep_ptr);
  EXPECT_EQ(kInitialValue, service.dep_ptr->value);
}

TEST_F(IntegrationTestDependencyInjection,
       resolves_dependency_as_const_reference) {
  struct Service {
    int_t copied_value;
    explicit Service(const Dependency& dep) : copied_value{dep.value} {}
  };

  auto sut = Container{bind<Dependency>(), bind<Service>()};

  auto service = sut.template resolve<Service>();
  EXPECT_EQ(kInitialValue, service.copied_value);
}

TEST_F(IntegrationTestDependencyInjection, resolves_dependency_as_shared_ptr) {
  struct Service {
    std::shared_ptr<Dependency> dep;
    explicit Service(std::shared_ptr<Dependency> d) : dep{std::move(d)} {}
  };

  auto sut =
      Container{bind<Dependency>().in<scope::Singleton>(), bind<Service>()};

  auto service = sut.template resolve<Service>();
  EXPECT_EQ(kInitialValue, service.dep->value);
  EXPECT_EQ(2, service.dep.use_count());  // cached + service.dep
}

TEST_F(IntegrationTestDependencyInjection, resolves_dependency_as_unique_ptr) {
  struct Service {
    std::unique_ptr<Dependency> dep;
    explicit Service(std::unique_ptr<Dependency> d) : dep{std::move(d)} {}
  };

  auto sut =
      Container{bind<Dependency>().in<scope::Transient>(), bind<Service>()};

  auto service = sut.template resolve<Service>();
  EXPECT_EQ(kInitialValue, service.dep->value);
}

TEST_F(IntegrationTestDependencyInjection, resolves_dependency_as_pointer) {
  struct Service {
    Dependency* dep;
    explicit Service(Dependency* d) : dep{d} {}
  };

  auto sut =
      Container{bind<Dependency>().in<scope::Singleton>(), bind<Service>()};

  auto service = sut.template resolve<Service>();
  auto* dep = sut.template resolve<Dependency*>();

  EXPECT_EQ(dep, service.dep);
  EXPECT_EQ(kInitialValue, service.dep->value);
}

TEST_F(IntegrationTestDependencyInjection, mixed_dependency_types) {
  struct Service {
    int_t sum;
    Service(Dep1 d1, const Dep2& d2, Dep3* d3)
        : sum{d1.value + d2.value + d3->value} {}
  };

  auto sut = Container{bind<Dep1>(), bind<Dep2>(),
                       bind<Dep3>().in<scope::Singleton>(), bind<Service>()};

  auto service = sut.template resolve<Service>();
  EXPECT_EQ(6, service.sum);  // 1 + 2 + 3
}

// Scope Interaction
// ----------------------------------------------------------------------------

TEST_F(IntegrationTestDependencyInjection,
       singleton_dependency_shared_across_services) {
  struct Service1 {
    Dep1* dep;
    explicit Service1(Dep1& d) : dep{&d} {}
  };

  struct Service2 {
    Dep1* dep;
    explicit Service2(Dep1& d) : dep{&d} {}
  };

  auto sut = Container{bind<Dep1>().in<scope::Singleton>(), bind<Service1>(),
                       bind<Service2>()};

  auto service1 = sut.template resolve<Service1>();
  auto service2 = sut.template resolve<Service2>();

  EXPECT_EQ(service1.dep, service2.dep);
  EXPECT_EQ(0, service1.dep->id);
  EXPECT_EQ(1, Counted::num_instances);
}

TEST_F(IntegrationTestDependencyInjection,
       mixed_value_categories_in_constructor) {
  struct SingletonType : Singleton {};
  struct TransientType : Initialized {};

  struct Service {
    int_t sum;
    Service(SingletonType& s, TransientType t) : sum{s.value + t.value} {}
  };

  auto sut =
      Container{bind<SingletonType>().in<scope::Singleton>(),
                bind<TransientType>().in<scope::Transient>(), bind<Service>()};

  auto service = sut.template resolve<Service>();
  EXPECT_EQ(kInitialValue + kInitialValue, service.sum);
}

// =============================================================================
// POLYMORPHISM - Interfaces and Implementations
// Binding interfaces to concrete implementations
// =============================================================================

// ----------------------------------------------------------------------------
// Interface Binding Tests
// ----------------------------------------------------------------------------

struct IntegrationTestInterface : IntegrationTest {};

// Resolution
// ----------------------------------------------------------------------------

TEST_F(IntegrationTestInterface, binds_interface_to_implementation) {
  struct Service : IService {
    int_t get_value() const override { return kInitialValue; }
  };

  auto sut = Container{bind<IService>().as<Service>()};

  auto& service = sut.template resolve<IService&>();
  EXPECT_EQ(kInitialValue, service.get_value());
}

TEST_F(IntegrationTestInterface, resolves_implementation_directly) {
  struct Service : IService {
    int_t get_value() const override { return kInitialValue; }
  };

  auto sut = Container{bind<IService>().as<Service>()};

  // Can still resolve Service directly.
  auto& impl = sut.template resolve<Service&>();
  EXPECT_EQ(kInitialValue, impl.get_value());
}

// Scope Interaction
// ----------------------------------------------------------------------------

TEST_F(IntegrationTestInterface, interface_binding_with_singleton_scope) {
  struct Service : IService, Counted {
    int_t get_value() const override { return id; }
  };

  auto sut = Container{bind<IService>().as<Service>().in<scope::Singleton>()};

  auto& ref1 = sut.template resolve<IService&>();
  auto& ref2 = sut.template resolve<IService&>();

  EXPECT_EQ(&ref1, &ref2);
  EXPECT_EQ(0, ref1.get_value());
}

// Provider Interaction
// ----------------------------------------------------------------------------

TEST_F(IntegrationTestInterface, interface_binding_with_factory) {
  struct Service : IService {
    int_t value;
    explicit Service(int_t value) : value{value} {}
    int_t get_value() const override { return value; }
  };

  auto factory = []() { return Service{kModifiedValue}; };

  auto sut = Container{bind<IService>().as<Service>().via(factory)};

  auto& service = sut.template resolve<IService&>();
  EXPECT_EQ(kModifiedValue, service.get_value());
}

// Multiple Bindings
// ----------------------------------------------------------------------------

TEST_F(IntegrationTestInterface, multiple_interfaces_to_implementations) {
  struct IService2 {
    virtual ~IService2() = default;
    virtual int_t get_value() const = 0;
  };

  struct Service1 : IService {
    int_t get_value() const override { return 1; }
  };
  struct Service2 : IService2 {
    int_t get_value() const override { return 2; }
  };

  auto sut = Container{bind<IService>().as<Service1>(),
                       bind<IService2>().as<Service2>()};

  auto& service1 = sut.template resolve<IService&>();
  auto& service2 = sut.template resolve<IService2&>();

  EXPECT_EQ(1, service1.get_value());
  EXPECT_EQ(2, service2.get_value());
}

// ----------------------------------------------------------------------------
// Multiple Interface Binding Tests
// ----------------------------------------------------------------------------

// Caching is keyed on the To type, not the From type, so multiple interfaces
// to the same type will return the same instance.
struct IntegrationTestMultipleInheritance : IntegrationTest {
  struct IService2 {
    virtual ~IService2() = default;
    virtual int_t get_value2() const = 0;
  };

  struct Service : IService, IService2 {
    int_t get_value() const override { return 1; }
    int_t get_value2() const override { return 2; }
  };
};

TEST_F(IntegrationTestMultipleInheritance, same_impl_same_instance_singleton) {
  auto sut = Container{bind<IService>().as<Service>().in<scope::Singleton>(),
                       bind<IService2>().as<Service>().in<scope::Singleton>()};

  auto& service1 = sut.template resolve<IService&>();
  auto& service2 = sut.template resolve<IService2&>();

  ASSERT_EQ(dynamic_cast<Service*>(&service1),
            dynamic_cast<Service*>(&service2));

  EXPECT_EQ(1, service1.get_value());
  EXPECT_EQ(2, service2.get_value2());
}

TEST_F(IntegrationTestMultipleInheritance,
       same_impl_same_instance_transient_promotion) {
  auto sut = Container{bind<IService>().as<Service>(),
                       bind<IService2>().as<Service>()};

  auto& service1 = sut.template resolve<IService&>();
  auto& service2 = sut.template resolve<IService2&>();

  ASSERT_EQ(dynamic_cast<Service*>(&service1),
            dynamic_cast<Service*>(&service2));

  EXPECT_EQ(1, service1.get_value());
  EXPECT_EQ(2, service2.get_value2());
}

TEST_F(IntegrationTestMultipleInheritance,
       same_impl_same_instance_mixed_singleton_and_transient_promotion) {
  auto sut = Container{bind<IService>().as<Service>().in<scope::Singleton>(),
                       bind<IService2>().as<Service>().in<scope::Transient>()};

  auto& service1 = sut.template resolve<IService&>();
  auto& service2 = sut.template resolve<IService2&>();

  ASSERT_EQ(dynamic_cast<Service*>(&service1),
            dynamic_cast<Service*>(&service2));

  EXPECT_EQ(1, service1.get_value());
  EXPECT_EQ(2, service2.get_value2());
}

// =============================================================================
// CONTAINER HIERARCHIES - Parent/Child Relationships
// How child containers inherit and override parent bindings
// =============================================================================

// ----------------------------------------------------------------------------
// Hierarchical Container Tests - Delegation
// ----------------------------------------------------------------------------

struct IntegrationTestHierarchyDelegation : IntegrationTest {
  struct Type : Initialized {};
};

TEST_F(IntegrationTestHierarchyDelegation, child_finds_binding_in_parent) {
  auto parent = Container{bind<Type>()};
  auto child = Container{parent};

  auto result = child.template resolve<Type>();
  EXPECT_EQ(kInitialValue, result.value);
}

TEST_F(IntegrationTestHierarchyDelegation, child_overrides_parent_binding) {
  auto parent_factory = []() { return Product{kInitialValue}; };
  auto child_factory = []() { return Product{kModifiedValue}; };

  auto parent = Container{bind<Product>().via(parent_factory)};
  auto child = Container{parent, bind<Product>().via(child_factory)};

  auto parent_result = parent.template resolve<Product>();
  auto child_result = child.template resolve<Product>();

  EXPECT_EQ(kInitialValue, parent_result.value);
  EXPECT_EQ(kModifiedValue, child_result.value);
}

TEST_F(IntegrationTestHierarchyDelegation, multi_level_hierarchy) {
  struct Grandparent {
    int_t value = 1;
    Grandparent() = default;
  };
  struct Parent {
    int_t value = 2;
    Parent() = default;
  };
  struct Child {
    int_t value = 3;
    Child() = default;
  };

  auto grandparent = Container{bind<Grandparent>()};
  auto parent = Container{grandparent, bind<Parent>()};
  auto child = Container{parent, bind<Child>()};

  // Child can resolve from all levels.
  auto grandparent_result = child.template resolve<Grandparent>();
  auto parent_result = child.template resolve<Parent>();
  auto child_result = child.template resolve<Child>();

  EXPECT_EQ(1, grandparent_result.value);
  EXPECT_EQ(2, parent_result.value);
  EXPECT_EQ(3, child_result.value);
}

TEST_F(IntegrationTestHierarchyDelegation,
       multi_level_hierarchy_via_factories) {
  auto grandparent_factory = []() { return Product{1}; };
  auto parent_factory = []() { return Product{2}; };
  auto child_factory = []() { return Product{3}; };

  auto grandparent = Container{bind<Product>().via(grandparent_factory)};
  auto parent = Container{grandparent, bind<Product>().via(parent_factory)};
  auto child = Container{parent, bind<Product>().via(child_factory)};

  auto grandparent_result = grandparent.template resolve<Product>();
  auto parent_result = parent.template resolve<Product>();
  auto child_result = child.template resolve<Product>();

  EXPECT_EQ(1, grandparent_result.value);
  EXPECT_EQ(2, parent_result.value);
  EXPECT_EQ(3, child_result.value);
}

TEST_F(IntegrationTestHierarchyDelegation,
       unbound_type_uses_fallback_in_hierarchy) {
  auto parent = Container{};
  auto child = Container{parent};

  // Uses fallback binding at the root level.
  auto result = child.template resolve<Type>();
  EXPECT_EQ(kInitialValue, result.value);
}

// ----------------------------------------------------------------------------
// Hierarchical Container Tests - Singleton Sharing
// ----------------------------------------------------------------------------

struct IntegrationTestHierarchySingletonSharing : IntegrationTest {};

TEST_F(IntegrationTestHierarchySingletonSharing,
       singleton_in_parent_shared_with_child) {
  struct Type : Singleton {};

  auto parent = Container{bind<Type>().in<scope::Singleton>()};
  auto child = Container{parent};

  auto& parent_ref = parent.template resolve<Type&>();
  auto& child_ref = child.template resolve<Type&>();

  EXPECT_EQ(&parent_ref, &child_ref);
  EXPECT_EQ(0, parent_ref.id);
  EXPECT_EQ(1, Counted::num_instances);
}

TEST_F(IntegrationTestHierarchySingletonSharing,
       singleton_in_grandparent_shared_with_all) {
  struct Type : Singleton {};

  auto grandparent = Container{bind<Type>().in<scope::Singleton>()};
  auto parent = Container{grandparent};
  auto child = Container{parent};

  auto& grandparent_ref = grandparent.template resolve<Type&>();
  auto& parent_ref = parent.template resolve<Type&>();
  auto& child_ref = child.template resolve<Type&>();

  EXPECT_EQ(&grandparent_ref, &parent_ref);
  EXPECT_EQ(&parent_ref, &child_ref);
  EXPECT_EQ(0, grandparent_ref.id);
  EXPECT_EQ(1, Counted::num_instances);
}

TEST_F(IntegrationTestHierarchySingletonSharing,
       child_singleton_does_not_affect_parent) {
  struct Type : Singleton {};

  auto parent = Container{};
  auto child = Container{parent, bind<Type>().in<scope::Singleton>()};

  auto& child_ref = child.template resolve<Type&>();
  // Parent should create new instance (unbound type, promoted).
  auto& parent_ref = parent.template resolve<Type&>();

  EXPECT_NE(&child_ref, &parent_ref);
  EXPECT_EQ(0, child_ref.id);
  EXPECT_EQ(1, parent_ref.id);
  EXPECT_EQ(2, Counted::num_instances);
}

TEST_F(IntegrationTestHierarchySingletonSharing,
       parent_and_child_can_have_separate_singletons) {
  struct Type : Singleton {};

  auto parent = Container{bind<Type>().in<scope::Singleton>()};
  auto child = Container{parent, bind<Type>().in<scope::Singleton>()};

  auto& parent_ref = parent.template resolve<Type&>();
  auto& child_ref = child.template resolve<Type&>();

  // Child overrides, so they should be different.
  EXPECT_NE(&parent_ref, &child_ref);
  EXPECT_EQ(0, parent_ref.id);
  EXPECT_EQ(1, child_ref.id);
  EXPECT_EQ(2, Counted::num_instances);
}

// ----------------------------------------------------------------------------
// Hierarchical Container Tests - Transient Behavior
// ----------------------------------------------------------------------------

struct IntegrationTestHierarchyTransient : IntegrationTest {
  struct Type : Initialized {};
};

TEST_F(IntegrationTestHierarchyTransient,
       transient_in_parent_creates_new_instances_for_child) {
  auto parent = Container{bind<Type>().in<scope::Transient>()};
  auto child = Container{parent};

  auto parent_val1 = parent.template resolve<Type>();
  auto child_val1 = child.template resolve<Type>();
  auto child_val2 = child.template resolve<Type>();

  EXPECT_EQ(0, parent_val1.id);
  EXPECT_EQ(1, child_val1.id);
  EXPECT_EQ(2, child_val2.id);
  EXPECT_EQ(3, Counted::num_instances);
}

TEST_F(IntegrationTestHierarchyTransient,
       transient_in_grandparent_creates_new_instances_for_all) {
  auto grandparent = Container{bind<Type>().in<scope::Transient>()};
  auto parent = Container{grandparent};
  auto child = Container{parent};

  auto grandparent_val = grandparent.template resolve<Type>();
  auto parent_val = parent.template resolve<Type>();
  auto child_val = child.template resolve<Type>();

  EXPECT_EQ(0, grandparent_val.id);
  EXPECT_EQ(1, parent_val.id);
  EXPECT_EQ(2, child_val.id);
  EXPECT_EQ(3, Counted::num_instances);
}

// ----------------------------------------------------------------------------
// Hierarchical Container Tests - Promotion in Hierarchy
// ----------------------------------------------------------------------------

struct IntegrationTestHierarchyPromotion : IntegrationTest {};

TEST_F(IntegrationTestHierarchyPromotion,
       child_promotes_transient_from_parent) {
  struct Type : Singleton {};
  auto parent = Container{bind<Type>().in<scope::Transient>()};
  auto child = Container{parent};

  // Child requests by reference, should promote
  auto& child_ref1 = child.template resolve<Type&>();
  auto& child_ref2 = child.template resolve<Type&>();

  EXPECT_EQ(&child_ref1, &child_ref2);
  EXPECT_EQ(0, child_ref1.id);
  EXPECT_EQ(1, Counted::num_instances);
}

TEST_F(IntegrationTestHierarchyPromotion,
       child_shares_parent_promoted_instance_when_delegating) {
  struct Type : Singleton {};
  auto parent = Container{bind<Type>().in<scope::Transient>()};
  auto child = Container{parent};  // Child has no binding, will delegate

  // Parent promotes to singleton when requested by reference.
  auto& parent_ref = parent.template resolve<Type&>();

  // Child delegates to parent, gets same promoted instance.
  auto& child_ref = child.template resolve<Type&>();

  EXPECT_EQ(&parent_ref, &child_ref);  // Same instance
  EXPECT_EQ(0, parent_ref.id);
  EXPECT_EQ(1, Counted::num_instances);  // Only one instance created
}

TEST_F(IntegrationTestHierarchyPromotion,
       child_has_separate_promoted_instance_with_own_binding) {
  struct Type : Singleton {};
  auto parent = Container{bind<Type>().in<scope::Transient>()};
  auto child = Container{parent, bind<Type>().in<scope::Transient>()};

  // Each promotes separately because each has its own binding.
  auto& parent_ref = parent.template resolve<Type&>();
  auto& child_ref = child.template resolve<Type&>();

  EXPECT_NE(&parent_ref, &child_ref);  // Different instances
  EXPECT_EQ(0, parent_ref.id);
  EXPECT_EQ(1, child_ref.id);
  EXPECT_EQ(2, Counted::num_instances);
}

TEST_F(IntegrationTestHierarchyPromotion,
       grandparent_parent_child_share_promoted_instance_when_delegating) {
  struct Type : Singleton {};
  auto grandparent = Container{bind<Type>().in<scope::Transient>()};
  auto parent = Container{grandparent};  // No binding, delegates to grandparent
  auto child =
      Container{parent};  // No binding, delegates to parent -> grandparent

  auto& grandparent_ref = grandparent.template resolve<Type&>();
  auto& parent_ref = parent.template resolve<Type&>();
  auto& child_ref = child.template resolve<Type&>();

  // All share grandparent's promoted instance
  EXPECT_EQ(&grandparent_ref, &parent_ref);
  EXPECT_EQ(&parent_ref, &child_ref);
  EXPECT_EQ(0, grandparent_ref.id);
  EXPECT_EQ(1, Counted::num_instances);
}

// Ancestry is part of a container's type, so ancestors can all have the same
// bindings but remain unique types and have separate cached instances.
TEST_F(IntegrationTestHierarchyPromotion,
       ancestry_with_same_bindings_promote_separate_instances) {
  struct Type : Singleton {};
  auto grandparent = Container{bind<Type>().in<scope::Transient>()};
  auto parent = Container{grandparent, bind<Type>().in<scope::Transient>()};
  auto child = Container{parent, bind<Type>().in<scope::Transient>()};

  auto& grandparent_ref = grandparent.template resolve<Type&>();
  auto& parent_ref = parent.template resolve<Type&>();
  auto& child_ref = child.template resolve<Type&>();

  // Each has its own promoted instance.
  EXPECT_NE(&grandparent_ref, &parent_ref);
  EXPECT_NE(&parent_ref, &child_ref);
  EXPECT_EQ(0, grandparent_ref.id);
  EXPECT_EQ(1, parent_ref.id);
  EXPECT_EQ(2, child_ref.id);
  EXPECT_EQ(3, Counted::num_instances);
}

// ----------------------------------------------------------------------------
// Hierarchical Container Tests - Value Resolution in Hierarchy
// ----------------------------------------------------------------------------

struct IntegrationTestHierarchyValueResolution : IntegrationTest {};

TEST_F(IntegrationTestHierarchyValueResolution,
       child_gets_copies_of_parent_singleton) {
  struct Type : Singleton {};
  auto parent = Container{bind<Type>().in<scope::Singleton>()};
  auto child = Container{parent};

  // Child requests by value, gets copies of parent's singleton.
  auto child_val1 = child.template resolve<Type>();
  auto child_val2 = child.template resolve<Type>();

  EXPECT_NE(&child_val1, &child_val2);   // Different copies
  EXPECT_EQ(0, child_val1.id);           // Both copies of same singleton (id 0)
  EXPECT_EQ(0, child_val2.id);           // Both copies of same singleton (id 0)
  EXPECT_EQ(1, Counted::num_instances);  // Only parent's singleton
}

TEST_F(IntegrationTestHierarchyValueResolution,
       parent_singleton_reference_differs_from_child_value_copies) {
  struct Type : Singleton {};
  auto parent = Container{bind<Type>().in<scope::Singleton>()};
  auto child = Container{parent};

  auto& parent_ref = parent.template resolve<Type&>();
  auto child_val = child.template resolve<Type>();

  EXPECT_NE(&parent_ref, &child_val);    // Value is a copy
  EXPECT_EQ(0, parent_ref.id);           // Singleton
  EXPECT_EQ(0, child_val.id);            // Copy of same singleton
  EXPECT_EQ(1, Counted::num_instances);  // Only 1 singleton
}

TEST_F(
    IntegrationTestHierarchyValueResolution,
    grandparent_singleton_reference_accessible_but_child_can_get_value_copies) {
  struct Type : Singleton {};
  auto grandparent = Container{bind<Type>().in<scope::Singleton>()};
  auto parent = Container{grandparent};
  auto child = Container{parent};

  auto& grandparent_ref = grandparent.template resolve<Type&>();
  auto& child_ref = child.template resolve<Type&>();
  auto child_val = child.template resolve<Type>();

  EXPECT_EQ(&grandparent_ref, &child_ref);  // References shared
  EXPECT_NE(&grandparent_ref, &child_val);  // Value is a copy
  EXPECT_EQ(0, grandparent_ref.id);         // Singleton
  EXPECT_EQ(0, child_val.id);               // Copy of same singleton
  EXPECT_EQ(1, Counted::num_instances);     // Only 1 singleton
}

// =============================================================================
// CACHE STRATEGIES - Per-Type vs Per-Instance
// Choosing between Meyers singleton and instance caches
// =============================================================================

// ----------------------------------------------------------------------------
// Per-Type Cache Tests
// ----------------------------------------------------------------------------

struct IntegrationTestPerTypeCache : IntegrationTest {};

TEST_F(IntegrationTestPerTypeCache,
       containers_with_same_type_share_bound_singletons) {
  struct Type : Singleton {};

  auto parent = Container{};

  // These containers have the same type.
  auto child1 = Container{parent, bind<Type>().in<scope::Singleton>()};
  auto child2 = Container{parent, bind<Type>().in<scope::Singleton>()};
  static_assert(std::same_as<decltype(child1), decltype(child2)>);

  // Children with the same type share per-type cache.
  auto& child1_ref = child1.template resolve<Type&>();
  auto& child2_ref = child2.template resolve<Type&>();

  EXPECT_EQ(&child1_ref, &child2_ref);
  EXPECT_EQ(0, child1_ref.id);
  EXPECT_EQ(0, child2_ref.id);
  EXPECT_EQ(1, Counted::num_instances);
}

TEST_F(IntegrationTestPerTypeCache,
       containers_with_same_type_share_promoted_singletons) {
  struct Type : Singleton {};

  auto parent = Container{};

  // These containers have the same type.
  auto child1 = Container{parent, bind<Type>().in<scope::Transient>()};
  auto child2 = Container{parent, bind<Type>().in<scope::Transient>()};
  static_assert(std::same_as<decltype(child1), decltype(child2)>);

  // Children with the same type share per-type cache, even when promoted.
  auto& child1_ref = child1.template resolve<Type&>();
  auto& child2_ref = child2.template resolve<Type&>();

  EXPECT_EQ(&child1_ref, &child2_ref);
  EXPECT_EQ(0, child1_ref.id);
  EXPECT_EQ(0, child2_ref.id);
  EXPECT_EQ(1, Counted::num_instances);
}

TEST_F(IntegrationTestPerTypeCache,
       containers_with_different_types_do_not_share_singletons) {
  struct Type : Singleton {};

  auto parent = Container{};

  // These containers have different bindings, so they have different types.
  auto child1 = Container{parent, bind<Type>().in<scope::Singleton>()};
  auto child2 = Container{parent, bind<Type>().in<scope::Transient>()};
  static_assert(!std::same_as<decltype(child1), decltype(child2)>);

  // Children with different types do not share per-type cache.
  auto& child1_ref = child1.template resolve<Type&>();
  auto& child2_ref = child2.template resolve<Type&>();

  EXPECT_NE(&child1_ref, &child2_ref);
  EXPECT_EQ(0, child1_ref.id);
  EXPECT_EQ(1, child2_ref.id);
  EXPECT_EQ(2, Counted::num_instances);
}

TEST_F(IntegrationTestPerTypeCache,
       dink_unique_container_creates_distinct_types) {
  struct Type : Singleton {};

  auto parent = Container{bind<Type>().in<scope::Transient>()};

  // These containers have unique types.
  auto child1 =
      dink_unique_container(parent, bind<Type>().in<scope::Singleton>());
  auto child2 =
      dink_unique_container(parent, bind<Type>().in<scope::Singleton>());
  static_assert(!std::same_as<decltype(child1), decltype(child2)>);

  // Children with unique types do not share per-type cache.
  auto& child1_ref = child1.template resolve<Type&>();
  auto& child2_ref = child2.template resolve<Type&>();

  EXPECT_NE(&child1_ref, &child2_ref);
  EXPECT_EQ(0, child1_ref.id);
  EXPECT_EQ(1, child2_ref.id);
  EXPECT_EQ(2, Counted::num_instances);
}

TEST_F(IntegrationTestPerTypeCache,
       repeated_macro_invocations_create_unique_types) {
  auto c1 = dink_unique_container();
  auto c2 = dink_unique_container(c1);
  auto c3 = dink_unique_container(c1);

  static_assert(!std::same_as<decltype(c1), decltype(c2)>);
  static_assert(!std::same_as<decltype(c2), decltype(c3)>);
  static_assert(!std::same_as<decltype(c1), decltype(c3)>);
}

// ----------------------------------------------------------------------------
// Per-Instance Cache Tests
// ----------------------------------------------------------------------------

struct IntegrationTestPerInstanceCache : IntegrationTest {};

TEST_F(IntegrationTestPerInstanceCache,
       containers_with_same_type_do_not_share_share_bound_singletons) {
  struct Type : Singleton {};

  auto parent = Container{};

  // These containers have the same type.
  auto child1 =
      Container{parent, cache::Instance{}, bind<Type>().in<scope::Singleton>()};
  auto child2 =
      Container{parent, cache::Instance{}, bind<Type>().in<scope::Singleton>()};
  static_assert(std::same_as<decltype(child1), decltype(child2)>);

  // Children with the same type do not share per-instance cache.
  auto& child1_ref = child1.template resolve<Type&>();
  auto& child2_ref = child2.template resolve<Type&>();

  EXPECT_NE(&child1_ref, &child2_ref);
  EXPECT_EQ(0, child1_ref.id);
  EXPECT_EQ(1, child2_ref.id);
  EXPECT_EQ(2, Counted::num_instances);
}

TEST_F(IntegrationTestPerInstanceCache,
       containers_with_same_type_do_not_share_promoted_singletons) {
  struct Type : Singleton {};

  auto parent = Container{};

  // These containers have the same type.
  auto child1 =
      Container{parent, cache::Instance{}, bind<Type>().in<scope::Transient>()};
  auto child2 =
      Container{parent, cache::Instance{}, bind<Type>().in<scope::Transient>()};
  static_assert(std::same_as<decltype(child1), decltype(child2)>);

  // Children with the same type do not share per-instance cache.
  auto& child1_ref = child1.template resolve<Type&>();
  auto& child2_ref = child2.template resolve<Type&>();

  EXPECT_NE(&child1_ref, &child2_ref);
  EXPECT_EQ(0, child1_ref.id);
  EXPECT_EQ(1, child2_ref.id);
  EXPECT_EQ(2, Counted::num_instances);
}

// =============================================================================
// COMPLEX SCENARIOS
// Multiple features working together
// =============================================================================

// ----------------------------------------------------------------------------
// Complex Hierarchical Scenarios
// ----------------------------------------------------------------------------

struct IntegrationTestComplexScenarios : IntegrationTest {};

TEST_F(IntegrationTestComplexScenarios, mixed_scopes_across_hierarchy) {
  struct SingletonInGrandparent : Singleton {};
  struct TransientInParent : Initialized {};
  struct SingletonInChild : Singleton {};

  auto grandparent =
      Container{bind<SingletonInGrandparent>().in<scope::Singleton>()};
  auto parent =
      Container{grandparent, bind<TransientInParent>().in<scope::Transient>()};
  auto child =
      Container{parent, bind<SingletonInChild>().in<scope::Singleton>()};

  // Singleton from grandparent shared.
  auto& sg1 = child.template resolve<SingletonInGrandparent&>();
  auto& sg2 = child.template resolve<SingletonInGrandparent&>();
  EXPECT_EQ(&sg1, &sg2);
  EXPECT_EQ(0, sg1.id);

  // Transient from parent creates new instances.
  auto tp1 = child.template resolve<TransientInParent>();
  auto tp2 = child.template resolve<TransientInParent>();
  EXPECT_NE(tp1.id, tp2.id);
  EXPECT_EQ(1, tp1.id);
  EXPECT_EQ(2, tp2.id);

  // Singleton in child
  auto& sc1 = child.template resolve<SingletonInChild&>();
  auto& sc2 = child.template resolve<SingletonInChild&>();
  EXPECT_EQ(&sc1, &sc2);
  EXPECT_EQ(3, sc1.id);

  EXPECT_EQ(4, Counted::num_instances);
}

TEST_F(IntegrationTestComplexScenarios, dependency_chain_across_hierarchy) {
  struct GrandparentDep : Singleton {};
  struct ParentDep : Initialized {
    GrandparentDep* grandparent_dep;
    explicit ParentDep(GrandparentDep& d) : grandparent_dep{&d} {}
  };
  struct ChildService : Singleton {
    ParentDep* parent_dep;
    explicit ChildService(ParentDep& d) : parent_dep{&d} {}
  };

  auto grandparent = Container{bind<GrandparentDep>().in<scope::Singleton>()};
  auto parent = Container{grandparent};  // Unbound, will be promoted.
  auto child = Container{parent};        // Unbound, will be promoted.

  auto& service = child.template resolve<ChildService&>();

  EXPECT_EQ(0, service.parent_dep->grandparent_dep->id);  // singleton
  EXPECT_EQ(1, service.parent_dep->id);                   // promoted in parent
  EXPECT_EQ(2, service.id);                               // promoted in child
  EXPECT_EQ(3, Counted::num_instances);
}

TEST_F(IntegrationTestComplexScenarios,
       promotion_and_value_resolution_across_hierarchy) {
  struct Type : Singleton {};

  auto parent = Container{bind<Type>().in<scope::Transient>()};
  auto child = Container{parent, bind<Type>().in<scope::Singleton>()};

  // Parent transient promoted to singleton.
  auto& parent_ref1 = parent.template resolve<Type&>();
  auto& parent_ref2 = parent.template resolve<Type&>();
  EXPECT_EQ(&parent_ref1, &parent_ref2);
  EXPECT_EQ(0, parent_ref1.id);

  // Child singleton.
  auto& child_ref = child.template resolve<Type&>();
  EXPECT_EQ(1, child_ref.id);

  // Child singleton values are copies.
  auto child_val1 = child.template resolve<Type>();
  auto child_val2 = child.template resolve<Type>();
  EXPECT_NE(&child_val1, &child_val2);  // Different copies
  EXPECT_EQ(1, child_val1.id);          // copy of child singleton
  EXPECT_EQ(1, child_val2.id);          // copy of child singleton

  EXPECT_EQ(2, Counted::num_instances);  // 1 parent (promoted) + 1 child
}

TEST_F(IntegrationTestComplexScenarios,
       promoted_unbound_instances_are_root_singletons) {
  struct Type : Singleton {};

  auto parent = Container{};
  auto child = Container{parent};

  auto& parent_ref = parent.template resolve<Type&>();
  auto& child_ref = child.template resolve<Type&>();

  EXPECT_EQ(&parent_ref, &child_ref);
  EXPECT_EQ(0, parent_ref.id);
  EXPECT_EQ(0, child_ref.id);
  EXPECT_EQ(1, Counted::num_instances);
}

TEST_F(IntegrationTestComplexScenarios,
       delegated_transient_promotions_are_shared) {
  struct Type : Singleton {};

  auto parent = Container{bind<Type>().in<scope::Transient>()};
  auto child1 = Container{parent};
  auto child2 = Container{parent};

  // Both children delegate to parent, share parent's promoted transient.
  auto& child1_ref = child1.template resolve<Type&>();
  auto& child2_ref = child2.template resolve<Type&>();

  EXPECT_EQ(&child1_ref, &child2_ref);  // Same instance.
  EXPECT_EQ(0, child1_ref.id);
  EXPECT_EQ(1, Counted::num_instances);
}

TEST_F(IntegrationTestComplexScenarios,
       delegated_unbound_promotions_are_shared) {
  struct Type : Singleton {};

  auto parent = Container{};
  auto child1 = Container{parent};
  auto child2 = Container{parent};

  // Both children delegate to parent and share parent's promoted, unbound
  // instance.
  auto& child1_ref = child1.template resolve<Type&>();
  auto& child2_ref = child2.template resolve<Type&>();

  EXPECT_EQ(&child1_ref, &child2_ref);  // Same instance.
  EXPECT_EQ(0, child1_ref.id);
  EXPECT_EQ(1, Counted::num_instances);
}

TEST_F(IntegrationTestComplexScenarios,
       deep_hierarchy_with_multiple_overrides) {
  auto level0_factory = []() { return Product{0}; };
  auto level2_factory = []() { return Product{2}; };
  auto level4_factory = []() { return Product{4}; };

  auto level0 = Container{bind<Product>().via(level0_factory)};
  auto level1 = Container{level0};
  auto level2 = Container{level1, bind<Product>().via(level2_factory)};
  auto level3 = Container{level2};
  auto level4 = Container{level3, bind<Product>().via(level4_factory)};

  auto r0 = level0.template resolve<Product>();
  auto r1 = level1.template resolve<Product>();
  auto r2 = level2.template resolve<Product>();
  auto r3 = level3.template resolve<Product>();
  auto r4 = level4.template resolve<Product>();

  EXPECT_EQ(0, r0.value);
  EXPECT_EQ(0, r1.value);  // Inherits from level0
  EXPECT_EQ(2, r2.value);  // Overrides
  EXPECT_EQ(2, r3.value);  // Inherits from level2
  EXPECT_EQ(4, r4.value);  // Overrides
}

// =============================================================================
// MIXED SCOPES
// Multiple scope types coexisting in one container
// =============================================================================

// ----------------------------------------------------------------------------
// Mixed Scopes Tests
// ----------------------------------------------------------------------------

struct IntegrationTestMixedScopes : IntegrationTest {};

TEST_F(IntegrationTestMixedScopes, transient_and_singleton_coexist) {
  using TransientType = Initialized;
  struct SingletonType : Singleton {};

  auto sut = Container{bind<TransientType>().in<scope::Transient>(),
                       bind<SingletonType>().in<scope::Singleton>()};

  auto t1 = sut.template resolve<std::shared_ptr<TransientType>>();
  auto t2 = sut.template resolve<std::shared_ptr<TransientType>>();
  EXPECT_NE(t1.get(), t2.get());

  auto s1 = sut.template resolve<std::shared_ptr<SingletonType>>();
  auto s2 = sut.template resolve<std::shared_ptr<SingletonType>>();
  EXPECT_EQ(s1.get(), s2.get());
}

TEST_F(IntegrationTestMixedScopes, all_scopes_coexist) {
  using TransientType = Initialized;
  struct SingletonType : Singleton {};
  struct InstanceType : Initialized {};
  auto external = InstanceType{};

  auto sut = Container{bind<TransientType>().in<scope::Transient>(),
                       bind<SingletonType>().in<scope::Singleton>(),
                       bind<InstanceType>().to(external)};

  auto t = sut.template resolve<std::shared_ptr<TransientType>>();
  auto s = sut.template resolve<std::shared_ptr<SingletonType>>();
  auto i = sut.template resolve<std::shared_ptr<InstanceType>>();

  EXPECT_NE(t.get(), nullptr);
  EXPECT_NE(s.get(), nullptr);
  EXPECT_EQ(&external, i.get());
}

// =============================================================================
// EDGE CASES & SPECIAL SITUATIONS
// Boundary conditions and unusual scenarios
// =============================================================================

// ----------------------------------------------------------------------------
// Edge Cases
// ----------------------------------------------------------------------------

struct IntegrationTestEdgeCases : IntegrationTest {};

TEST_F(IntegrationTestEdgeCases, zero_argument_constructor) {
  struct ZeroArgs {
    int_t value = kModifiedValue;
    ZeroArgs() = default;
  };

  auto sut = Container{bind<ZeroArgs>()};

  auto value = sut.template resolve<ZeroArgs>();
  EXPECT_EQ(kModifiedValue, value.value);
}

TEST_F(IntegrationTestEdgeCases, multi_argument_constructor) {
  struct MultiArg {
    int_t sum;
    MultiArg(Dep1 d1, Dep2 d2, Dep3 d3) : sum{d1.value + d2.value + d3.value} {}
  };

  auto sut =
      Container{bind<Dep1>(), bind<Dep2>(), bind<Dep3>(), bind<MultiArg>()};

  auto result = sut.template resolve<MultiArg>();
  EXPECT_EQ(6, result.sum);  // 1 + 2 + 3
}

TEST_F(IntegrationTestEdgeCases, deeply_nested_dependencies) {
  struct Level0 {
    int_t value = 3;
    Level0() = default;
  };
  struct Level1 {
    int_t value;
    explicit Level1(Level0 l0) : value{l0.value * 2} {}
  };
  struct Level2 {
    int_t value;
    explicit Level2(Level1 l1) : value{l1.value * 2} {}
  };
  struct Level3 {
    int_t value;
    explicit Level3(Level2 l2) : value{l2.value * 2} {}
  };
  struct Level4 {
    int_t value;
    explicit Level4(Level3 l3) : value{l3.value * 2} {}
  };

  auto sut = Container{bind<Level0>(), bind<Level1>(), bind<Level2>(),
                       bind<Level3>(), bind<Level4>()};

  auto result = sut.template resolve<Level4>();
  EXPECT_EQ(48, result.value);  // 3 * 2 * 2 * 2 * 2
}

TEST_F(IntegrationTestEdgeCases, type_with_deleted_copy_constructor) {
  struct NoCopy {
    int_t value = kInitialValue;
    NoCopy() = default;
    NoCopy(const NoCopy&) = delete;
    NoCopy(NoCopy&&) = default;
  };

  auto sut = Container{bind<NoCopy>().in<scope::Singleton>()};

  // Can't resolve by value, but can resolve by reference.
  auto& ref = sut.template resolve<NoCopy&>();
  EXPECT_EQ(kInitialValue, ref.value);

  // Can resolve as pointer.
  auto* ptr = sut.template resolve<NoCopy*>();
  EXPECT_EQ(kInitialValue, ptr->value);
}

}  // namespace
}  // namespace dink::container
