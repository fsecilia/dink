/*
  Copyright (c) 2025 Frank Secilia \n
  SPDX-License-Identifier: MIT
*/

#include "integration_test.hpp"

namespace dink::container {
namespace {

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

}  // namespace
}  // namespace dink::container
