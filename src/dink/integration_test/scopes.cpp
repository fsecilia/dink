/*
  Copyright (c) 2025 Frank Secilia \n
  SPDX-License-Identifier: MIT
*/

#include "integration_test.hpp"

namespace dink::container {
namespace {

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

}  // namespace
}  // namespace dink::container
