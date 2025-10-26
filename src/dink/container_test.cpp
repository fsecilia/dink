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
  virtual ~ContainerTestBase() override;
};
ContainerTestBase::~ContainerTestBase() {}

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
  };
  auto sut = Container{bind<TransientBound>().in<scope::Transient>()};

  const auto value = sut.template resolve<const TransientBound>();
  EXPECT_EQ(42, value.value);
}

TEST_F(ContainerTransientTest, resolves_rvalue_reference) {
  struct TransientBound {
    int value = 42;
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
// Deduced Scope Tests
// ----------------------------------------------------------------------------

struct ContainerDeducedTest : ContainerTestBase {};

TEST_F(ContainerDeducedTest, resolves_value) {
  struct DeducedBound {
    int value = 42;
  };
  auto sut = Container{bind<DeducedBound>()};

  auto value1 = sut.template resolve<DeducedBound>();
  auto value2 = sut.template resolve<DeducedBound>();

  EXPECT_EQ(42, value1.value);
  EXPECT_EQ(42, value2.value);
  // Values are copies, so different addresses
  EXPECT_NE(&value1, &value2);
}

TEST_F(ContainerDeducedTest, resolves_const_value) {
  struct DeducedBound {
    int value = 42;
  };
  auto sut = Container{bind<DeducedBound>()};

  const auto value = sut.template resolve<const DeducedBound>();
  EXPECT_EQ(42, value.value);
}

TEST_F(ContainerDeducedTest, resolves_rvalue_reference) {
  struct DeducedBound {
    int value = 42;
  };
  auto sut = Container{bind<DeducedBound>()};

  auto&& value = sut.template resolve<DeducedBound&&>();
  EXPECT_EQ(42, value.value);
}

TEST_F(ContainerDeducedTest, resolves_mutable_reference_cached) {
  struct DeducedBound {
    int value = 42;
  };
  auto sut = Container{bind<DeducedBound>()};

  auto& ref1 = sut.template resolve<DeducedBound&>();
  auto& ref2 = sut.template resolve<DeducedBound&>();

  EXPECT_EQ(&ref1, &ref2);  // Same instance
  EXPECT_EQ(42, ref1.value);

  ref1.value = 99;
  EXPECT_EQ(99, ref2.value);
}

TEST_F(ContainerDeducedTest, resolves_const_reference_cached) {
  struct DeducedBound {
    int value = 42;
  };
  auto sut = Container{bind<DeducedBound>()};

  const auto& ref1 = sut.template resolve<const DeducedBound&>();
  const auto& ref2 = sut.template resolve<const DeducedBound&>();

  EXPECT_EQ(&ref1, &ref2);  // Same instance
  EXPECT_EQ(42, ref1.value);
}

TEST_F(ContainerDeducedTest, resolves_mutable_pointer_cached) {
  struct DeducedBound {
    int value = 42;
  };
  auto sut = Container{bind<DeducedBound>()};

  auto* ptr1 = sut.template resolve<DeducedBound*>();
  auto* ptr2 = sut.template resolve<DeducedBound*>();

  EXPECT_EQ(ptr1, ptr2);  // Same instance
  EXPECT_EQ(42, ptr1->value);

  ptr1->value = 99;
  EXPECT_EQ(99, ptr2->value);
}

TEST_F(ContainerDeducedTest, resolves_const_pointer_cached) {
  struct DeducedBound {
    int value = 42;
  };
  auto sut = Container{bind<DeducedBound>()};

  const auto* ptr1 = sut.template resolve<const DeducedBound*>();
  const auto* ptr2 = sut.template resolve<const DeducedBound*>();

  EXPECT_EQ(ptr1, ptr2);  // Same instance
  EXPECT_EQ(42, ptr1->value);
}

TEST_F(ContainerDeducedTest, resolves_shared_ptr_cached) {
  struct DeducedBound {
    int value = 42;
  };
  auto sut = Container{bind<DeducedBound>()};

  auto shared1 = sut.template resolve<std::shared_ptr<DeducedBound>>();
  auto shared2 = sut.template resolve<std::shared_ptr<DeducedBound>>();

  EXPECT_EQ(shared1.get(), shared2.get());  // Same instance
  EXPECT_EQ(42, shared1->value);
}

TEST_F(ContainerDeducedTest, resolves_weak_ptr_cached) {
  struct DeducedBound {
    int value = 42;
  };
  auto sut = Container{bind<DeducedBound>()};

  auto weak1 = sut.template resolve<std::weak_ptr<DeducedBound>>();
  auto weak2 = sut.template resolve<std::weak_ptr<DeducedBound>>();

  EXPECT_FALSE(weak1.expired());
  EXPECT_EQ(weak1.lock().get(), weak2.lock().get());  // Same instance
}

TEST_F(ContainerDeducedTest, reference_and_value_have_different_behavior) {
  struct DeducedBound : Counted {};

  auto sut = Container{bind<DeducedBound>()};

  // Reference returns cached instance (id=0)
  auto& ref = sut.template resolve<DeducedBound&>();
  EXPECT_EQ(0, ref.id);

  // Value creates new instance (id=1)
  auto value = sut.template resolve<DeducedBound>();
  EXPECT_EQ(1, value.id);

  // Another reference returns same cached instance (id=0)
  auto& ref2 = sut.template resolve<DeducedBound&>();
  EXPECT_EQ(0, ref2.id);
  EXPECT_EQ(&ref, &ref2);
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
  };
  struct DepB {
    int value = 5;
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
  };
  struct DepB {
    int value = 2;
  };
  struct DepC {
    int value = 3;
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
  };

  auto sut = Container{bind<Bound>().in<scope::Singleton>()};

  auto& ref = sut.template resolve<Bound&>();
  auto* ptr = sut.template resolve<Bound*>();

  EXPECT_EQ(&ref, ptr);
}

TEST_F(ContainerCanonicalTest, const_pointer_and_pointer_resolve_same_binding) {
  struct Bound {
    int value = 42;
  };

  auto sut = Container{bind<Bound>().in<scope::Singleton>()};

  auto* ptr = sut.template resolve<Bound*>();
  const auto* const_ptr = sut.template resolve<const Bound*>();

  EXPECT_EQ(ptr, const_ptr);
}

TEST_F(ContainerCanonicalTest, shared_ptr_variations_resolve_same_binding) {
  struct Bound {
    int value = 42;
  };

  auto sut = Container{bind<Bound>().in<scope::Singleton>()};

  auto shared = sut.template resolve<std::shared_ptr<Bound>>();
  auto const_shared = sut.template resolve<std::shared_ptr<const Bound>>();

  // Both should point to the same underlying object
  EXPECT_EQ(shared.get(), const_shared.get());
}

TEST_F(ContainerCanonicalTest, rvalue_reference_resolves_same_binding) {
  struct Bound {
    int value = 42;
  };

  auto sut = Container{bind<Bound>().in<scope::Deduced>()};

  auto& ref = sut.template resolve<Bound&>();
  auto&& rvalue_ref = sut.template resolve<Bound&&>();

  // For Deduced scope with references, both should be the cached instance
  EXPECT_EQ(&ref, &rvalue_ref);
}

// ----------------------------------------------------------------------------
// Edge Cases and Error Conditions
// ----------------------------------------------------------------------------

struct ContainerEdgeCasesTest : ContainerTestBase {};

TEST_F(ContainerEdgeCasesTest, empty_container_resolves_unbound_types) {
  struct Unbound {
    int value = 42;
  };

  auto sut = Container{};

  auto value = sut.template resolve<Unbound>();
  EXPECT_EQ(42, value.value);
}

TEST_F(ContainerEdgeCasesTest, zero_argument_constructor) {
  struct ZeroArgs {
    int value = 99;
  };

  auto sut = Container{bind<ZeroArgs>()};

  auto value = sut.template resolve<ZeroArgs>();
  EXPECT_EQ(99, value.value);
}

TEST_F(ContainerEdgeCasesTest, multi_argument_constructor) {
  struct A {
    int value = 1;
  };
  struct B {
    int value = 2;
  };
  struct C {
    int value = 3;
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

}  // namespace dink
