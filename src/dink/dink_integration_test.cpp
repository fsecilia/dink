// \file
// Copyright (c) 2025 Frank Secilia
// SPDX-License-Identifier: MIT

#include "dink/container.hpp"
#include <dink/test.hpp>
#include <dink/binding.hpp>
#include <dink/binding_dsl.hpp>
#include <dink/scope.hpp>

namespace dink {
namespace {

// ----------------------------------------------------------------------------
// Common Test Infrastructure
// ----------------------------------------------------------------------------

// Base class for types that need instance counting
struct Counted {
  static inline int_t num_instances = 0;
  int_t id;
  Counted() : id{num_instances++} {}
};

// Common base for all container tests - resets counters
struct ContainerTest : Test {
  static inline const auto initial_value = int_t{7793};   // arbitrary
  static inline const auto modified_value = int_t{2145};  // arbitrary

  ContainerTest() { Counted::num_instances = 0; }
};

// ----------------------------------------------------------------------------
// Singleton Scope Tests
// ----------------------------------------------------------------------------
// These tests require unique types per test, or cached instances will leak
// between them.

struct ContainerSingletonTest : ContainerTest {};

TEST_F(ContainerSingletonTest, canonical_shared_wraps_instance) {
  struct Type {};
  auto sut = Container{bind<Type>().in<scope::Singleton>()};

  const auto shared = sut.template resolve<std::shared_ptr<Type>>();
  auto& instance = sut.template resolve<Type&>();
  ASSERT_EQ(&instance, shared.get());
}

TEST_F(ContainerSingletonTest, canonical_shared_ptr_value) {
  struct Type {};
  auto sut = Container{bind<Type>().in<scope::Singleton>()};

  const auto result1 = sut.template resolve<std::shared_ptr<Type>>();
  const auto result2 = sut.template resolve<std::shared_ptr<Type>>();
  ASSERT_EQ(result1, result2);
  ASSERT_EQ(result1.use_count(), result2.use_count());
  ASSERT_EQ(result1.use_count(), 3);  // result1 + result2 + canonical

  auto& instance = sut.template resolve<Type&>();
  ASSERT_EQ(&instance, result1.get());
}

TEST_F(ContainerSingletonTest, canonical_shared_ptr_identity) {
  struct Type {};
  auto sut = Container{bind<Type>().in<scope::Singleton>()};

  const auto& result1 = sut.template resolve<std::shared_ptr<Type>&>();
  const auto& result2 = sut.template resolve<std::shared_ptr<Type>&>();
  ASSERT_EQ(&result1, &result2);
  ASSERT_EQ(result1.use_count(), result2.use_count());
  ASSERT_EQ(result1.use_count(), 1);
}

TEST_F(ContainerSingletonTest, weak_ptr_from_singleton) {
  struct Type {};
  auto sut = Container{bind<Type>().in<scope::Singleton>()};

  auto weak1 = sut.template resolve<std::weak_ptr<Type>>();
  auto weak2 = sut.template resolve<std::weak_ptr<Type>>();

  EXPECT_FALSE(weak1.expired());
  EXPECT_EQ(weak1.lock(), weak2.lock());
}

TEST_F(ContainerSingletonTest, weak_ptr_does_not_expire_while_singleton_alive) {
  struct Type {};
  auto sut = Container{bind<Type>().in<scope::Singleton>()};

  const auto& weak = sut.template resolve<std::weak_ptr<Type>>();

  // Even with no shared_ptr in scope, weak_ptr should not expire
  // because it tracks the canonical shared_ptr which aliases the instance.
  EXPECT_FALSE(weak.expired());

  auto shared = weak.lock();
  EXPECT_NE(nullptr, shared);
}

TEST_F(ContainerSingletonTest, weak_ptr_expires_with_canonical_shared_ptr) {
  struct Type {};
  auto sut = Container{bind<Type>().in<scope::Singleton>()};

  // resolve reference directly to canonical shared_ptr
  auto& canonical_shared_ptr = sut.template resolve<std::shared_ptr<Type>&>();
  const auto weak = sut.template resolve<std::weak_ptr<Type>>();

  EXPECT_FALSE(weak.expired());
  canonical_shared_ptr.reset();
  EXPECT_TRUE(weak.expired());
}

TEST_F(ContainerSingletonTest, const_shared_ptr) {
  struct Type {};
  auto sut = Container{bind<Type>().in<scope::Singleton>()};

  auto shared = sut.template resolve<std::shared_ptr<const Type>>();
  auto& instance = sut.template resolve<Type&>();

  EXPECT_EQ(&instance, shared.get());
}

TEST_F(ContainerSingletonTest, multiple_singleton_types) {
  struct A {};
  struct B {};

  auto sut = Container{bind<A>().in<scope::Singleton>(),
                       bind<B>().in<scope::Singleton>()};

  auto shared_a = sut.template resolve<std::shared_ptr<A>>();
  auto shared_b = sut.template resolve<std::shared_ptr<B>>();

  EXPECT_NE(shared_a.get(), nullptr);
  EXPECT_NE(shared_b.get(), nullptr);
}

TEST_F(ContainerSingletonTest, resolves_mutable_reference) {
  struct Type {
    int_t value = initial_value;
    Type() = default;
  };
  auto sut = Container{bind<Type>().in<scope::Singleton>()};

  auto& ref1 = sut.template resolve<Type&>();
  auto& ref2 = sut.template resolve<Type&>();

  EXPECT_EQ(&ref1, &ref2);
  EXPECT_EQ(initial_value, ref1.value);

  ref1.value = modified_value;
  EXPECT_EQ(modified_value, ref2.value);
}

TEST_F(ContainerSingletonTest, resolves_const_reference) {
  struct Type {
    int_t value = initial_value;
    Type() = default;
  };
  auto sut = Container{bind<Type>().in<scope::Singleton>()};

  const auto& ref1 = sut.template resolve<const Type&>();
  const auto& ref2 = sut.template resolve<const Type&>();

  EXPECT_EQ(&ref1, &ref2);
  EXPECT_EQ(initial_value, ref1.value);
}

TEST_F(ContainerSingletonTest, resolves_mutable_pointer) {
  struct Type {
    int_t value = initial_value;
    Type() = default;
  };
  auto sut = Container{bind<Type>().in<scope::Singleton>()};

  auto* ptr1 = sut.template resolve<Type*>();
  auto* ptr2 = sut.template resolve<Type*>();

  EXPECT_EQ(ptr1, ptr2);
  EXPECT_EQ(initial_value, ptr1->value);

  ptr1->value = modified_value;
  EXPECT_EQ(modified_value, ptr2->value);
}

TEST_F(ContainerSingletonTest, resolves_const_pointer) {
  struct Type {
    int_t value = initial_value;
    Type() = default;
  };
  auto sut = Container{bind<Type>().in<scope::Singleton>()};

  const auto* ptr1 = sut.template resolve<const Type*>();
  const auto* ptr2 = sut.template resolve<const Type*>();

  EXPECT_EQ(ptr1, ptr2);
  EXPECT_EQ(initial_value, ptr1->value);
}

TEST_F(ContainerSingletonTest, reference_and_pointer_point_to_same_instance) {
  struct Type {};
  auto sut = Container{bind<Type>().in<scope::Singleton>()};

  auto& ref = sut.template resolve<Type&>();
  auto* ptr = sut.template resolve<Type*>();

  EXPECT_EQ(&ref, ptr);
}

// ----------------------------------------------------------------------------
// Transient Scope Tests
// ----------------------------------------------------------------------------

struct ContainerTransientTest : ContainerTest {
  struct Type : Counted {
    int_t value = initial_value;
    Type() = default;
  };
};  // namespace

TEST_F(ContainerTransientTest, creates_new_shared_ptr_per_resolve) {
  auto sut = Container{bind<Type>().in<scope::Transient>()};

  auto shared1 = sut.template resolve<std::shared_ptr<Type>>();
  auto shared2 = sut.template resolve<std::shared_ptr<Type>>();

  EXPECT_NE(shared1.get(), shared2.get());  // Different instances
}

TEST_F(ContainerTransientTest, creates_new_value_per_resolve) {
  auto sut = Container{bind<Type>().in<scope::Transient>()};

  auto value1 = sut.template resolve<Type>();
  auto value2 = sut.template resolve<Type>();

  EXPECT_EQ(0, value1.id);
  EXPECT_EQ(1, value2.id);
}

TEST_F(ContainerTransientTest, creates_new_unique_ptr_per_resolve) {
  auto sut = Container{bind<Type>().in<scope::Transient>()};

  auto unique1 = sut.template resolve<std::unique_ptr<Type>>();
  auto unique2 = sut.template resolve<std::unique_ptr<Type>>();

  EXPECT_NE(unique1.get(), unique2.get());
  EXPECT_EQ(initial_value, unique1->value);
  EXPECT_EQ(initial_value, unique2->value);
}

TEST_F(ContainerTransientTest, resolves_const_value) {
  auto sut = Container{bind<Type>().in<scope::Transient>()};

  const auto value = sut.template resolve<const Type>();
  EXPECT_EQ(initial_value, value.value);
}

TEST_F(ContainerTransientTest, resolves_rvalue_reference) {
  auto sut = Container{bind<Type>().in<scope::Transient>()};

  auto&& value = sut.template resolve<Type&&>();
  EXPECT_EQ(initial_value, value.value);
}

// ----------------------------------------------------------------------------
// Instance Scope Tests
// ----------------------------------------------------------------------------

struct ContainerInstanceTest : ContainerTest {
  struct Type {
    int_t value = initial_value;
  };
  Type instance;
};

TEST_F(ContainerInstanceTest, shared_ptr_wraps_external_instance) {
  auto sut = Container{bind<Type>().to(instance)};

  // shared_ptr should wrap the external instance
  auto shared1 = sut.template resolve<std::shared_ptr<Type>>();
  auto shared2 = sut.template resolve<std::shared_ptr<Type>>();

  EXPECT_EQ(&instance, shared1.get());      // Points to external
  EXPECT_EQ(shared1.get(), shared2.get());  // Same cached shared_ptr
  EXPECT_EQ(3, shared1.use_count());        // canonical + shared1 + shared2

  // Reference to the external instance
  auto& ref = sut.template resolve<Type&>();
  EXPECT_EQ(&instance, &ref);
  EXPECT_EQ(&ref, shared1.get());
}

TEST_F(ContainerInstanceTest, canonical_shared_ptr_reference) {
  auto sut = Container{bind<Type>().to(instance)};

  auto& canonical1 = sut.template resolve<std::shared_ptr<Type>&>();
  auto& canonical2 = sut.template resolve<std::shared_ptr<Type>&>();

  EXPECT_EQ(&canonical1, &canonical2);         // Same cached shared_ptr
  EXPECT_EQ(&instance, canonical1.get());      // Wraps external
  EXPECT_EQ(1, canonical1.use_count());        // Only canonical exists
}

TEST_F(ContainerInstanceTest, weak_ptr_tracks_external_instance) {
  auto sut = Container{bind<Type>().to(instance)};

  auto weak = sut.template resolve<std::weak_ptr<Type>>();

  EXPECT_FALSE(weak.expired());
  EXPECT_EQ(&instance, weak.lock().get());
}

TEST_F(ContainerInstanceTest, weak_ptr_does_not_expire_while_instance_alive) {
  auto container = Container{bind<Type>().to(instance)};

  auto weak = container.template resolve<std::weak_ptr<Type>>();

  // Even with no shared_ptr in scope, weak_ptr should not expire
  // because it tracks the canonical shared_ptr which aliases the static
  EXPECT_FALSE(weak.expired());

  auto shared = weak.lock();
  EXPECT_NE(nullptr, shared);
}

TEST_F(ContainerInstanceTest, weak_ptr_expires_with_canonical_shared_ptr) {
  auto container = Container{bind<Type>().to(instance)};

  // resolve reference directly to canonical shared_ptr
  auto& canonical_shared_ptr =
      container.template resolve<std::shared_ptr<Type>&>();
  const auto weak = container.template resolve<std::weak_ptr<Type>>();

  EXPECT_FALSE(weak.expired());
  canonical_shared_ptr.reset();
  EXPECT_TRUE(weak.expired());
}

TEST_F(ContainerInstanceTest, resolves_value_copy_of_external) {
  instance.value = modified_value;

  auto sut = Container{bind<Type>().to(instance)};

  auto copy = sut.template resolve<Type>();
  EXPECT_EQ(modified_value, copy.value);

  // Verify it's a copy, not the original
  copy.value *= 2;
  EXPECT_EQ(modified_value, instance.value);
}

TEST_F(ContainerInstanceTest, resolves_mutable_reference) {
  auto sut = Container{bind<Type>().to(instance)};

  auto& ref = sut.template resolve<Type&>();
  EXPECT_EQ(&instance, &ref);

  ref.value = modified_value;
  EXPECT_EQ(modified_value, instance.value);
}

TEST_F(ContainerInstanceTest, resolves_const_reference) {
  auto sut = Container{bind<Type>().to(instance)};

  const auto& ref = sut.template resolve<const Type&>();
  EXPECT_EQ(&instance, &ref);
}

TEST_F(ContainerInstanceTest, resolves_mutable_pointer) {
  auto sut = Container{bind<Type>().to(instance)};

  auto* ptr = sut.template resolve<Type*>();
  EXPECT_EQ(&instance, ptr);

  ptr->value = modified_value;
  EXPECT_EQ(modified_value, instance.value);
}

TEST_F(ContainerInstanceTest, resolves_const_pointer) {
  auto sut = Container{bind<Type>().to(instance)};

  const auto* ptr = sut.template resolve<const Type*>();
  EXPECT_EQ(&instance, ptr);
}

// ----------------------------------------------------------------------------
// Factory Binding Tests
// ----------------------------------------------------------------------------

struct ContainerFactoryTest : ContainerTest {
  struct Product : Counted {
    int_t value = initial_value;
  };
};

TEST_F(ContainerFactoryTest, resolves_with_factory) {
  auto factory = []() { return Product{.value = modified_value}; };

  auto sut = Container{bind<Product>().as<Product>().via(factory)};

  auto value = sut.template resolve<Product>();
  EXPECT_EQ(modified_value, value.value);
}

TEST_F(ContainerFactoryTest, factory_with_singleton_scope) {
  auto factory = []() { return Product{}; };

  auto sut = Container{
      bind<Product>().as<Product>().via(factory).in<scope::Singleton>()};

  auto& ref1 = sut.template resolve<Product&>();
  auto& ref2 = sut.template resolve<Product&>();

  EXPECT_EQ(&ref1, &ref2);
  EXPECT_EQ(0, ref1.id);
  EXPECT_EQ(1, Counted::num_instances);  // Factory called once
}

TEST_F(ContainerFactoryTest, factory_with_transient_scope) {
  auto factory = []() { return Product{}; };

  auto sut = Container{
      bind<Product>().as<Product>().via(factory).in<scope::Transient>()};

  auto value1 = sut.template resolve<Product>();
  auto value2 = sut.template resolve<Product>();

  EXPECT_EQ(0, value1.id);
  EXPECT_EQ(1, value2.id);
  EXPECT_EQ(2, Counted::num_instances);  // Factory called twice
}

TEST_F(ContainerFactoryTest, factory_with_deduced_scope) {
  auto factory = []() { return Product{}; };

  auto sut = Container{bind<Product>().as<Product>().via(factory)};

  auto value = sut.template resolve<Product>();
  auto& ref = sut.template resolve<Product&>();

  EXPECT_EQ(initial_value, value.value);
  EXPECT_EQ(initial_value, ref.value);
}

TEST_F(ContainerFactoryTest, factory_with_parameters_from_container) {
  struct Dependency {
    int_t value = 10;
    Dependency() = default;
  };

  struct ProductWithDep {
    int_t combined_value;
    explicit ProductWithDep(Dependency dep) : combined_value{dep.value * 2} {}
  };

  auto factory = [](Dependency dep) { return ProductWithDep{dep}; };

  auto sut =
      Container{bind<Dependency>(),
                bind<ProductWithDep>().as<ProductWithDep>().via(factory)};

  auto product = sut.template resolve<ProductWithDep>();
  EXPECT_EQ(20, product.combined_value);
}

// ----------------------------------------------------------------------------
// Interface/Implementation Binding Tests
// ----------------------------------------------------------------------------

struct ContainerInterfaceTest : ContainerTest {
  struct IService {
    virtual ~IService() = default;
    virtual int_t get_value() const = 0;
  };
};

TEST_F(ContainerInterfaceTest, binds_interface_to_implementation) {
  struct ServiceImpl : IService {
    int_t get_value() const override { return initial_value; }
  };

  auto sut = Container{bind<IService>().as<ServiceImpl>()};

  auto& service = sut.template resolve<IService&>();
  EXPECT_EQ(initial_value, service.get_value());
}

TEST_F(ContainerInterfaceTest, interface_binding_with_singleton_scope) {
  struct ServiceImpl : IService, Counted {
    int_t get_value() const override { return id; }
  };

  auto sut =
      Container{bind<IService>().as<ServiceImpl>().in<scope::Singleton>()};

  auto& ref1 = sut.template resolve<IService&>();
  auto& ref2 = sut.template resolve<IService&>();

  EXPECT_EQ(&ref1, &ref2);
  EXPECT_EQ(0, ref1.get_value());
}

TEST_F(ContainerInterfaceTest, interface_binding_with_factory) {
  struct ServiceImpl : IService {
    int_t value;
    explicit ServiceImpl(int_t v) : value{v} {}
    int_t get_value() const override { return value; }
  };

  auto factory = []() { return ServiceImpl{modified_value}; };

  auto sut = Container{bind<IService>().as<ServiceImpl>().via(factory)};

  auto& service = sut.template resolve<IService&>();
  EXPECT_EQ(modified_value, service.get_value());
}

TEST_F(ContainerInterfaceTest, resolves_implementation_directly) {
  struct ServiceImpl : IService {
    int_t get_value() const override { return initial_value; }
  };

  auto sut = Container{bind<IService>().as<ServiceImpl>()};

  // Can still resolve ServiceImpl directly (not bound)
  auto& impl = sut.template resolve<ServiceImpl&>();
  EXPECT_EQ(initial_value, impl.get_value());
}

TEST_F(ContainerInterfaceTest, multiple_interfaces_to_implementations) {
  struct IFoo {
    virtual ~IFoo() = default;
    virtual int_t foo() const = 0;
  };
  struct IBar {
    virtual ~IBar() = default;
    virtual int_t bar() const = 0;
  };

  struct FooImpl : IFoo {
    int_t foo() const override { return 1; }
  };
  struct BarImpl : IBar {
    int_t bar() const override { return 2; }
  };

  auto sut = Container{bind<IFoo>().as<FooImpl>(), bind<IBar>().as<BarImpl>()};

  auto& foo = sut.template resolve<IFoo&>();
  auto& bar = sut.template resolve<IBar&>();

  EXPECT_EQ(1, foo.foo());
  EXPECT_EQ(2, bar.bar());
}

// ----------------------------------------------------------------------------
// Dependency Injection Tests
// ----------------------------------------------------------------------------

struct ContainerDependencyInjectionTest : ContainerTest {};

TEST_F(ContainerDependencyInjectionTest, resolves_single_dependency) {
  struct Dependency {
    int_t value = 10;
    Dependency() = default;
  };

  struct Service {
    int_t result;
    explicit Service(Dependency dep) : result{dep.value * 2} {}
  };

  auto sut = Container{bind<Dependency>(), bind<Service>()};

  auto service = sut.template resolve<Service>();
  EXPECT_EQ(20, service.result);
}

TEST_F(ContainerDependencyInjectionTest, resolves_multiple_dependencies) {
  struct DepA {
    int_t value = 10;
    DepA() = default;
  };
  struct DepB {
    int_t value = 5;
    DepB() = default;
  };

  struct Service {
    int_t sum;
    Service(DepA a, DepB b) : sum{a.value + b.value} {}
  };

  auto sut = Container{bind<DepA>(), bind<DepB>(), bind<Service>()};

  auto service = sut.template resolve<Service>();
  EXPECT_EQ(15, service.sum);
}

TEST_F(ContainerDependencyInjectionTest, resolves_dependency_chain) {
  struct DepA {
    int_t value = 3;
    DepA() = default;
  };

  struct DepB {
    int_t value;
    explicit DepB(DepA a) : value{a.value * 5} {}
  };

  struct Service {
    int_t value;
    explicit Service(DepB b) : value{b.value * 7} {}
  };

  auto sut = Container{bind<DepA>(), bind<DepB>(), bind<Service>()};

  auto service = sut.template resolve<Service>();
  EXPECT_EQ(105, service.value);  // 3 * 5 * 7
}

TEST_F(ContainerDependencyInjectionTest, resolves_dependency_as_reference) {
  struct Dependency {
    int_t value = initial_value;
    Dependency() = default;
  };

  struct Service {
    Dependency* dep_ptr;
    explicit Service(Dependency& dep) : dep_ptr{&dep} {}
  };

  auto sut =
      Container{bind<Dependency>().in<scope::Singleton>(), bind<Service>()};

  auto service = sut.template resolve<Service>();
  auto& dep = sut.template resolve<Dependency&>();

  EXPECT_EQ(&dep, service.dep_ptr);
  EXPECT_EQ(initial_value, service.dep_ptr->value);
}

TEST_F(ContainerDependencyInjectionTest,
       resolves_dependency_as_const_reference) {
  struct Dependency {
    int_t value = initial_value;
    Dependency() = default;
  };

  struct Service {
    int_t copied_value;
    explicit Service(const Dependency& dep) : copied_value{dep.value} {}
  };

  auto sut = Container{bind<Dependency>(), bind<Service>()};

  auto service = sut.template resolve<Service>();
  EXPECT_EQ(initial_value, service.copied_value);
}

TEST_F(ContainerDependencyInjectionTest, resolves_dependency_as_shared_ptr) {
  struct Dependency {
    int_t value = initial_value;
    Dependency() = default;
  };

  struct Service {
    std::shared_ptr<Dependency> dep;
    explicit Service(std::shared_ptr<Dependency> d) : dep{std::move(d)} {}
  };

  auto sut =
      Container{bind<Dependency>().in<scope::Singleton>(), bind<Service>()};

  auto service = sut.template resolve<Service>();
  EXPECT_EQ(initial_value, service.dep->value);
  EXPECT_EQ(2, service.dep.use_count());  // canonical + service.dep
}

TEST_F(ContainerDependencyInjectionTest, resolves_dependency_as_unique_ptr) {
  struct Dependency {
    int_t value = initial_value;
    Dependency() = default;
  };

  struct Service {
    std::unique_ptr<Dependency> dep;
    explicit Service(std::unique_ptr<Dependency> d) : dep{std::move(d)} {}
  };

  auto sut =
      Container{bind<Dependency>().in<scope::Transient>(), bind<Service>()};

  auto service = sut.template resolve<Service>();
  EXPECT_EQ(initial_value, service.dep->value);
}

TEST_F(ContainerDependencyInjectionTest, resolves_dependency_as_pointer) {
  struct Dependency {
    int_t value = initial_value;
    Dependency() = default;
  };

  struct Service {
    Dependency* dep;
    explicit Service(Dependency* d) : dep{d} {}
  };

  auto sut =
      Container{bind<Dependency>().in<scope::Singleton>(), bind<Service>()};

  auto service = sut.template resolve<Service>();
  auto* dep = sut.template resolve<Dependency*>();

  EXPECT_EQ(dep, service.dep);
  EXPECT_EQ(initial_value, service.dep->value);
}

TEST_F(ContainerDependencyInjectionTest, mixed_dependency_types) {
  struct DepA {
    int_t value = 1;
    DepA() = default;
  };
  struct DepB {
    int_t value = 2;
    DepB() = default;
  };
  struct DepC {
    int_t value = 3;
    DepC() = default;
  };

  struct Service {
    int_t sum;
    Service(DepA a, const DepB& b, DepC* c)
        : sum{a.value + b.value + c->value} {}
  };

  auto sut = Container{bind<DepA>(), bind<DepB>(),
                       bind<DepC>().in<scope::Singleton>(), bind<Service>()};

  auto service = sut.template resolve<Service>();
  EXPECT_EQ(6, service.sum);
}

TEST_F(ContainerDependencyInjectionTest,
       singleton_dependency_shared_across_services) {
  struct SharedDep : Counted {};

  struct ServiceA {
    SharedDep* dep;
    explicit ServiceA(SharedDep& d) : dep{&d} {}
  };

  struct ServiceB {
    SharedDep* dep;
    explicit ServiceB(SharedDep& d) : dep{&d} {}
  };

  auto sut = Container{bind<SharedDep>().in<scope::Singleton>(),
                       bind<ServiceA>(), bind<ServiceB>()};

  auto serviceA = sut.template resolve<ServiceA>();
  auto serviceB = sut.template resolve<ServiceB>();

  EXPECT_EQ(serviceA.dep, serviceB.dep);  // Same instance
  EXPECT_EQ(0, serviceA.dep->id);
  EXPECT_EQ(1, Counted::num_instances);  // Only one created
}

TEST_F(ContainerDependencyInjectionTest,
       mixed_value_categories_in_constructor) {
  struct Singleton {
    int_t value = 1;
    Singleton() = default;
  };
  struct Transient {
    int_t value = 2;
    Transient() = default;
  };

  struct Service {
    int_t sum;
    Service(Singleton& s, Transient t) : sum{s.value + t.value} {}
  };

  auto sut =
      Container{bind<Singleton>().in<scope::Singleton>(),
                bind<Transient>().in<scope::Transient>(), bind<Service>()};

  auto service = sut.template resolve<Service>();
  EXPECT_EQ(3, service.sum);
}

// ----------------------------------------------------------------------------
// Canonical Type Resolution Tests
// ----------------------------------------------------------------------------

struct ContainerCanonicalTest : ContainerTest {};

TEST_F(ContainerCanonicalTest, const_and_non_const_resolve_same_binding) {
  struct Bound {
    int_t value = initial_value;
    Bound() = default;
  };

  auto sut = Container{bind<Bound>().in<scope::Singleton>()};

  auto& ref = sut.template resolve<Bound&>();
  const auto& const_ref = sut.template resolve<const Bound&>();

  EXPECT_EQ(&ref, &const_ref);
}

TEST_F(ContainerCanonicalTest, reference_and_value_resolve_same_binding) {
  struct Bound : Counted {};

  auto sut = Container{bind<Bound>().in<scope::Singleton>()};

  auto& ref = sut.template resolve<Bound&>();
  // Value copies from the singleton, so ids should match initially
  EXPECT_EQ(0, ref.id);
}

TEST_F(ContainerCanonicalTest, pointer_and_reference_resolve_same_binding) {
  struct Bound {
    int_t value = initial_value;
    Bound() = default;
  };

  auto sut = Container{bind<Bound>().in<scope::Singleton>()};

  auto& ref = sut.template resolve<Bound&>();
  auto* ptr = sut.template resolve<Bound*>();

  EXPECT_EQ(&ref, ptr);
}

TEST_F(ContainerCanonicalTest, const_pointer_and_pointer_resolve_same_binding) {
  struct Bound {
    int_t value = initial_value;
    Bound() = default;
  };

  auto sut = Container{bind<Bound>().in<scope::Singleton>()};

  auto* ptr = sut.template resolve<Bound*>();
  const auto* const_ptr = sut.template resolve<const Bound*>();

  EXPECT_EQ(ptr, const_ptr);
}

TEST_F(ContainerCanonicalTest, shared_ptr_variations_resolve_same_binding) {
  struct Bound {
    int_t value = initial_value;
    Bound() = default;
  };

  auto sut = Container{bind<Bound>().in<scope::Singleton>()};

  auto shared = sut.template resolve<std::shared_ptr<Bound>>();
  auto const_shared = sut.template resolve<std::shared_ptr<const Bound>>();

  // Both should point to the same underlying object
  EXPECT_EQ(shared.get(), const_shared.get());
}

// ----------------------------------------------------------------------------
// Edge Cases and Error Conditions
// ----------------------------------------------------------------------------

struct ContainerEdgeCasesTest : ContainerTest {};

TEST_F(ContainerEdgeCasesTest, empty_container_resolves_unbound_types) {
  struct Unbound {
    int_t value = initial_value;
    Unbound() = default;
  };

  auto sut = Container{};

  auto value = sut.template resolve<Unbound>();
  EXPECT_EQ(initial_value, value.value);
}

TEST_F(ContainerEdgeCasesTest, zero_argument_constructor) {
  struct ZeroArgs {
    int_t value = modified_value;
    ZeroArgs() = default;
  };

  auto sut = Container{bind<ZeroArgs>()};

  auto value = sut.template resolve<ZeroArgs>();
  EXPECT_EQ(modified_value, value.value);
}

TEST_F(ContainerEdgeCasesTest, multi_argument_constructor) {
  struct A {
    int_t value = 1;
    A() = default;
  };
  struct B {
    int_t value = 2;
    B() = default;
  };
  struct C {
    int_t value = 3;
    C() = default;
  };

  struct MultiArg {
    int_t sum;
    MultiArg(A a, B b, C c) : sum{a.value + b.value + c.value} {}
  };

  auto sut = Container{bind<A>(), bind<B>(), bind<C>(), bind<MultiArg>()};

  auto result = sut.template resolve<MultiArg>();
  EXPECT_EQ(6, result.sum);
}

TEST_F(ContainerEdgeCasesTest, resolve_same_type_multiple_ways) {
  struct Type {
    int_t value = initial_value;
    Type() = default;
  };

  auto sut = Container{bind<Type>().in<scope::Singleton>()};

  auto value = sut.template resolve<Type>();
  auto& ref = sut.template resolve<Type&>();
  auto* ptr = sut.template resolve<Type*>();
  auto shared = sut.template resolve<std::shared_ptr<Type>>();

  EXPECT_EQ(&ref, ptr);
  EXPECT_EQ(ptr, shared.get());
  EXPECT_EQ(value.value, ref.value);
  EXPECT_EQ(initial_value, value.value);
}

TEST_F(ContainerEdgeCasesTest, deeply_nested_dependencies) {
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

TEST_F(ContainerEdgeCasesTest, type_with_deleted_copy_constructor) {
  struct NoCopy {
    int_t value = initial_value;
    NoCopy() = default;
    NoCopy(const NoCopy&) = delete;
    NoCopy(NoCopy&&) = default;
  };

  auto sut = Container{bind<NoCopy>().in<scope::Singleton>()};

  // Can't resolve by value, but can resolve by reference
  auto& ref = sut.template resolve<NoCopy&>();
  EXPECT_EQ(initial_value, ref.value);

  // Can resolve as pointer
  auto* ptr = sut.template resolve<NoCopy*>();
  EXPECT_EQ(initial_value, ptr->value);
}

TEST_F(ContainerEdgeCasesTest, resolve_from_multiple_containers) {
  struct Type {
    int_t value;
    explicit Type(int_t v = 0) : value{v} {}
  };

  Type external1{1};
  Type external2{2};

  auto container1 = Container{bind<Type>().to(external1)};
  auto container2 = Container{bind<Type>().to(external2)};

  auto& ref1 = container1.template resolve<Type&>();
  auto& ref2 = container2.template resolve<Type&>();

  EXPECT_EQ(1, ref1.value);
  EXPECT_EQ(2, ref2.value);
  EXPECT_NE(&ref1, &ref2);
}

// ----------------------------------------------------------------------------
// Mixed Scopes Tests
// ----------------------------------------------------------------------------

struct ContainerMixedScopesTest : ContainerTest {};

TEST_F(ContainerMixedScopesTest, transient_and_singleton_coexist) {
  struct Transient {};
  struct Singleton {};

  auto sut = Container{bind<Transient>().in<scope::Transient>(),
                       bind<Singleton>().in<scope::Singleton>()};

  auto t1 = sut.template resolve<std::shared_ptr<Transient>>();
  auto t2 = sut.template resolve<std::shared_ptr<Transient>>();
  EXPECT_NE(t1.get(), t2.get());

  auto s1 = sut.template resolve<std::shared_ptr<Singleton>>();
  auto s2 = sut.template resolve<std::shared_ptr<Singleton>>();
  EXPECT_EQ(s1.get(), s2.get());
}

TEST_F(ContainerMixedScopesTest, all_scopes_coexist) {
  struct Trans {};
  struct Single {};
  struct Deduced {};
  struct External {
    int_t value = modified_value;
  };

  External ext;

  auto sut = Container{bind<Trans>().in<scope::Transient>(),
                       bind<Single>().in<scope::Singleton>(), bind<Deduced>(),
                       bind<External>().to(ext)};

  // Transient creates new each time
  auto t1 = sut.template resolve<Trans>();
  auto t2 = sut.template resolve<Trans>();
  EXPECT_NE(&t1, &t2);

  // Singleton returns same reference
  auto& s1 = sut.template resolve<Single&>();
  auto& s2 = sut.template resolve<Single&>();
  EXPECT_EQ(&s1, &s2);

  // Deduced caches for references, creates for values
  auto& d1 = sut.template resolve<Deduced&>();
  auto& d2 = sut.template resolve<Deduced&>();
  EXPECT_EQ(&d1, &d2);

  // Instance wraps external
  auto& e1 = sut.template resolve<External&>();
  EXPECT_EQ(&ext, &e1);
}

// ----------------------------------------------------------------------------
// Default Scope Tests
// ----------------------------------------------------------------------------

struct ContainerDefaultScopeTest : ContainerTest {};

TEST_F(ContainerDefaultScopeTest, unbound_type_uses_default_scope) {
  struct Type {};
  struct Unbound {};
  auto sut = Container{bind<Type>()};

  [[maybe_unused]] auto instance = sut.template resolve<Unbound>();
  // Should work - uses default Deduced scope
}

TEST_F(ContainerDefaultScopeTest, unbound_type_with_dependencies) {
  struct Dep {
    int_t value = 10;
    Dep() = default;
  };

  struct Unbound {
    int_t result;
    explicit Unbound(Dep d) : result{d.value * 2} {}
  };

  auto sut = Container{bind<Dep>()};

  auto unbound = sut.template resolve<Unbound>();
  EXPECT_EQ(20, unbound.result);
}

TEST_F(ContainerDefaultScopeTest, unbound_type_caches_for_references) {
  struct Unbound : Counted {};

  auto sut = Container{};

  auto& ref1 = sut.template resolve<Unbound&>();
  auto& ref2 = sut.template resolve<Unbound&>();

  EXPECT_EQ(&ref1, &ref2);
  EXPECT_EQ(0, ref1.id);
  EXPECT_EQ(1, Counted::num_instances);
}

TEST_F(ContainerDefaultScopeTest, unbound_type_creates_values) {
  struct Unbound : Counted {};

  auto sut = Container{};

  auto val1 = sut.template resolve<Unbound>();
  auto val2 = sut.template resolve<Unbound>();

  EXPECT_NE(&val1, &val2);
  EXPECT_EQ(0, val1.id);
  EXPECT_EQ(1, val2.id);
}

// ----------------------------------------------------------------------------
// Promotion Tests (Transient -> Singleton-like behavior)
// ----------------------------------------------------------------------------
//
// Promotion occurs when a type bound as Transient is requested in a way that
// requires shared ownership or reference semantics:
//
// PROMOTED (Transient -> Singleton-like):
// - References (T&, const T&) - must be stable across calls
// - Pointers (T*, const T*) - must point to same instance
// - weak_ptr<T> - requires a cached shared_ptr to track
//
// NOT PROMOTED (remains Transient):
// - Values (T, T&&) - each call creates new instance
// - unique_ptr<T> - exclusive ownership, each call creates new instance
// - shared_ptr<T> - can create new instances in each shared_ptr
//
// Note: shared_ptr<T> from Transient creates new instances each time; it is
// not promoted. This is intentional - transient shared_ptr means "give me a
// new instance in a shared_ptr". Only weak_ptr requires promotion because it
// needs a cached backing shared_ptr to track.
//
// ----------------------------------------------------------------------------

struct ContainerPromotionTest : ContainerTest {};

TEST_F(ContainerPromotionTest, transient_promoted_to_singleton_for_reference) {
  struct TransientBound : Counted {};
  auto sut = Container{bind<TransientBound>().in<scope::Transient>()};

  // First reference request should cache
  auto& ref1 = sut.template resolve<TransientBound&>();
  auto& ref2 = sut.template resolve<TransientBound&>();

  // Should return same instance (promoted to singleton)
  EXPECT_EQ(&ref1, &ref2);
  EXPECT_EQ(0, ref1.id);
  EXPECT_EQ(1, Counted::num_instances);  // Only one instance created
}

TEST_F(ContainerPromotionTest,
       transient_promoted_to_singleton_for_const_reference) {
  struct TransientBound : Counted {};
  auto sut = Container{bind<TransientBound>().in<scope::Transient>()};

  const auto& ref1 = sut.template resolve<const TransientBound&>();
  const auto& ref2 = sut.template resolve<const TransientBound&>();

  EXPECT_EQ(&ref1, &ref2);
  EXPECT_EQ(0, ref1.id);
  EXPECT_EQ(1, Counted::num_instances);
}

TEST_F(ContainerPromotionTest, transient_promoted_to_singleton_for_pointer) {
  struct TransientBound : Counted {};
  auto sut = Container{bind<TransientBound>().in<scope::Transient>()};

  auto* ptr1 = sut.template resolve<TransientBound*>();
  auto* ptr2 = sut.template resolve<TransientBound*>();

  EXPECT_EQ(ptr1, ptr2);
  EXPECT_EQ(0, ptr1->id);
  EXPECT_EQ(1, Counted::num_instances);
}

TEST_F(ContainerPromotionTest,
       transient_promoted_to_singleton_for_const_pointer) {
  struct TransientBound : Counted {};
  auto sut = Container{bind<TransientBound>().in<scope::Transient>()};

  const auto* ptr1 = sut.template resolve<const TransientBound*>();
  const auto* ptr2 = sut.template resolve<const TransientBound*>();

  EXPECT_EQ(ptr1, ptr2);
  EXPECT_EQ(0, ptr1->id);
  EXPECT_EQ(1, Counted::num_instances);
}

TEST_F(ContainerPromotionTest,
       transient_shared_ptr_creates_new_instances_not_promoted) {
  struct TransientBound : Counted {};
  auto sut = Container{bind<TransientBound>().in<scope::Transient>()};

  auto shared1 = sut.template resolve<std::shared_ptr<TransientBound>>();
  auto shared2 = sut.template resolve<std::shared_ptr<TransientBound>>();

  // Transient shared_ptr creates NEW instances (not promoted)
  EXPECT_NE(shared1.get(), shared2.get());
  EXPECT_EQ(0, shared1->id);
  EXPECT_EQ(1, shared2->id);
  EXPECT_EQ(2, Counted::num_instances);
}

TEST_F(ContainerPromotionTest, transient_promoted_to_singleton_for_weak_ptr) {
  struct TransientBound : Counted {};
  auto sut = Container{bind<TransientBound>().in<scope::Transient>()};

  auto weak1 = sut.template resolve<std::weak_ptr<TransientBound>>();
  auto weak2 = sut.template resolve<std::weak_ptr<TransientBound>>();

  EXPECT_FALSE(weak1.expired());
  EXPECT_EQ(weak1.lock(), weak2.lock());
  EXPECT_EQ(0, weak1.lock()->id);
  EXPECT_EQ(1, Counted::num_instances);
}

TEST_F(ContainerPromotionTest, transient_not_promoted_for_value_or_unique_ptr) {
  struct TransientBound : Counted {};
  auto sut = Container{bind<TransientBound>().in<scope::Transient>()};

  // Values should still be transient
  auto val1 = sut.template resolve<TransientBound>();
  auto val2 = sut.template resolve<TransientBound>();
  EXPECT_EQ(0, val1.id);
  EXPECT_EQ(1, val2.id);

  // unique_ptr should still be transient
  auto unique1 = sut.template resolve<std::unique_ptr<TransientBound>>();
  auto unique2 = sut.template resolve<std::unique_ptr<TransientBound>>();
  EXPECT_NE(unique1.get(), unique2.get());
  EXPECT_EQ(2, unique1->id);
  EXPECT_EQ(3, unique2->id);
  EXPECT_EQ(4, Counted::num_instances);
}

TEST_F(ContainerPromotionTest,
       transient_promotion_consistent_across_different_request_types) {
  struct TransientBound : Counted {};
  auto sut = Container{bind<TransientBound>().in<scope::Transient>()};

  // All reference-like requests should share the same promoted instance
  // (but not shared_ptr, which creates new instances)
  auto& ref = sut.template resolve<TransientBound&>();
  auto* ptr = sut.template resolve<TransientBound*>();
  auto weak = sut.template resolve<std::weak_ptr<TransientBound>>();

  EXPECT_EQ(&ref, ptr);
  EXPECT_EQ(ptr, weak.lock().get());
  EXPECT_EQ(0, ref.id);
  EXPECT_EQ(1, Counted::num_instances);
}

TEST_F(ContainerPromotionTest, transient_promotion_with_dependencies) {
  struct Dependency : Counted {};
  struct Service : Counted {
    Dependency* dep;
    explicit Service(Dependency& d) : dep{&d} {}
  };

  auto sut = Container{bind<Dependency>().in<scope::Transient>(),
                       bind<Service>().in<scope::Transient>()};

  // When Service is constructed by reference, it causes Dependency to be
  // promoted (since Service's ctor takes Dependency&, which forces promotion)
  auto& service = sut.template resolve<Service&>();

  // Verify Dependency was promoted (id=0) and Service was promoted (id=1)
  EXPECT_EQ(0, service.dep->id);  // Dependency created first
  EXPECT_EQ(1, service.id);       // Service created second

  // Verify subsequent reference requests return the same promoted instances
  auto& service2 = sut.template resolve<Service&>();
  EXPECT_EQ(&service, &service2);
  EXPECT_EQ(service.dep, service2.dep);

  EXPECT_EQ(2, Counted::num_instances);  // 1 Service + 1 Dependency
}

// ----------------------------------------------------------------------------
// Relegation Tests (Singleton -> Transient-like behavior)
// ----------------------------------------------------------------------------
//
// Relegation occurs when a type bound as Singleton is requested in a way that
// requires exclusive ownership or value semantics:
//
// RELEGATED (Singleton -> Transient-like):
// - Values (T) - creates copies of singleton value
// - rvalue references (T&&) - creates copies of singleton value
// - unique_ptr<T> - exclusive ownership, creates new instances initialized
// with singleton value
//
// NOT RELEGATED (remains Singleton):
// - References (T&, const T&) - returns reference to singleton
// - Pointers (T*, const T*) - returns pointer to singleton
// - shared_ptr<T> - wraps singleton via canonical shared_ptr
// - weak_ptr<T> - tracks the canonical shared_ptr of singleton
//
// Note: Relegated instances are initialized with the value in the singleton at
// the time of creation; they make copies. The singleton instance remains
// unchanged and can still be accessed via references/pointers.
//
// ----------------------------------------------------------------------------

struct ContainerRelegationTest : ContainerTest {};

TEST_F(ContainerRelegationTest, singleton_relegated_to_transient_for_value) {
  struct Type : Counted {};
  auto sut = Container{bind<Type>().in<scope::Singleton>()};

  // Values return copies of the singleton.
  auto val1 = sut.template resolve<Type>();
  auto val2 = sut.template resolve<Type>();

  EXPECT_NE(&val1, &val2);
  EXPECT_EQ(0, val1.id);
  EXPECT_EQ(0, val2.id);
  EXPECT_EQ(1, Counted::num_instances);
}

TEST_F(ContainerRelegationTest,
       singleton_relegated_to_transient_for_rvalue_reference) {
  struct Type : Counted {};
  auto sut = Container{bind<Type>().in<scope::Singleton>()};

  auto&& rval1 = sut.template resolve<Type&&>();
  auto&& rval2 = sut.template resolve<Type&&>();

  EXPECT_NE(&rval1, &rval2);
  EXPECT_EQ(0, rval1.id);
  EXPECT_EQ(0, rval2.id);
  EXPECT_EQ(1, Counted::num_instances);
}

TEST_F(ContainerRelegationTest,
       singleton_relegated_to_transient_for_unique_ptr) {
  struct Type : Counted {};
  auto sut = Container{bind<Type>().in<scope::Singleton>()};

  auto unique1 = sut.template resolve<std::unique_ptr<Type>>();
  auto unique2 = sut.template resolve<std::unique_ptr<Type>>();

  EXPECT_NE(unique1.get(), unique2.get());
  EXPECT_EQ(0, unique1->id);
  EXPECT_EQ(1, unique2->id);
  EXPECT_EQ(2, Counted::num_instances);
}

TEST_F(ContainerRelegationTest,
       singleton_not_relegated_for_references_or_shared_ptr) {
  struct Type : Counted {};
  auto sut = Container{bind<Type>().in<scope::Singleton>()};

  // References should still be singleton
  auto& ref1 = sut.template resolve<Type&>();
  auto& ref2 = sut.template resolve<Type&>();
  EXPECT_EQ(&ref1, &ref2);
  EXPECT_EQ(0, ref1.id);

  // Pointers should still be singleton
  auto* ptr1 = sut.template resolve<Type*>();
  auto* ptr2 = sut.template resolve<Type*>();
  EXPECT_EQ(ptr1, ptr2);
  EXPECT_EQ(&ref1, ptr1);

  // shared_ptr should wrap the singleton (not relegated)
  auto shared1 = sut.template resolve<std::shared_ptr<Type>>();
  auto shared2 = sut.template resolve<std::shared_ptr<Type>>();
  EXPECT_EQ(shared1.get(), shared2.get());
  EXPECT_EQ(&ref1, shared1.get());

  EXPECT_EQ(1, Counted::num_instances);  // Only one instance total
}

TEST_F(ContainerRelegationTest,
       singleton_shared_ptr_wraps_singleton_not_relegated) {
  struct Type : Counted {
    int_t value = initial_value;
    Type() = default;
  };
  auto sut = Container{bind<Type>().in<scope::Singleton>()};

  // Modify the singleton
  auto& singleton = sut.template resolve<Type&>();
  singleton.value = modified_value;

  // shared_ptr should wrap the singleton, showing the modified value
  auto shared = sut.template resolve<std::shared_ptr<Type>>();
  EXPECT_EQ(modified_value, shared->value);
  EXPECT_EQ(&singleton, shared.get());

  // Values are copies of the singleton with the modified value
  auto val = sut.template resolve<Type>();
  EXPECT_EQ(modified_value, val.value);  // Copy of modified singleton
  EXPECT_NE(&singleton, &val);           // But different address

  EXPECT_EQ(1, Counted::num_instances);  // Only 1 singleton instance
}

TEST_F(ContainerRelegationTest,
       singleton_relegation_creates_copies_not_fresh_instances) {
  struct Type : Counted {
    int_t value = initial_value;
    Type() = default;
  };
  auto sut = Container{bind<Type>().in<scope::Singleton>()};

  // Get singleton reference and modify it
  auto& singleton = sut.template resolve<Type&>();
  singleton.value = modified_value;

  // Values are copies of the singleton. This creates copies of the
  // modified singleton, not fresh instances from the provider.
  auto val1 = sut.template resolve<Type>();
  auto val2 = sut.template resolve<Type>();

  // Copies of modified singleton, not default values from provider
  EXPECT_EQ(modified_value, val1.value);
  EXPECT_EQ(modified_value, val2.value);

  // Copies are independent from each other and from singleton
  EXPECT_NE(&singleton, &val1);
  EXPECT_NE(&singleton, &val2);
  EXPECT_NE(&val1, &val2);

  // Singleton itself is unchanged
  EXPECT_EQ(modified_value, singleton.value);
}

TEST_F(ContainerRelegationTest, singleton_relegation_with_dependencies) {
  struct Dependency : Counted {
    int_t value = initial_value;
    Dependency() = default;
  };
  struct Service : Counted {
    Dependency dep;
    explicit Service(Dependency d) : dep{d} {}
  };

  auto sut = Container{bind<Dependency>().in<scope::Singleton>(),
                       bind<Service>().in<scope::Singleton>()};

  // Service value is a copy of the Service singleton
  // The Service singleton contains a copy of the Dependency singleton
  // Each value resolution creates independent copies
  auto service1 = sut.template resolve<Service>();
  auto service2 = sut.template resolve<Service>();

  EXPECT_NE(&service1, &service2);          // Independent copies
  EXPECT_NE(&service1.dep, &service2.dep);  // Each copy has its own dep copy

  // Singletons created: Dependency (id=0) then Service (id=1)
  EXPECT_EQ(0, service1.dep.id);  // Copy of Dependency singleton
  EXPECT_EQ(1, service1.id);      // Copy of Service singleton

  // Both values are copies of the same singletons
  EXPECT_EQ(0, service2.dep.id);  // Copy of same Dependency singleton
  EXPECT_EQ(1, service2.id);      // Copy of same Service singleton

  EXPECT_EQ(
      2,
      Counted::num_instances);  // 1 Service singleton + 1 Dependency singleton
}

// ----------------------------------------------------------------------------
// Hierarchical Container Tests - Basic Delegation
// ----------------------------------------------------------------------------

struct ContainerHierarchyTest : ContainerTest {};

TEST_F(ContainerHierarchyTest, child_finds_binding_in_parent) {
  struct ParentBound {
    int_t value = initial_value;
    ParentBound() = default;
  };

  auto parent = Container{bind<ParentBound>()};
  auto child = Container{parent};

  auto result = child.template resolve<ParentBound>();
  EXPECT_EQ(initial_value, result.value);
}

TEST_F(ContainerHierarchyTest, child_overrides_parent_binding) {
  struct Bound {
    int_t value;
    explicit Bound(int_t v = 0) : value{v} {}
  };

  auto parent_factory = []() { return Bound{initial_value}; };
  auto child_factory = []() { return Bound{modified_value}; };

  auto parent = Container{bind<Bound>().as<Bound>().via(parent_factory)};
  auto child = Container{parent, bind<Bound>().as<Bound>().via(child_factory)};

  auto parent_result = parent.template resolve<Bound>();
  auto child_result = child.template resolve<Bound>();

  EXPECT_EQ(initial_value, parent_result.value);
  EXPECT_EQ(modified_value, child_result.value);
}

TEST_F(ContainerHierarchyTest, multi_level_hierarchy) {
  struct GrandparentBound {
    int_t value = 1;
    GrandparentBound() = default;
  };
  struct ParentBound {
    int_t value = 2;
    ParentBound() = default;
  };
  struct ChildBound {
    int_t value = 3;
    ChildBound() = default;
  };

  auto grandparent = Container{bind<GrandparentBound>()};
  auto parent = Container{grandparent, bind<ParentBound>()};
  auto child = Container{parent, bind<ChildBound>()};

  // Child can resolve from all levels
  auto g_result = child.template resolve<GrandparentBound>();
  auto p_result = child.template resolve<ParentBound>();
  auto c_result = child.template resolve<ChildBound>();

  EXPECT_EQ(1, g_result.value);
  EXPECT_EQ(2, p_result.value);
  EXPECT_EQ(3, c_result.value);
}

TEST_F(ContainerHierarchyTest,
       child_overrides_parent_in_multi_level_hierarchy) {
  struct Bound {
    int_t value;
    explicit Bound(int_t v = 0) : value{v} {}
  };

  auto grandparent_factory = []() { return Bound{1}; };
  auto parent_factory = []() { return Bound{2}; };
  auto child_factory = []() { return Bound{3}; };

  auto grandparent =
      Container{bind<Bound>().as<Bound>().via(grandparent_factory)};
  auto parent =
      Container{grandparent, bind<Bound>().as<Bound>().via(parent_factory)};
  auto child = Container{parent, bind<Bound>().as<Bound>().via(child_factory)};

  auto grandparent_result = grandparent.template resolve<Bound>();
  auto parent_result = parent.template resolve<Bound>();
  auto child_result = child.template resolve<Bound>();

  EXPECT_EQ(1, grandparent_result.value);
  EXPECT_EQ(2, parent_result.value);
  EXPECT_EQ(3, child_result.value);
}

TEST_F(ContainerHierarchyTest, unbound_type_uses_fallback_in_hierarchy) {
  struct Unbound {
    int_t value = initial_value;
    Unbound() = default;
  };

  auto parent = Container{};
  auto child = Container{parent};

  // Should use fallback binding at the root level
  auto result = child.template resolve<Unbound>();
  EXPECT_EQ(initial_value, result.value);
}

// ----------------------------------------------------------------------------
// Hierarchical Container Tests - Singleton Sharing
// ----------------------------------------------------------------------------

struct ContainerHierarchySingletonTest : ContainerTest {};

TEST_F(ContainerHierarchySingletonTest, singleton_in_parent_shared_with_child) {
  struct Type : Counted {};

  auto parent = Container{bind<Type>().in<scope::Singleton>()};
  auto child = Container{parent};

  auto& parent_ref = parent.template resolve<Type&>();
  auto& child_ref = child.template resolve<Type&>();

  EXPECT_EQ(&parent_ref, &child_ref);
  EXPECT_EQ(0, parent_ref.id);
  EXPECT_EQ(1, Counted::num_instances);
}

TEST_F(ContainerHierarchySingletonTest,
       singleton_in_grandparent_shared_with_all) {
  struct Type : Counted {};

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

TEST_F(ContainerHierarchySingletonTest,
       child_singleton_does_not_affect_parent) {
  struct Type : Counted {};

  auto parent = Container{};
  auto child = Container{parent, bind<Type>().in<scope::Singleton>()};

  auto& child_ref = child.template resolve<Type&>();
  // Parent should create new instance (fallback)
  auto& parent_ref = parent.template resolve<Type&>();

  EXPECT_NE(&child_ref, &parent_ref);
  EXPECT_EQ(0, child_ref.id);
  EXPECT_EQ(1, parent_ref.id);
  EXPECT_EQ(2, Counted::num_instances);
}

TEST_F(ContainerHierarchySingletonTest,
       parent_and_child_can_have_separate_singletons) {
  struct Bound : Counted {};

  auto parent = Container{bind<Bound>().in<scope::Singleton>()};
  auto child = Container{parent, bind<Bound>().in<scope::Singleton>()};

  auto& parent_ref = parent.template resolve<Bound&>();
  auto& child_ref = child.template resolve<Bound&>();

  // Child overrides, so they should be different
  EXPECT_NE(&parent_ref, &child_ref);
  EXPECT_EQ(0, parent_ref.id);
  EXPECT_EQ(1, child_ref.id);
  EXPECT_EQ(2, Counted::num_instances);
}

// ----------------------------------------------------------------------------
// Hierarchical Container Tests - Transient Behavior
// ----------------------------------------------------------------------------

struct ContainerHierarchyTransientTest : ContainerTest {};

TEST_F(ContainerHierarchyTransientTest,
       transient_in_parent_creates_new_instances_for_child) {
  struct TransientBound : Counted {};

  auto parent = Container{bind<TransientBound>().in<scope::Transient>()};
  auto child = Container{parent};

  auto parent_val1 = parent.template resolve<TransientBound>();
  auto child_val1 = child.template resolve<TransientBound>();
  auto child_val2 = child.template resolve<TransientBound>();

  EXPECT_EQ(0, parent_val1.id);
  EXPECT_EQ(1, child_val1.id);
  EXPECT_EQ(2, child_val2.id);
  EXPECT_EQ(3, Counted::num_instances);
}

TEST_F(ContainerHierarchyTransientTest,
       transient_in_grandparent_creates_new_instances_for_all) {
  struct TransientBound : Counted {};

  auto grandparent = Container{bind<TransientBound>().in<scope::Transient>()};
  auto parent = Container{grandparent};
  auto child = Container{parent};

  auto grandparent_val = grandparent.template resolve<TransientBound>();
  auto parent_val = parent.template resolve<TransientBound>();
  auto child_val = child.template resolve<TransientBound>();

  EXPECT_EQ(0, grandparent_val.id);
  EXPECT_EQ(1, parent_val.id);
  EXPECT_EQ(2, child_val.id);
  EXPECT_EQ(3, Counted::num_instances);
}

// ----------------------------------------------------------------------------
// Hierarchical Container Tests - Promotion in Hierarchy
// ----------------------------------------------------------------------------

struct ContainerHierarchyPromotionTest : ContainerTest {};

TEST_F(ContainerHierarchyPromotionTest, child_promotes_transient_from_parent) {
  struct TransientBound : Counted {};

  auto parent = Container{bind<TransientBound>().in<scope::Transient>()};
  auto child = Container{parent};

  // Child requests by reference, should promote
  auto& child_ref1 = child.template resolve<TransientBound&>();
  auto& child_ref2 = child.template resolve<TransientBound&>();

  EXPECT_EQ(&child_ref1, &child_ref2);
  EXPECT_EQ(0, child_ref1.id);
  EXPECT_EQ(1, Counted::num_instances);
}

TEST_F(ContainerHierarchyPromotionTest,
       child_shares_parent_promoted_instance_when_delegating) {
  struct TransientBound : Counted {};

  auto parent = Container{bind<TransientBound>().in<scope::Transient>()};
  auto child = Container{parent};  // Child has no binding, will delegate

  // Parent promotes to singleton when requested by reference
  auto& parent_ref = parent.template resolve<TransientBound&>();

  // Child delegates to parent, gets same promoted instance
  auto& child_ref = child.template resolve<TransientBound&>();

  EXPECT_EQ(&parent_ref, &child_ref);  // Same instance!
  EXPECT_EQ(0, parent_ref.id);
  EXPECT_EQ(1, Counted::num_instances);  // Only one instance created
}

TEST_F(ContainerHierarchyPromotionTest,
       child_has_separate_promoted_instance_with_own_binding) {
  struct TransientBound : Counted {};

  auto parent = Container{bind<TransientBound>().in<scope::Transient>()};
  auto child = Container{parent, bind<TransientBound>().in<scope::Transient>()};

  // Each promotes separately because each has its own binding
  auto& parent_ref = parent.template resolve<TransientBound&>();
  auto& child_ref = child.template resolve<TransientBound&>();

  EXPECT_NE(&parent_ref, &child_ref);  // Different instances!
  EXPECT_EQ(0, parent_ref.id);
  EXPECT_EQ(1, child_ref.id);
  EXPECT_EQ(2, Counted::num_instances);
}

TEST_F(ContainerHierarchyPromotionTest,
       grandparent_parent_child_share_promoted_instance_when_delegating) {
  struct TransientBound : Counted {};

  auto grandparent = Container{bind<TransientBound>().in<scope::Transient>()};
  auto parent = Container{grandparent};  // No binding, delegates to grandparent
  auto child =
      Container{parent};  // No binding, delegates to parent -> grandparent

  auto& grandparent_ref = grandparent.template resolve<TransientBound&>();
  auto& parent_ref = parent.template resolve<TransientBound&>();
  auto& child_ref = child.template resolve<TransientBound&>();

  // All share grandparent's promoted instance
  EXPECT_EQ(&grandparent_ref, &parent_ref);
  EXPECT_EQ(&parent_ref, &child_ref);
  EXPECT_EQ(0, grandparent_ref.id);
  EXPECT_EQ(1, Counted::num_instances);
}

// Ancestry is part of a container's type, so ancestors can all have the same
// bindings but remain unique types and have separate cached instances.
TEST_F(ContainerHierarchyPromotionTest,
       ancestry_with_same_bindings_promote_separate_instances) {
  struct TransientBound : Counted {};

  auto grandparent = Container{bind<TransientBound>().in<scope::Transient>()};
  auto parent =
      Container{grandparent, bind<TransientBound>().in<scope::Transient>()};
  auto child = Container{parent, bind<TransientBound>().in<scope::Transient>()};

  auto& grandparent_ref = grandparent.template resolve<TransientBound&>();
  auto& parent_ref = parent.template resolve<TransientBound&>();
  auto& child_ref = child.template resolve<TransientBound&>();

  // Each has its own promoted instance
  EXPECT_NE(&grandparent_ref, &parent_ref);
  EXPECT_NE(&parent_ref, &child_ref);
  EXPECT_EQ(0, grandparent_ref.id);
  EXPECT_EQ(1, parent_ref.id);
  EXPECT_EQ(2, child_ref.id);
  EXPECT_EQ(3, Counted::num_instances);
}

// ----------------------------------------------------------------------------
// Hierarchical Container Tests - Relegation in Hierarchy
// ----------------------------------------------------------------------------

struct ContainerHierarchyRelegationTest : ContainerTest {};

TEST_F(ContainerHierarchyRelegationTest,
       child_relegates_singleton_from_parent) {
  struct Type : Counted {};

  auto parent = Container{bind<Type>().in<scope::Singleton>()};
  auto child = Container{parent};

  // Child requests by value, gets copies of parent's singleton
  auto child_val1 = child.template resolve<Type>();
  auto child_val2 = child.template resolve<Type>();

  EXPECT_NE(&child_val1, &child_val2);   // Different copies
  EXPECT_EQ(0, child_val1.id);           // Both copies of same singleton
  EXPECT_EQ(0, child_val2.id);           // Both copies of same singleton
  EXPECT_EQ(1, Counted::num_instances);  // Only parent's singleton
}

TEST_F(ContainerHierarchyRelegationTest,
       parent_singleton_reference_differs_from_child_relegated_values) {
  struct Type : Counted {};

  auto parent = Container{bind<Type>().in<scope::Singleton>()};
  auto child = Container{parent};

  auto& parent_ref = parent.template resolve<Type&>();
  auto child_val = child.template resolve<Type>();

  EXPECT_NE(&parent_ref, &child_val);    // Value is a copy
  EXPECT_EQ(0, parent_ref.id);           // Singleton
  EXPECT_EQ(0, child_val.id);            // Copy of same singleton
  EXPECT_EQ(1, Counted::num_instances);  // Only 1 singleton
}

TEST_F(ContainerHierarchyRelegationTest,
       grandparent_singleton_reference_accessible_but_child_can_relegate) {
  struct Type : Counted {};

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

// ----------------------------------------------------------------------------
// Complex Hierarchical Scenarios
// ----------------------------------------------------------------------------

struct ContainerHierarchyComplexTest : ContainerTest {};

TEST_F(ContainerHierarchyComplexTest, mixed_scopes_across_hierarchy) {
  struct SingletonInGrandparent : Counted {};
  struct TransientInParent : Counted {};
  struct SingletonInChild : Counted {};

  auto grandparent =
      Container{bind<SingletonInGrandparent>().in<scope::Singleton>()};
  auto parent =
      Container{grandparent, bind<TransientInParent>().in<scope::Transient>()};
  auto child =
      Container{parent, bind<SingletonInChild>().in<scope::Singleton>()};

  // Singleton from grandparent shared
  auto& sg1 = child.template resolve<SingletonInGrandparent&>();
  auto& sg2 = child.template resolve<SingletonInGrandparent&>();
  EXPECT_EQ(&sg1, &sg2);
  EXPECT_EQ(0, sg1.id);

  // Transient from parent creates new instances
  auto tp1 = child.template resolve<TransientInParent>();
  auto tp2 = child.template resolve<TransientInParent>();
  EXPECT_NE(&tp1, &tp2);
  EXPECT_EQ(1, tp1.id);
  EXPECT_EQ(2, tp2.id);

  // Singleton in child
  auto& sc1 = child.template resolve<SingletonInChild&>();
  auto& sc2 = child.template resolve<SingletonInChild&>();
  EXPECT_EQ(&sc1, &sc2);
  EXPECT_EQ(3, sc1.id);

  EXPECT_EQ(4, Counted::num_instances);
}

TEST_F(ContainerHierarchyComplexTest, dependency_chain_across_hierarchy) {
  struct GrandparentDep : Counted {};
  struct ParentDep : Counted {
    GrandparentDep* dep;
    explicit ParentDep(GrandparentDep& d) : dep{&d} {}
  };
  struct ChildService : Counted {
    ParentDep* dep;
    explicit ChildService(ParentDep& d) : dep{&d} {}
  };

  auto grandparent = Container{bind<GrandparentDep>().in<scope::Singleton>()};
  auto parent = Container{grandparent, bind<ParentDep>()};
  auto child = Container{parent, bind<ChildService>()};

  auto& service = child.template resolve<ChildService&>();

  EXPECT_EQ(0, service.dep->dep->id);  // GrandparentDep
  EXPECT_EQ(1, service.dep->id);       // ParentDep
  EXPECT_EQ(2, service.id);            // ChildService
  EXPECT_EQ(3, Counted::num_instances);
}

TEST_F(ContainerHierarchyComplexTest,
       promotion_and_relegation_across_hierarchy) {
  struct Bound : Counted {};

  auto parent = Container{bind<Bound>().in<scope::Transient>()};
  auto child = Container{parent, bind<Bound>().in<scope::Singleton>()};

  // Parent transient promoted to singleton
  auto& parent_ref1 = parent.template resolve<Bound&>();
  auto& parent_ref2 = parent.template resolve<Bound&>();
  EXPECT_EQ(&parent_ref1, &parent_ref2);
  EXPECT_EQ(0, parent_ref1.id);

  // Child singleton
  auto& child_ref = child.template resolve<Bound&>();
  EXPECT_EQ(1, child_ref.id);

  // Child singleton values are copies
  auto child_val1 = child.template resolve<Bound>();
  auto child_val2 = child.template resolve<Bound>();
  EXPECT_NE(&child_val1, &child_val2);  // Different copies
  EXPECT_EQ(1, child_val1.id);          // Both copies of child singleton
  EXPECT_EQ(1, child_val2.id);          // Both copies of child singleton

  EXPECT_EQ(2, Counted::num_instances);  // 1 parent + 1 child singleton
}

TEST_F(ContainerHierarchyComplexTest,
       sibling_containers_share_parent_promotion_when_delegating) {
  struct Bound : Counted {};

  auto parent = Container{bind<Bound>().in<scope::Transient>()};
  auto child1 = Container{parent};  // No binding, delegates to parent
  auto child2 = Container{parent};  // No binding, delegates to parent

  // Both children delegate to parent, share parent's promoted instance
  auto& child1_ref = child1.template resolve<Bound&>();
  auto& child2_ref = child2.template resolve<Bound&>();

  EXPECT_EQ(&child1_ref, &child2_ref);  // Same instance!
  EXPECT_EQ(0, child1_ref.id);
  EXPECT_EQ(1, Counted::num_instances);
}

// This test shows a surprising result that can't be avoided.
//
// Two containers with the same type cache the same singletons. This is because
// they are cached in Meyers singletons keyed on container and provider. When
// they are the same, the same singletons are found. The solution is found in
// sibling_containers_using_macro_are_independent_with_own_bindings: use
// dink_unique_container to distinguish between containers with the same
// bindings.
TEST_F(ContainerHierarchyComplexTest,
       sibling_containers_with_same_type_share_singletons) {
  struct Bound : Counted {};

  auto parent = Container{bind<Bound>().in<scope::Transient>()};

  // these two have the same type
  auto child1 = Container(parent, bind<Bound>().in<scope::Singleton>());
  auto child2 = Container(parent, bind<Bound>().in<scope::Singleton>());
  static_assert(std::same_as<decltype(child1), decltype(child2)>);

  // Children with the same type have the same binding, even during promotion.
  auto& child1_ref = child1.template resolve<Bound&>();
  auto& child2_ref = child2.template resolve<Bound&>();

  EXPECT_EQ(&child1_ref, &child2_ref);
  EXPECT_EQ(0, child1_ref.id);
  EXPECT_EQ(0, child2_ref.id);
  EXPECT_EQ(1, Counted::num_instances);
}

// Promoted instances are real singletons.
TEST_F(ContainerHierarchyComplexTest,
       sibling_containers_with_same_promoted_type_share_singletons) {
  struct Bound : Counted {};

  auto parent = Container{bind<Bound>().in<scope::Transient>()};

  // these two have the same type
  auto child1 = Container(parent, bind<Bound>().in<scope::Transient>());
  auto child2 = Container(parent, bind<Bound>().in<scope::Transient>());
  static_assert(std::same_as<decltype(child1), decltype(child2)>);

  // Children with the same type have the same binding, even during promotion.
  auto& child1_ref = child1.template resolve<Bound&>();
  auto& child2_ref = child2.template resolve<Bound&>();

  EXPECT_EQ(&child1_ref, &child2_ref);
  EXPECT_EQ(0, child1_ref.id);
  EXPECT_EQ(0, child2_ref.id);
  EXPECT_EQ(1, Counted::num_instances);
}

TEST_F(ContainerHierarchyComplexTest,
       sibling_containers_using_macro_are_independent_with_own_bindings) {
  struct Bound : Counted {};

  auto parent = Container{bind<Bound>().in<scope::Transient>()};

  // using dink_unique_container, these two have unique types
  auto child1 =
      dink_unique_container(parent, bind<Bound>().in<scope::Singleton>());
  auto child2 =
      dink_unique_container(parent, bind<Bound>().in<scope::Singleton>());
  static_assert(!std::same_as<decltype(child1), decltype(child2)>);

  // Each child has own binding, promotes separately
  auto& child1_ref = child1.template resolve<Bound&>();
  auto& child2_ref = child2.template resolve<Bound&>();

  EXPECT_NE(&child1_ref, &child2_ref);
  EXPECT_EQ(0, child1_ref.id);
  EXPECT_EQ(1, child2_ref.id);
  EXPECT_EQ(2, Counted::num_instances);
}

TEST_F(ContainerHierarchyComplexTest,
       repeated_macro_invocations_create_unique_types) {
  auto c1 = dink_unique_container();
  auto c2 = dink_unique_container(c1);
  auto c3 = dink_unique_container(c1);

  static_assert(!std::same_as<decltype(c1), decltype(c2)>);
  static_assert(!std::same_as<decltype(c2), decltype(c3)>);
  static_assert(!std::same_as<decltype(c1), decltype(c3)>);
}

TEST_F(ContainerHierarchyComplexTest,
       promoted_transitive_instances_are_root_singletons) {
  struct Bound : Counted {};

  auto parent = Container{bind<Bound>().in<scope::Singleton>()};
  auto child = Container{parent};

  auto& parent_ref = parent.template resolve<Bound&>();
  auto& child_ref = child.template resolve<Bound&>();

  EXPECT_EQ(&parent_ref, &child_ref);
  EXPECT_EQ(0, parent_ref.id);
  EXPECT_EQ(0, child_ref.id);
  EXPECT_EQ(1, Counted::num_instances);
}

TEST_F(ContainerHierarchyComplexTest, deep_hierarchy_with_multiple_overrides) {
  struct Bound {
    int_t value;
    explicit Bound(int_t v = 0) : value{v} {}
  };

  auto level0_factory = []() { return Bound{0}; };
  auto level2_factory = []() { return Bound{2}; };
  auto level4_factory = []() { return Bound{4}; };

  auto level0 = Container{bind<Bound>().as<Bound>().via(level0_factory)};
  auto level1 = Container{level0};
  auto level2 =
      Container{level1, bind<Bound>().as<Bound>().via(level2_factory)};
  auto level3 = Container{level2};
  auto level4 =
      Container{level3, bind<Bound>().as<Bound>().via(level4_factory)};

  auto r0 = level0.template resolve<Bound>();
  auto r1 = level1.template resolve<Bound>();
  auto r2 = level2.template resolve<Bound>();
  auto r3 = level3.template resolve<Bound>();
  auto r4 = level4.template resolve<Bound>();

  EXPECT_EQ(0, r0.value);
  EXPECT_EQ(0, r1.value);  // Inherits from level0
  EXPECT_EQ(2, r2.value);  // Overrides
  EXPECT_EQ(2, r3.value);  // Inherits from level2
  EXPECT_EQ(4, r4.value);  // Overrides
}

}  // namespace
}  // namespace dink
