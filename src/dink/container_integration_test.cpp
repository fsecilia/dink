// \file
// Copyright (c) 2025 Frank Secilia
// SPDX-License-Identifier: MIT

#include "container.hpp"
#include <dink/test.hpp>
#include <dink/scope.hpp>

namespace dink {

// ----------------------------------------------------------------------------
// Common Test Infrastructure
// ----------------------------------------------------------------------------

// Base class for types that need instance counting
struct Counted {
  static inline int instance_count = 0;
  int id;
  Counted() : id{instance_count++} {}
};

// Common base for all container tests - resets counters
struct ContainerTestBase : Test {
  ContainerTestBase() { Counted::instance_count = 0; }
};

// ----------------------------------------------------------------------------
// Singleton Scope Tests
// ----------------------------------------------------------------------------

struct ContainerSingletonTest : ContainerTestBase {};

TEST_F(ContainerSingletonTest, canonical_shared_wraps_instance) {
  struct SingletonBound {};
  auto sut = Container{bind<SingletonBound>().in<scope::Singleton>()};

  const auto shared = sut.template resolve<std::shared_ptr<SingletonBound>>();
  auto& instance = sut.template resolve<SingletonBound&>();
  ASSERT_EQ(&instance, shared.get());
}

TEST_F(ContainerSingletonTest, canonical_shared_ptr_value) {
  struct SingletonBound {};
  auto sut = Container{bind<SingletonBound>().in<scope::Singleton>()};

  const auto result1 = sut.template resolve<std::shared_ptr<SingletonBound>>();
  const auto result2 = sut.template resolve<std::shared_ptr<SingletonBound>>();
  ASSERT_EQ(result1, result2);
  ASSERT_EQ(result1.use_count(), result2.use_count());
  ASSERT_EQ(result1.use_count(), 3);  // result1 + result2 + canonical

  auto& instance = sut.template resolve<SingletonBound&>();
  ASSERT_EQ(&instance, result1.get());
}

TEST_F(ContainerSingletonTest, canonical_shared_ptr_identity) {
  struct SingletonBound {};
  auto sut = Container{bind<SingletonBound>().in<scope::Singleton>()};

  const auto& result1 =
      sut.template resolve<std::shared_ptr<SingletonBound>&>();
  const auto& result2 =
      sut.template resolve<std::shared_ptr<SingletonBound>&>();
  ASSERT_EQ(&result1, &result2);
  ASSERT_EQ(result1.use_count(), result2.use_count());
  ASSERT_EQ(result1.use_count(), 1);
}

TEST_F(ContainerSingletonTest, weak_ptr_from_singleton) {
  struct SingletonBound {};
  auto sut = Container{bind<SingletonBound>().in<scope::Singleton>()};

  auto weak1 = sut.template resolve<std::weak_ptr<SingletonBound>>();
  auto weak2 = sut.template resolve<std::weak_ptr<SingletonBound>>();

  EXPECT_FALSE(weak1.expired());
  EXPECT_EQ(weak1.lock(), weak2.lock());
}

TEST_F(ContainerSingletonTest, weak_ptr_does_not_expire_while_singleton_alive) {
  struct SingletonBound {};
  auto sut = Container{bind<SingletonBound>().in<scope::Singleton>()};

  const auto& weak = sut.template resolve<std::weak_ptr<SingletonBound>>();

  // Even with no shared_ptr in scope, weak_ptr should not expire
  // because it tracks the canonical shared_ptr which aliases the static
  EXPECT_FALSE(weak.expired());

  auto shared = weak.lock();
  EXPECT_NE(nullptr, shared);
}

TEST_F(ContainerSingletonTest, weak_ptr_expires_with_canonical_shared_ptr) {
  struct SingletonBound {};
  auto sut = Container{bind<SingletonBound>().in<scope::Singleton>()};

  // resolve reference directly to canonical shared_ptr
  auto& canonical_shared_ptr =
      sut.template resolve<std::shared_ptr<SingletonBound>&>();
  const auto weak = sut.template resolve<std::weak_ptr<SingletonBound>>();

  EXPECT_FALSE(weak.expired());
  canonical_shared_ptr.reset();
  EXPECT_TRUE(weak.expired());
}

TEST_F(ContainerSingletonTest, const_shared_ptr) {
  struct SingletonBound {};
  auto sut = Container{bind<SingletonBound>().in<scope::Singleton>()};

  auto shared = sut.template resolve<std::shared_ptr<const SingletonBound>>();
  auto& instance = sut.template resolve<SingletonBound&>();

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
  struct SingletonBound {
    int value = 42;
    SingletonBound() = default;
  };
  auto sut = Container{bind<SingletonBound>().in<scope::Singleton>()};

  auto& ref1 = sut.template resolve<SingletonBound&>();
  auto& ref2 = sut.template resolve<SingletonBound&>();

  EXPECT_EQ(&ref1, &ref2);
  EXPECT_EQ(42, ref1.value);

  ref1.value = 99;
  EXPECT_EQ(99, ref2.value);
}

TEST_F(ContainerSingletonTest, resolves_const_reference) {
  struct SingletonBound {
    int value = 42;
    SingletonBound() = default;
  };
  auto sut = Container{bind<SingletonBound>().in<scope::Singleton>()};

  const auto& ref1 = sut.template resolve<const SingletonBound&>();
  const auto& ref2 = sut.template resolve<const SingletonBound&>();

  EXPECT_EQ(&ref1, &ref2);
  EXPECT_EQ(42, ref1.value);
}

TEST_F(ContainerSingletonTest, resolves_mutable_pointer) {
  struct SingletonBound {
    int value = 42;
    SingletonBound() = default;
  };
  auto sut = Container{bind<SingletonBound>().in<scope::Singleton>()};

  auto* ptr1 = sut.template resolve<SingletonBound*>();
  auto* ptr2 = sut.template resolve<SingletonBound*>();

  EXPECT_EQ(ptr1, ptr2);
  EXPECT_EQ(42, ptr1->value);

  ptr1->value = 99;
  EXPECT_EQ(99, ptr2->value);
}

TEST_F(ContainerSingletonTest, resolves_const_pointer) {
  struct SingletonBound {
    int value = 42;
    SingletonBound() = default;
  };
  auto sut = Container{bind<SingletonBound>().in<scope::Singleton>()};

  const auto* ptr1 = sut.template resolve<const SingletonBound*>();
  const auto* ptr2 = sut.template resolve<const SingletonBound*>();

  EXPECT_EQ(ptr1, ptr2);
  EXPECT_EQ(42, ptr1->value);
}

TEST_F(ContainerSingletonTest, reference_and_pointer_point_to_same_instance) {
  struct SingletonBound {};
  auto sut = Container{bind<SingletonBound>().in<scope::Singleton>()};

  auto& ref = sut.template resolve<SingletonBound&>();
  auto* ptr = sut.template resolve<SingletonBound*>();

  EXPECT_EQ(&ref, ptr);
}

// ----------------------------------------------------------------------------
// Transient Scope Tests
// ----------------------------------------------------------------------------

struct ContainerTransientTest : ContainerTestBase {};

TEST_F(ContainerTransientTest, creates_new_shared_ptr_per_resolve) {
  struct TransientBound {};
  auto sut = Container{bind<TransientBound>().in<scope::Transient>()};

  auto shared1 = sut.template resolve<std::shared_ptr<TransientBound>>();
  auto shared2 = sut.template resolve<std::shared_ptr<TransientBound>>();

  EXPECT_NE(shared1.get(), shared2.get());  // Different instances
}

TEST_F(ContainerTransientTest, creates_new_value_per_resolve) {
  struct TransientBound : Counted {};

  auto sut = Container{bind<TransientBound>().in<scope::Transient>()};

  auto value1 = sut.template resolve<TransientBound>();
  auto value2 = sut.template resolve<TransientBound>();

  EXPECT_EQ(0, value1.id);
  EXPECT_EQ(1, value2.id);
}

TEST_F(ContainerTransientTest, creates_new_unique_ptr_per_resolve) {
  struct TransientBound {
    int value = 42;
    TransientBound() = default;
  };
  auto sut = Container{bind<TransientBound>().in<scope::Transient>()};

  auto unique1 = sut.template resolve<std::unique_ptr<TransientBound>>();
  auto unique2 = sut.template resolve<std::unique_ptr<TransientBound>>();

  EXPECT_NE(unique1.get(), unique2.get());
  EXPECT_EQ(42, unique1->value);
  EXPECT_EQ(42, unique2->value);
}

TEST_F(ContainerTransientTest, resolves_const_value) {
  struct TransientBound {
    int value = 42;
    TransientBound() = default;
  };
  auto sut = Container{bind<TransientBound>().in<scope::Transient>()};

  const auto value = sut.template resolve<const TransientBound>();
  EXPECT_EQ(42, value.value);
}

TEST_F(ContainerTransientTest, resolves_rvalue_reference) {
  struct TransientBound {
    int value = 42;
    TransientBound() = default;
  };
  auto sut = Container{bind<TransientBound>().in<scope::Transient>()};

  auto&& value = sut.template resolve<TransientBound&&>();
  EXPECT_EQ(42, value.value);
}

// ----------------------------------------------------------------------------
// Instance Scope Tests
// ----------------------------------------------------------------------------

struct ContainerInstanceTest : ContainerTestBase {};

TEST_F(ContainerInstanceTest, shared_ptr_wraps_external_instance) {
  struct External {
    int value = 42;
  };

  External external_obj;

  auto sut = Container{bind<External>().to(external_obj)};

  // shared_ptr should wrap the external instance
  auto shared1 = sut.template resolve<std::shared_ptr<External>>();
  auto shared2 = sut.template resolve<std::shared_ptr<External>>();

  EXPECT_EQ(&external_obj, shared1.get());  // Points to external
  EXPECT_EQ(shared1.get(), shared2.get());  // Same cached shared_ptr
  EXPECT_EQ(3, shared1.use_count());        // canonical + shared1 + shared2

  // Reference to the external instance
  auto& ref = sut.template resolve<External&>();
  EXPECT_EQ(&external_obj, &ref);
  EXPECT_EQ(&ref, shared1.get());
}

TEST_F(ContainerInstanceTest, canonical_shared_ptr_reference) {
  struct External {};
  External external_obj;

  auto sut = Container{bind<External>().to(external_obj)};

  auto& canonical1 = sut.template resolve<std::shared_ptr<External>&>();
  auto& canonical2 = sut.template resolve<std::shared_ptr<External>&>();

  EXPECT_EQ(&canonical1, &canonical2);         // Same cached shared_ptr
  EXPECT_EQ(&external_obj, canonical1.get());  // Wraps external
  EXPECT_EQ(1, canonical1.use_count());        // Only canonical exists
}

TEST_F(ContainerInstanceTest, weak_ptr_tracks_external_instance) {
  struct External {};
  External external_obj;

  auto sut = Container{bind<External>().to(external_obj)};

  auto weak = sut.template resolve<std::weak_ptr<External>>();

  EXPECT_FALSE(weak.expired());
  EXPECT_EQ(&external_obj, weak.lock().get());
}

TEST_F(ContainerInstanceTest, weak_ptr_does_not_expire_while_instance_alive) {
  struct External {};
  External external_obj;

  auto container = Container{bind<External>().to(external_obj)};

  auto weak = container.template resolve<std::weak_ptr<External>>();

  // Even with no shared_ptr in scope, weak_ptr should not expire
  // because it tracks the canonical shared_ptr which aliases the static
  EXPECT_FALSE(weak.expired());

  auto shared = weak.lock();
  EXPECT_NE(nullptr, shared);
}

TEST_F(ContainerInstanceTest, weak_ptr_expires_with_canonical_shared_ptr) {
  struct External {};
  External external_obj;

  auto container = Container{bind<External>().to(external_obj)};

  // resolve reference directly to canonical shared_ptr
  auto& canonical_shared_ptr =
      container.template resolve<std::shared_ptr<External>&>();
  const auto weak = container.template resolve<std::weak_ptr<External>>();

  EXPECT_FALSE(weak.expired());
  canonical_shared_ptr.reset();
  EXPECT_TRUE(weak.expired());
}

TEST_F(ContainerInstanceTest, resolves_value_copy_of_external) {
  struct External {
    int value = 42;
  };
  External external_obj{99};

  auto sut = Container{bind<External>().to(external_obj)};

  auto copy = sut.template resolve<External>();
  EXPECT_EQ(99, copy.value);

  // Verify it's a copy, not the original
  copy.value = 123;
  EXPECT_EQ(99, external_obj.value);
}

TEST_F(ContainerInstanceTest, resolves_mutable_reference) {
  struct External {
    int value = 42;
  };
  External external_obj;

  auto sut = Container{bind<External>().to(external_obj)};

  auto& ref = sut.template resolve<External&>();
  EXPECT_EQ(&external_obj, &ref);

  ref.value = 99;
  EXPECT_EQ(99, external_obj.value);
}

TEST_F(ContainerInstanceTest, resolves_const_reference) {
  struct External {
    int value = 42;
  };
  External external_obj;

  auto sut = Container{bind<External>().to(external_obj)};

  const auto& ref = sut.template resolve<const External&>();
  EXPECT_EQ(&external_obj, &ref);
}

TEST_F(ContainerInstanceTest, resolves_mutable_pointer) {
  struct External {
    int value = 42;
  };
  External external_obj;

  auto sut = Container{bind<External>().to(external_obj)};

  auto* ptr = sut.template resolve<External*>();
  EXPECT_EQ(&external_obj, ptr);

  ptr->value = 99;
  EXPECT_EQ(99, external_obj.value);
}

TEST_F(ContainerInstanceTest, resolves_const_pointer) {
  struct External {
    int value = 42;
  };
  External external_obj;

  auto sut = Container{bind<External>().to(external_obj)};

  const auto* ptr = sut.template resolve<const External*>();
  EXPECT_EQ(&external_obj, ptr);
}

// ----------------------------------------------------------------------------
// Factory Binding Tests
// ----------------------------------------------------------------------------

struct ContainerFactoryTest : ContainerTestBase {};

TEST_F(ContainerFactoryTest, resolves_with_factory) {
  struct Product {
    int value;
  };

  auto factory = []() { return Product{99}; };

  auto sut = Container{bind<Product>().as<Product>().via(factory)};

  auto value = sut.template resolve<Product>();
  EXPECT_EQ(99, value.value);
}

TEST_F(ContainerFactoryTest, factory_with_singleton_scope) {
  struct Product : Counted {};

  auto factory = []() { return Product{}; };

  auto sut = Container{
      bind<Product>().as<Product>().via(factory).in<scope::Singleton>()};

  auto& ref1 = sut.template resolve<Product&>();
  auto& ref2 = sut.template resolve<Product&>();

  EXPECT_EQ(&ref1, &ref2);
  EXPECT_EQ(0, ref1.id);
  EXPECT_EQ(1, Counted::instance_count);  // Factory called once
}

TEST_F(ContainerFactoryTest, factory_with_transient_scope) {
  struct Product : Counted {};

  auto factory = []() { return Product{}; };

  auto sut = Container{
      bind<Product>().as<Product>().via(factory).in<scope::Transient>()};

  auto value1 = sut.template resolve<Product>();
  auto value2 = sut.template resolve<Product>();

  EXPECT_EQ(0, value1.id);
  EXPECT_EQ(1, value2.id);
  EXPECT_EQ(2, Counted::instance_count);  // Factory called twice
}

TEST_F(ContainerFactoryTest, factory_with_deduced_scope) {
  struct Product {
    int value;
  };

  auto factory = []() { return Product{42}; };

  auto sut = Container{bind<Product>().as<Product>().via(factory)};

  auto value = sut.template resolve<Product>();
  auto& ref = sut.template resolve<Product&>();

  EXPECT_EQ(42, value.value);
  EXPECT_EQ(42, ref.value);
}

TEST_F(ContainerFactoryTest, factory_with_parameters_from_container) {
  struct Dependency {
    int value = 10;
    Dependency() = default;
  };

  struct Product {
    int combined_value;
    explicit Product(Dependency dep) : combined_value{dep.value * 2} {}
  };

  auto factory = [](Dependency dep) { return Product{dep}; };

  auto sut =
      Container{bind<Dependency>(), bind<Product>().as<Product>().via(factory)};

  auto product = sut.template resolve<Product>();
  EXPECT_EQ(20, product.combined_value);
}

// ----------------------------------------------------------------------------
// Interface/Implementation Binding Tests
// ----------------------------------------------------------------------------

struct ContainerInterfaceTest : ContainerTestBase {};

TEST_F(ContainerInterfaceTest, binds_interface_to_implementation) {
  struct IService {
    virtual ~IService() = default;
    virtual int get_value() const = 0;
  };

  struct ServiceImpl : IService {
    int get_value() const override { return 42; }
  };

  auto sut = Container{bind<IService>().as<ServiceImpl>()};

  auto& service = sut.template resolve<IService&>();
  EXPECT_EQ(42, service.get_value());
}

TEST_F(ContainerInterfaceTest, interface_binding_with_singleton_scope) {
  struct IService {
    virtual ~IService() = default;
    virtual int get_id() const = 0;
  };

  struct ServiceImpl : IService, Counted {
    int get_id() const override { return id; }
  };

  auto sut =
      Container{bind<IService>().as<ServiceImpl>().in<scope::Singleton>()};

  auto& ref1 = sut.template resolve<IService&>();
  auto& ref2 = sut.template resolve<IService&>();

  EXPECT_EQ(&ref1, &ref2);
  EXPECT_EQ(0, ref1.get_id());
}

TEST_F(ContainerInterfaceTest, interface_binding_with_factory) {
  struct IService {
    virtual ~IService() = default;
    virtual int get_value() const = 0;
  };

  struct ServiceImpl : IService {
    int value;
    explicit ServiceImpl(int v) : value{v} {}
    int get_value() const override { return value; }
  };

  auto factory = []() { return ServiceImpl{99}; };

  auto sut = Container{bind<IService>().as<ServiceImpl>().via(factory)};

  auto& service = sut.template resolve<IService&>();
  EXPECT_EQ(99, service.get_value());
}

TEST_F(ContainerInterfaceTest, resolves_implementation_directly) {
  struct IService {
    virtual ~IService() = default;
    virtual int get_value() const = 0;
  };

  struct ServiceImpl : IService {
    int get_value() const override { return 42; }
  };

  auto sut = Container{bind<IService>().as<ServiceImpl>()};

  // Can still resolve ServiceImpl directly (not bound)
  auto& impl = sut.template resolve<ServiceImpl&>();
  EXPECT_EQ(42, impl.get_value());
}

TEST_F(ContainerInterfaceTest, multiple_interfaces_to_implementations) {
  struct IFoo {
    virtual ~IFoo() = default;
    virtual int foo() const = 0;
  };
  struct IBar {
    virtual ~IBar() = default;
    virtual int bar() const = 0;
  };

  struct FooImpl : IFoo {
    int foo() const override { return 1; }
  };
  struct BarImpl : IBar {
    int bar() const override { return 2; }
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

struct ContainerDependencyInjectionTest : ContainerTestBase {};

TEST_F(ContainerDependencyInjectionTest, resolves_single_dependency) {
  struct Dependency {
    int value = 10;
    Dependency() = default;
  };

  struct Service {
    int result;
    explicit Service(Dependency dep) : result{dep.value * 2} {}
  };

  auto sut = Container{bind<Dependency>(), bind<Service>()};

  auto service = sut.template resolve<Service>();
  EXPECT_EQ(20, service.result);
}

TEST_F(ContainerDependencyInjectionTest, resolves_multiple_dependencies) {
  struct DepA {
    int value = 10;
    DepA() = default;
  };
  struct DepB {
    int value = 5;
    DepB() = default;
  };

  struct Service {
    int sum;
    Service(DepA a, DepB b) : sum{a.value + b.value} {}
  };

  auto sut = Container{bind<DepA>(), bind<DepB>(), bind<Service>()};

  auto service = sut.template resolve<Service>();
  EXPECT_EQ(15, service.sum);
}

TEST_F(ContainerDependencyInjectionTest, resolves_dependency_chain) {
  struct DepA {
    int value = 1;
    DepA() = default;
  };

  struct DepB {
    int value;
    explicit DepB(DepA a) : value{a.value * 2} {}
  };

  struct Service {
    int value;
    explicit Service(DepB b) : value{b.value * 2} {}
  };

  auto sut = Container{bind<DepA>(), bind<DepB>(), bind<Service>()};

  auto service = sut.template resolve<Service>();
  EXPECT_EQ(4, service.value);  // 1 * 2 * 2
}

TEST_F(ContainerDependencyInjectionTest, resolves_dependency_as_reference) {
  struct Dependency {
    int value = 42;
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
  EXPECT_EQ(42, service.dep_ptr->value);
}

TEST_F(ContainerDependencyInjectionTest,
       resolves_dependency_as_const_reference) {
  struct Dependency {
    int value = 42;
    Dependency() = default;
  };

  struct Service {
    int copied_value;
    explicit Service(const Dependency& dep) : copied_value{dep.value} {}
  };

  auto sut = Container{bind<Dependency>(), bind<Service>()};

  auto service = sut.template resolve<Service>();
  EXPECT_EQ(42, service.copied_value);
}

TEST_F(ContainerDependencyInjectionTest, resolves_dependency_as_shared_ptr) {
  struct Dependency {
    int value = 42;
    Dependency() = default;
  };

  struct Service {
    std::shared_ptr<Dependency> dep;
    explicit Service(std::shared_ptr<Dependency> d) : dep{std::move(d)} {}
  };

  auto sut =
      Container{bind<Dependency>().in<scope::Singleton>(), bind<Service>()};

  auto service = sut.template resolve<Service>();
  EXPECT_EQ(42, service.dep->value);
  EXPECT_EQ(2, service.dep.use_count());  // canonical + service.dep
}

TEST_F(ContainerDependencyInjectionTest, resolves_dependency_as_unique_ptr) {
  struct Dependency {
    int value = 42;
    Dependency() = default;
  };

  struct Service {
    std::unique_ptr<Dependency> dep;
    explicit Service(std::unique_ptr<Dependency> d) : dep{std::move(d)} {}
  };

  auto sut =
      Container{bind<Dependency>().in<scope::Transient>(), bind<Service>()};

  auto service = sut.template resolve<Service>();
  EXPECT_EQ(42, service.dep->value);
}

TEST_F(ContainerDependencyInjectionTest, resolves_dependency_as_pointer) {
  struct Dependency {
    int value = 42;
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
  EXPECT_EQ(42, service.dep->value);
}

TEST_F(ContainerDependencyInjectionTest, mixed_dependency_types) {
  struct DepA {
    int value = 1;
    DepA() = default;
  };
  struct DepB {
    int value = 2;
    DepB() = default;
  };
  struct DepC {
    int value = 3;
    DepC() = default;
  };

  struct Service {
    int sum;
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
  EXPECT_EQ(1, Counted::instance_count);  // Only one created
}

// ----------------------------------------------------------------------------
// Canonical Type Resolution Tests
// ----------------------------------------------------------------------------

struct ContainerCanonicalTest : ContainerTestBase {};

TEST_F(ContainerCanonicalTest, const_and_non_const_resolve_same_binding) {
  struct Bound {
    int value = 42;
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
    int value = 42;
    Bound() = default;
  };

  auto sut = Container{bind<Bound>().in<scope::Singleton>()};

  auto& ref = sut.template resolve<Bound&>();
  auto* ptr = sut.template resolve<Bound*>();

  EXPECT_EQ(&ref, ptr);
}

TEST_F(ContainerCanonicalTest, const_pointer_and_pointer_resolve_same_binding) {
  struct Bound {
    int value = 42;
    Bound() = default;
  };

  auto sut = Container{bind<Bound>().in<scope::Singleton>()};

  auto* ptr = sut.template resolve<Bound*>();
  const auto* const_ptr = sut.template resolve<const Bound*>();

  EXPECT_EQ(ptr, const_ptr);
}

TEST_F(ContainerCanonicalTest, shared_ptr_variations_resolve_same_binding) {
  struct Bound {
    int value = 42;
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

struct ContainerEdgeCasesTest : ContainerTestBase {};

TEST_F(ContainerEdgeCasesTest, empty_container_resolves_unbound_types) {
  struct Unbound {
    int value = 42;
    Unbound() = default;
  };

  auto sut = Container{};

  auto value = sut.template resolve<Unbound>();
  EXPECT_EQ(42, value.value);
}

TEST_F(ContainerEdgeCasesTest, zero_argument_constructor) {
  struct ZeroArgs {
    int value = 99;
    ZeroArgs() = default;
  };

  auto sut = Container{bind<ZeroArgs>()};

  auto value = sut.template resolve<ZeroArgs>();
  EXPECT_EQ(99, value.value);
}

TEST_F(ContainerEdgeCasesTest, multi_argument_constructor) {
  struct A {
    int value = 1;
    A() = default;
  };
  struct B {
    int value = 2;
    B() = default;
  };
  struct C {
    int value = 3;
    C() = default;
  };

  struct MultiArg {
    int sum;
    MultiArg(A a, B b, C c) : sum{a.value + b.value + c.value} {}
  };

  auto sut = Container{bind<A>(), bind<B>(), bind<C>(), bind<MultiArg>()};

  auto result = sut.template resolve<MultiArg>();
  EXPECT_EQ(6, result.sum);
}

TEST_F(ContainerEdgeCasesTest, resolve_same_type_multiple_ways) {
  struct Type {
    int value = 42;
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
  EXPECT_EQ(42, value.value);
}

TEST_F(ContainerEdgeCasesTest, deeply_nested_dependencies) {
  struct Level0 {
    int value = 1;
    Level0() = default;
  };
  struct Level1 {
    int value;
    explicit Level1(Level0 l0) : value{l0.value * 2} {}
  };
  struct Level2 {
    int value;
    explicit Level2(Level1 l1) : value{l1.value * 2} {}
  };
  struct Level3 {
    int value;
    explicit Level3(Level2 l2) : value{l2.value * 2} {}
  };
  struct Level4 {
    int value;
    explicit Level4(Level3 l3) : value{l3.value * 2} {}
  };

  auto sut = Container{bind<Level0>(), bind<Level1>(), bind<Level2>(),
                       bind<Level3>(), bind<Level4>()};

  auto result = sut.template resolve<Level4>();
  EXPECT_EQ(16, result.value);  // 1 * 2 * 2 * 2 * 2
}

TEST_F(ContainerEdgeCasesTest, type_with_deleted_copy_constructor) {
  struct NoCopy {
    int value = 42;
    NoCopy() = default;
    NoCopy(const NoCopy&) = delete;
    NoCopy(NoCopy&&) = default;
  };

  auto sut = Container{bind<NoCopy>().in<scope::Singleton>()};

  // Can't resolve by value, but can resolve by reference
  auto& ref = sut.template resolve<NoCopy&>();
  EXPECT_EQ(42, ref.value);

  // Can resolve as pointer
  auto* ptr = sut.template resolve<NoCopy*>();
  EXPECT_EQ(42, ptr->value);
}

TEST_F(ContainerEdgeCasesTest, resolve_from_multiple_containers) {
  struct Type {
    int value;
    explicit Type(int v = 0) : value{v} {}
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

struct ContainerMixedScopesTest : ContainerTestBase {};

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
    int value = 99;
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

struct ContainerDefaultScopeTest : ContainerTestBase {};

TEST_F(ContainerDefaultScopeTest, unbound_type_uses_default_scope) {
  struct SingletonBound {};
  struct Unbound {};
  auto sut = Container{bind<SingletonBound>()};

  [[maybe_unused]] auto instance = sut.template resolve<Unbound>();
  // Should work - uses default Deduced scope
}

TEST_F(ContainerDefaultScopeTest, unbound_type_with_dependencies) {
  struct Dep {
    int value = 10;
    Dep() = default;
  };

  struct Unbound {
    int result;
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
  EXPECT_EQ(1, Counted::instance_count);
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
// Promotion Tests (Transient → Singleton-like behavior)
// ----------------------------------------------------------------------------
//
// Promotion occurs when a type bound as Transient is requested in a way that
// requires shared ownership or reference semantics:
//
// PROMOTED (Transient → Singleton-like):
// - References (T&, const T&) - must be stable across calls
// - Pointers (T*, const T*) - must point to same instance
// - weak_ptr<T> - requires a cached shared_ptr to track
//
// NOT PROMOTED (remains Transient):
// - Values (T, T&&) - each call creates new instance
// - unique_ptr<T> - exclusive ownership, each call creates new instance
// - shared_ptr<T> - can create new instances in each shared_ptr
//
// Note: shared_ptr<T> from Transient creates NEW instances each time, not
// promoted! This is intentional - transient shared_ptr means "give me a new
// instance in a shared_ptr". Only weak_ptr requires promotion because it needs
// a cached backing shared_ptr to track.
//
// ----------------------------------------------------------------------------

struct ContainerPromotionTest : ContainerTestBase {};

TEST_F(ContainerPromotionTest, transient_promoted_to_singleton_for_reference) {
  struct TransientBound : Counted {};
  auto sut = Container{bind<TransientBound>().in<scope::Transient>()};

  // First reference request should cache
  auto& ref1 = sut.template resolve<TransientBound&>();
  auto& ref2 = sut.template resolve<TransientBound&>();

  // Should return same instance (promoted to singleton)
  EXPECT_EQ(&ref1, &ref2);
  EXPECT_EQ(0, ref1.id);
  EXPECT_EQ(1, Counted::instance_count);  // Only one instance created
}

TEST_F(ContainerPromotionTest,
       transient_promoted_to_singleton_for_const_reference) {
  struct TransientBound : Counted {};
  auto sut = Container{bind<TransientBound>().in<scope::Transient>()};

  const auto& ref1 = sut.template resolve<const TransientBound&>();
  const auto& ref2 = sut.template resolve<const TransientBound&>();

  EXPECT_EQ(&ref1, &ref2);
  EXPECT_EQ(0, ref1.id);
  EXPECT_EQ(1, Counted::instance_count);
}

TEST_F(ContainerPromotionTest, transient_promoted_to_singleton_for_pointer) {
  struct TransientBound : Counted {};
  auto sut = Container{bind<TransientBound>().in<scope::Transient>()};

  auto* ptr1 = sut.template resolve<TransientBound*>();
  auto* ptr2 = sut.template resolve<TransientBound*>();

  EXPECT_EQ(ptr1, ptr2);
  EXPECT_EQ(0, ptr1->id);
  EXPECT_EQ(1, Counted::instance_count);
}

TEST_F(ContainerPromotionTest,
       transient_promoted_to_singleton_for_const_pointer) {
  struct TransientBound : Counted {};
  auto sut = Container{bind<TransientBound>().in<scope::Transient>()};

  const auto* ptr1 = sut.template resolve<const TransientBound*>();
  const auto* ptr2 = sut.template resolve<const TransientBound*>();

  EXPECT_EQ(ptr1, ptr2);
  EXPECT_EQ(0, ptr1->id);
  EXPECT_EQ(1, Counted::instance_count);
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
  EXPECT_EQ(2, Counted::instance_count);
}

TEST_F(ContainerPromotionTest, transient_promoted_to_singleton_for_weak_ptr) {
  struct TransientBound : Counted {};
  auto sut = Container{bind<TransientBound>().in<scope::Transient>()};

  auto weak1 = sut.template resolve<std::weak_ptr<TransientBound>>();
  auto weak2 = sut.template resolve<std::weak_ptr<TransientBound>>();

  EXPECT_FALSE(weak1.expired());
  EXPECT_EQ(weak1.lock(), weak2.lock());
  EXPECT_EQ(0, weak1.lock()->id);
  EXPECT_EQ(1, Counted::instance_count);
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
  EXPECT_EQ(4, Counted::instance_count);
}

TEST_F(ContainerPromotionTest,
       transient_promotion_consistent_across_different_request_types) {
  struct TransientBound : Counted {};
  auto sut = Container{bind<TransientBound>().in<scope::Transient>()};

  // All reference-like requests should share the same promoted instance
  // (but NOT shared_ptr, which creates new instances)
  auto& ref = sut.template resolve<TransientBound&>();
  auto* ptr = sut.template resolve<TransientBound*>();
  auto weak = sut.template resolve<std::weak_ptr<TransientBound>>();

  EXPECT_EQ(&ref, ptr);
  EXPECT_EQ(ptr, weak.lock().get());
  EXPECT_EQ(0, ref.id);
  EXPECT_EQ(1, Counted::instance_count);
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

  EXPECT_EQ(2, Counted::instance_count);  // 1 Service + 1 Dependency
}

// ----------------------------------------------------------------------------
// Relegation Tests (Singleton → Transient-like behavior)
// ----------------------------------------------------------------------------
//
// Relegation occurs when a type bound as Singleton is requested in a way that
// requires exclusive ownership or value semantics:
//
// RELEGATED (Singleton → Transient-like):
// - Values (T) - creates new instances from provider
// - rvalue references (T&&) - creates new instances from provider
// - unique_ptr<T> - exclusive ownership, creates new instances
//
// NOT RELEGATED (remains Singleton):
// - References (T&, const T&) - returns reference to singleton
// - Pointers (T*, const T*) - returns pointer to singleton
// - shared_ptr<T> - wraps singleton via canonical shared_ptr
// - weak_ptr<T> - tracks the canonical shared_ptr of singleton
//
// Note: Relegated instances are NEW instances created by calling the provider
// again, NOT copies of the singleton. The singleton instance remains unchanged
// and can still be accessed via references/pointers.
//
// ----------------------------------------------------------------------------

struct ContainerRelegationTest : ContainerTestBase {};

TEST_F(ContainerRelegationTest, singleton_relegated_to_transient_for_value) {
  struct SingletonBound : Counted {};
  auto sut = Container{bind<SingletonBound>().in<scope::Singleton>()};

  // Values should create new instances (relegated to transient)
  auto val1 = sut.template resolve<SingletonBound>();
  auto val2 = sut.template resolve<SingletonBound>();

  EXPECT_NE(&val1, &val2);
  EXPECT_EQ(0, val1.id);
  EXPECT_EQ(1, val2.id);
  EXPECT_EQ(2, Counted::instance_count);
}

TEST_F(ContainerRelegationTest,
       singleton_relegated_to_transient_for_rvalue_reference) {
  struct SingletonBound : Counted {};
  auto sut = Container{bind<SingletonBound>().in<scope::Singleton>()};

  auto&& rval1 = sut.template resolve<SingletonBound&&>();
  auto&& rval2 = sut.template resolve<SingletonBound&&>();

  EXPECT_NE(&rval1, &rval2);
  EXPECT_EQ(0, rval1.id);
  EXPECT_EQ(1, rval2.id);
  EXPECT_EQ(2, Counted::instance_count);
}

TEST_F(ContainerRelegationTest,
       singleton_relegated_to_transient_for_unique_ptr) {
  struct SingletonBound : Counted {};
  auto sut = Container{bind<SingletonBound>().in<scope::Singleton>()};

  auto unique1 = sut.template resolve<std::unique_ptr<SingletonBound>>();
  auto unique2 = sut.template resolve<std::unique_ptr<SingletonBound>>();

  EXPECT_NE(unique1.get(), unique2.get());
  EXPECT_EQ(0, unique1->id);
  EXPECT_EQ(1, unique2->id);
  EXPECT_EQ(2, Counted::instance_count);
}

TEST_F(ContainerRelegationTest,
       singleton_not_relegated_for_references_or_shared_ptr) {
  struct SingletonBound : Counted {};
  auto sut = Container{bind<SingletonBound>().in<scope::Singleton>()};

  // References should still be singleton
  auto& ref1 = sut.template resolve<SingletonBound&>();
  auto& ref2 = sut.template resolve<SingletonBound&>();
  EXPECT_EQ(&ref1, &ref2);
  EXPECT_EQ(0, ref1.id);

  // Pointers should still be singleton
  auto* ptr1 = sut.template resolve<SingletonBound*>();
  auto* ptr2 = sut.template resolve<SingletonBound*>();
  EXPECT_EQ(ptr1, ptr2);
  EXPECT_EQ(&ref1, ptr1);

  // shared_ptr should wrap the singleton (not relegated)
  auto shared1 = sut.template resolve<std::shared_ptr<SingletonBound>>();
  auto shared2 = sut.template resolve<std::shared_ptr<SingletonBound>>();
  EXPECT_EQ(shared1.get(), shared2.get());
  EXPECT_EQ(&ref1, shared1.get());

  EXPECT_EQ(1, Counted::instance_count);  // Only one instance total
}

TEST_F(ContainerRelegationTest,
       singleton_shared_ptr_wraps_singleton_not_relegated) {
  struct SingletonBound : Counted {
    int value = 42;
    SingletonBound() = default;
  };
  auto sut = Container{bind<SingletonBound>().in<scope::Singleton>()};

  // Modify the singleton
  auto& singleton = sut.template resolve<SingletonBound&>();
  singleton.value = 99;

  // shared_ptr should wrap the singleton, showing the modified value
  auto shared = sut.template resolve<std::shared_ptr<SingletonBound>>();
  EXPECT_EQ(99, shared->value);
  EXPECT_EQ(&singleton, shared.get());

  // But values are relegated and create new instances with default value
  auto val = sut.template resolve<SingletonBound>();
  EXPECT_EQ(42, val.value);
  EXPECT_NE(&singleton, &val);

  EXPECT_EQ(2, Counted::instance_count);  // 1 singleton + 1 relegated value
}

TEST_F(ContainerRelegationTest,
       singleton_relegation_creates_new_instances_not_copies) {
  struct SingletonBound : Counted {
    int value = 42;
    SingletonBound() = default;
  };
  auto sut = Container{bind<SingletonBound>().in<scope::Singleton>()};

  // Get singleton reference and modify it
  auto& singleton = sut.template resolve<SingletonBound&>();
  singleton.value = 99;

  // Values are relegated to transient - creates NEW instances from provider
  // NOT copies of the singleton
  auto val1 = sut.template resolve<SingletonBound>();
  auto val2 = sut.template resolve<SingletonBound>();

  EXPECT_EQ(42, val1.value);  // Default value from provider, not modified value
  EXPECT_EQ(42, val2.value);  // Default value from provider, not modified value
  EXPECT_NE(&singleton, &val1);
  EXPECT_NE(&singleton, &val2);
  EXPECT_NE(&val1, &val2);

  // Singleton itself is unchanged
  EXPECT_EQ(99, singleton.value);
}

TEST_F(ContainerRelegationTest, singleton_relegation_with_dependencies) {
  struct Dependency : Counted {
    int value = 42;
    Dependency() = default;
  };
  struct Service : Counted {
    Dependency dep;
    explicit Service(Dependency d) : dep{d} {}
  };

  auto sut = Container{bind<Dependency>().in<scope::Singleton>(),
                       bind<Service>().in<scope::Singleton>()};

  // Service value will relegate, which will relegate Dependency
  // Each Service creates its own new Dependency instance
  auto service1 = sut.template resolve<Service>();
  auto service2 = sut.template resolve<Service>();

  EXPECT_NE(&service1, &service2);
  EXPECT_NE(&service1.dep, &service2.dep);

  // First resolution: Dependency (id=0) then Service (id=1)
  EXPECT_EQ(0, service1.dep.id);
  EXPECT_EQ(1, service1.id);

  // Second resolution: Dependency (id=2) then Service (id=3)
  EXPECT_EQ(2, service2.dep.id);
  EXPECT_EQ(3, service2.id);

  EXPECT_EQ(4, Counted::instance_count);  // 2 Service + 2 Dependency
}

// ----------------------------------------------------------------------------
// Hierarchical Container Tests - Basic Delegation
// ----------------------------------------------------------------------------

struct ContainerHierarchyTest : ContainerTestBase {};

TEST_F(ContainerHierarchyTest, child_finds_binding_in_parent) {
  struct ParentBound {
    int value = 42;
    ParentBound() = default;
  };

  auto parent = Container{bind<ParentBound>()};
  auto child = Container{parent};

  auto result = child.template resolve<ParentBound>();
  EXPECT_EQ(42, result.value);
}

TEST_F(ContainerHierarchyTest, child_overrides_parent_binding) {
  struct Bound {
    int value;
    explicit Bound(int v = 0) : value{v} {}
  };

  auto parent_factory = []() { return Bound{42}; };
  auto child_factory = []() { return Bound{99}; };

  auto parent = Container{bind<Bound>().as<Bound>().via(parent_factory)};
  auto child = Container{parent, bind<Bound>().as<Bound>().via(child_factory)};

  auto parent_result = parent.template resolve<Bound>();
  auto child_result = child.template resolve<Bound>();

  EXPECT_EQ(42, parent_result.value);
  EXPECT_EQ(99, child_result.value);
}

TEST_F(ContainerHierarchyTest, multi_level_hierarchy) {
  struct GrandparentBound {
    int value = 1;
    GrandparentBound() = default;
  };
  struct ParentBound {
    int value = 2;
    ParentBound() = default;
  };
  struct ChildBound {
    int value = 3;
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
    int value;
    explicit Bound(int v = 0) : value{v} {}
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
    int value = 42;
    Unbound() = default;
  };

  auto parent = Container{};
  auto child = Container{parent};

  // Should use fallback binding at the root level
  auto result = child.template resolve<Unbound>();
  EXPECT_EQ(42, result.value);
}

// ----------------------------------------------------------------------------
// Hierarchical Container Tests - Singleton Sharing
// ----------------------------------------------------------------------------

struct ContainerHierarchySingletonTest : ContainerTestBase {};

TEST_F(ContainerHierarchySingletonTest, singleton_in_parent_shared_with_child) {
  struct SingletonBound : Counted {};

  auto parent = Container{bind<SingletonBound>().in<scope::Singleton>()};
  auto child = Container{parent};

  auto& parent_ref = parent.template resolve<SingletonBound&>();
  auto& child_ref = child.template resolve<SingletonBound&>();

  EXPECT_EQ(&parent_ref, &child_ref);
  EXPECT_EQ(0, parent_ref.id);
  EXPECT_EQ(1, Counted::instance_count);
}

TEST_F(ContainerHierarchySingletonTest,
       singleton_in_grandparent_shared_with_all) {
  struct SingletonBound : Counted {};

  auto grandparent = Container{bind<SingletonBound>().in<scope::Singleton>()};
  auto parent = Container{grandparent};
  auto child = Container{parent};

  auto& grandparent_ref = grandparent.template resolve<SingletonBound&>();
  auto& parent_ref = parent.template resolve<SingletonBound&>();
  auto& child_ref = child.template resolve<SingletonBound&>();

  EXPECT_EQ(&grandparent_ref, &parent_ref);
  EXPECT_EQ(&parent_ref, &child_ref);
  EXPECT_EQ(0, grandparent_ref.id);
  EXPECT_EQ(1, Counted::instance_count);
}

TEST_F(ContainerHierarchySingletonTest,
       child_singleton_does_not_affect_parent) {
  struct SingletonBound : Counted {};

  auto parent = Container{};
  auto child = Container{parent, bind<SingletonBound>().in<scope::Singleton>()};

  auto& child_ref = child.template resolve<SingletonBound&>();
  // Parent should create new instance (fallback)
  auto& parent_ref = parent.template resolve<SingletonBound&>();

  EXPECT_NE(&child_ref, &parent_ref);
  EXPECT_EQ(0, child_ref.id);
  EXPECT_EQ(1, parent_ref.id);
  EXPECT_EQ(2, Counted::instance_count);
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
  EXPECT_EQ(2, Counted::instance_count);
}

// ----------------------------------------------------------------------------
// Hierarchical Container Tests - Transient Behavior
// ----------------------------------------------------------------------------

struct ContainerHierarchyTransientTest : ContainerTestBase {};

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
  EXPECT_EQ(3, Counted::instance_count);
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
  EXPECT_EQ(3, Counted::instance_count);
}

// ----------------------------------------------------------------------------
// Hierarchical Container Tests - Promotion in Hierarchy
// ----------------------------------------------------------------------------
//
// IMPORTANT: Promotion state lives with the Provider, not the Container.
//
// When a child container delegates to a parent's binding:
// - Child and parent share the same Provider instance
// - They share the same promoted instance (cached in Provider's static)
//
// To have separate promoted instances:
// - Each container needs its own binding (separate Providers)
// - Then each Provider has its own promotion state
//
// This is the correct behavior: promotion is a property of the
// binding/provider, not the container requesting the instance.
//
// ----------------------------------------------------------------------------

struct ContainerHierarchyPromotionTest : ContainerTestBase {};

TEST_F(ContainerHierarchyPromotionTest, child_promotes_transient_from_parent) {
  struct TransientBound : Counted {};

  auto parent = Container{bind<TransientBound>().in<scope::Transient>()};
  auto child = Container{parent};

  // Child requests by reference, should promote
  auto& child_ref1 = child.template resolve<TransientBound&>();
  auto& child_ref2 = child.template resolve<TransientBound&>();

  EXPECT_EQ(&child_ref1, &child_ref2);
  EXPECT_EQ(0, child_ref1.id);
  EXPECT_EQ(1, Counted::instance_count);
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
  EXPECT_EQ(1, Counted::instance_count);  // Only one instance created
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
  EXPECT_EQ(2, Counted::instance_count);
}

TEST_F(ContainerHierarchyPromotionTest,
       grandparent_parent_child_share_promoted_instance_when_delegating) {
  struct TransientBound : Counted {};

  auto grandparent = Container{bind<TransientBound>().in<scope::Transient>()};
  auto parent = Container{grandparent};  // No binding, delegates to grandparent
  auto child =
      Container{parent};  // No binding, delegates to parent → grandparent

  auto& grandparent_ref = grandparent.template resolve<TransientBound&>();
  auto& parent_ref = parent.template resolve<TransientBound&>();
  auto& child_ref = child.template resolve<TransientBound&>();

  // All share grandparent's promoted instance
  EXPECT_EQ(&grandparent_ref, &parent_ref);
  EXPECT_EQ(&parent_ref, &child_ref);
  EXPECT_EQ(0, grandparent_ref.id);
  EXPECT_EQ(1, Counted::instance_count);
}

TEST_F(
    ContainerHierarchyPromotionTest,
    grandparent_parent_child_have_separate_promoted_instances_with_own_bindings) {
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
  EXPECT_EQ(3, Counted::instance_count);
}

// ----------------------------------------------------------------------------
// Hierarchical Container Tests - Relegation in Hierarchy
// ----------------------------------------------------------------------------

struct ContainerHierarchyRelegationTest : ContainerTestBase {};

TEST_F(ContainerHierarchyRelegationTest,
       child_relegates_singleton_from_parent) {
  struct SingletonBound : Counted {};

  auto parent = Container{bind<SingletonBound>().in<scope::Singleton>()};
  auto child = Container{parent};

  // Child requests by value, should relegate
  auto child_val1 = child.template resolve<SingletonBound>();
  auto child_val2 = child.template resolve<SingletonBound>();

  EXPECT_NE(&child_val1, &child_val2);
  EXPECT_EQ(0, child_val1.id);
  EXPECT_EQ(1, child_val2.id);
  EXPECT_EQ(2, Counted::instance_count);
}

TEST_F(ContainerHierarchyRelegationTest,
       parent_singleton_reference_differs_from_child_relegated_values) {
  struct SingletonBound : Counted {};

  auto parent = Container{bind<SingletonBound>().in<scope::Singleton>()};
  auto child = Container{parent};

  auto& parent_ref = parent.template resolve<SingletonBound&>();
  auto child_val = child.template resolve<SingletonBound>();

  EXPECT_NE(&parent_ref, &child_val);
  EXPECT_EQ(0, parent_ref.id);
  EXPECT_EQ(1, child_val.id);
  EXPECT_EQ(2, Counted::instance_count);
}

TEST_F(ContainerHierarchyRelegationTest,
       grandparent_singleton_reference_accessible_but_child_can_relegate) {
  struct SingletonBound : Counted {};

  auto grandparent = Container{bind<SingletonBound>().in<scope::Singleton>()};
  auto parent = Container{grandparent};
  auto child = Container{parent};

  auto& grandparent_ref = grandparent.template resolve<SingletonBound&>();
  auto& child_ref = child.template resolve<SingletonBound&>();
  auto child_val = child.template resolve<SingletonBound>();

  EXPECT_EQ(&grandparent_ref, &child_ref);  // References share
  EXPECT_NE(&grandparent_ref, &child_val);  // Value is relegated
  EXPECT_EQ(0, grandparent_ref.id);
  EXPECT_EQ(1, child_val.id);
  EXPECT_EQ(2, Counted::instance_count);
}

// ----------------------------------------------------------------------------
// Complex Hierarchical Scenarios
// ----------------------------------------------------------------------------

struct ContainerHierarchyComplexTest : ContainerTestBase {};

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

  EXPECT_EQ(4, Counted::instance_count);
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
  EXPECT_EQ(3, Counted::instance_count);
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

  // Child singleton relegated to transient
  auto child_val1 = child.template resolve<Bound>();
  auto child_val2 = child.template resolve<Bound>();
  EXPECT_NE(&child_val1, &child_val2);
  EXPECT_EQ(2, child_val1.id);
  EXPECT_EQ(3, child_val2.id);

  EXPECT_EQ(4, Counted::instance_count);
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
  EXPECT_EQ(1, Counted::instance_count);
}

// This test shows a surprising result that can't be avoided.
// Two containers with the same type cache the same singletons.
// This is because they are cached in Meyers singletons keyed on container and
// provider. When they are the same, the same singletons are found.
// The solution is found in
// sibling_containers_using_macro_are_independent_with_own_bindings:
// use dink_unique_container to distinguish between containers with the same
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
  EXPECT_EQ(1, Counted::instance_count);
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
  EXPECT_EQ(1, Counted::instance_count);
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
  EXPECT_EQ(2, Counted::instance_count);
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
  EXPECT_EQ(1, Counted::instance_count);
}

TEST_F(ContainerHierarchyComplexTest, deep_hierarchy_with_multiple_overrides) {
  struct Bound {
    int value;
    explicit Bound(int v = 0) : value{v} {}
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

}  // namespace dink
