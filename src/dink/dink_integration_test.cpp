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

// Common base for all container tests.
struct ContainerTest : Test {
  static inline const auto initial_value = int_t{7793};   // arbitrary
  static inline const auto modified_value = int_t{2145};  // arbitrary

  // Base class for types that need instance counting.
  struct Counted {
    static inline int_t num_instances = 0;
    int_t id;
    Counted() : id{num_instances++} {}
  };

  // Arbitrary type with a known initial value.
  struct Initialized : Counted {
    int_t value = initial_value;
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

  ContainerTest() { Counted::num_instances = 0; }
};

// ----------------------------------------------------------------------------
// Singleton Scope Tests
// ----------------------------------------------------------------------------
// These tests require unique types per test, or cached instances will leak
// between them.

struct ContainerSingletonTest : ContainerTest {};

TEST_F(ContainerSingletonTest, canonical_shared_wraps_instance) {
  struct Type : Singleton {};
  auto sut = Container{bind<Type>().in<scope::Singleton>()};

  const auto shared = sut.template resolve<std::shared_ptr<Type>>();
  auto& instance = sut.template resolve<Type&>();
  ASSERT_EQ(&instance, shared.get());
}

TEST_F(ContainerSingletonTest, canonical_shared_ptr_value) {
  struct Type : Singleton {};
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
  struct Type : Singleton {};
  auto sut = Container{bind<Type>().in<scope::Singleton>()};

  const auto& result1 = sut.template resolve<std::shared_ptr<Type>&>();
  const auto& result2 = sut.template resolve<std::shared_ptr<Type>&>();
  ASSERT_EQ(&result1, &result2);
  ASSERT_EQ(result1.use_count(), result2.use_count());
  ASSERT_EQ(result1.use_count(), 1);
}

TEST_F(ContainerSingletonTest, weak_ptr_from_singleton) {
  struct Type : Singleton {};
  auto sut = Container{bind<Type>().in<scope::Singleton>()};

  auto weak1 = sut.template resolve<std::weak_ptr<Type>>();
  auto weak2 = sut.template resolve<std::weak_ptr<Type>>();

  EXPECT_FALSE(weak1.expired());
  EXPECT_EQ(weak1.lock(), weak2.lock());
}

TEST_F(ContainerSingletonTest, weak_ptr_does_not_expire_while_singleton_alive) {
  struct Type : Singleton {};
  auto sut = Container{bind<Type>().in<scope::Singleton>()};

  const auto& weak = sut.template resolve<std::weak_ptr<Type>>();

  // Even with no shared_ptr in scope, weak_ptr should not expire
  // because it tracks the canonical shared_ptr which aliases the instance.
  EXPECT_FALSE(weak.expired());

  auto shared = weak.lock();
  EXPECT_NE(nullptr, shared);
}

TEST_F(ContainerSingletonTest, weak_ptr_expires_with_canonical_shared_ptr) {
  struct Type : Singleton {};
  auto sut = Container{bind<Type>().in<scope::Singleton>()};

  // Resolve reference directly to canonical shared_ptr.
  auto& canonical_shared_ptr = sut.template resolve<std::shared_ptr<Type>&>();
  const auto weak = sut.template resolve<std::weak_ptr<Type>>();

  EXPECT_FALSE(weak.expired());
  canonical_shared_ptr.reset();
  EXPECT_TRUE(weak.expired());
}

TEST_F(ContainerSingletonTest, const_shared_ptr) {
  struct Type : Singleton {};
  auto sut = Container{bind<Type>().in<scope::Singleton>()};

  auto shared = sut.template resolve<std::shared_ptr<const Type>>();
  auto& instance = sut.template resolve<Type&>();

  EXPECT_EQ(&instance, shared.get());
}

TEST_F(ContainerSingletonTest, multiple_singleton_types) {
  struct Type1 : Singleton {};
  struct Type2 : Singleton {};

  auto sut = Container{bind<Type1>().in<scope::Singleton>(),
                       bind<Type2>().in<scope::Singleton>()};

  auto shared_1 = sut.template resolve<std::shared_ptr<Type1>>();
  auto shared_2 = sut.template resolve<std::shared_ptr<Type2>>();

  EXPECT_NE(shared_1.get(), nullptr);
  EXPECT_NE(shared_2.get(), nullptr);
}

TEST_F(ContainerSingletonTest, resolves_mutable_reference) {
  struct Type : Singleton {};
  auto sut = Container{bind<Type>().in<scope::Singleton>()};

  auto& ref1 = sut.template resolve<Type&>();
  auto& ref2 = sut.template resolve<Type&>();

  EXPECT_EQ(&ref1, &ref2);
  EXPECT_EQ(initial_value, ref1.value);

  ref1.value = modified_value;
  EXPECT_EQ(modified_value, ref2.value);
}

TEST_F(ContainerSingletonTest, resolves_const_reference) {
  struct Type : Singleton {};
  auto sut = Container{bind<Type>().in<scope::Singleton>()};

  const auto& ref1 = sut.template resolve<const Type&>();
  const auto& ref2 = sut.template resolve<const Type&>();

  EXPECT_EQ(&ref1, &ref2);
  EXPECT_EQ(initial_value, ref1.value);
}

TEST_F(ContainerSingletonTest, resolves_mutable_pointer) {
  struct Type : Singleton {};
  auto sut = Container{bind<Type>().in<scope::Singleton>()};

  auto* ptr1 = sut.template resolve<Type*>();
  auto* ptr2 = sut.template resolve<Type*>();

  EXPECT_EQ(ptr1, ptr2);
  EXPECT_EQ(initial_value, ptr1->value);

  ptr1->value = modified_value;
  EXPECT_EQ(modified_value, ptr2->value);
}

TEST_F(ContainerSingletonTest, resolves_const_pointer) {
  struct Type : Singleton {};
  auto sut = Container{bind<Type>().in<scope::Singleton>()};

  const auto* ptr1 = sut.template resolve<const Type*>();
  const auto* ptr2 = sut.template resolve<const Type*>();

  EXPECT_EQ(ptr1, ptr2);
  EXPECT_EQ(initial_value, ptr1->value);
}

TEST_F(ContainerSingletonTest, reference_and_pointer_point_to_same_instance) {
  struct Type : Singleton {};
  auto sut = Container{bind<Type>().in<scope::Singleton>()};

  auto& ref = sut.template resolve<Type&>();
  auto* ptr = sut.template resolve<Type*>();

  EXPECT_EQ(&ref, ptr);
}

// ----------------------------------------------------------------------------
// Transient Scope Tests
// ----------------------------------------------------------------------------

struct ContainerTransientTest : ContainerTest {};

TEST_F(ContainerTransientTest, creates_new_shared_ptr_per_resolve) {
  auto sut = Container{bind<Initialized>().in<scope::Transient>()};

  auto shared1 = sut.template resolve<std::shared_ptr<Initialized>>();
  auto shared2 = sut.template resolve<std::shared_ptr<Initialized>>();

  EXPECT_NE(shared1.get(), shared2.get());  // Different instances
}

TEST_F(ContainerTransientTest, creates_new_value_per_resolve) {
  auto sut = Container{bind<Initialized>().in<scope::Transient>()};

  auto value1 = sut.template resolve<Initialized>();
  auto value2 = sut.template resolve<Initialized>();

  EXPECT_EQ(0, value1.id);
  EXPECT_EQ(1, value2.id);
}

TEST_F(ContainerTransientTest, creates_new_unique_ptr_per_resolve) {
  auto sut = Container{bind<Initialized>().in<scope::Transient>()};

  auto unique1 = sut.template resolve<std::unique_ptr<Initialized>>();
  auto unique2 = sut.template resolve<std::unique_ptr<Initialized>>();

  EXPECT_NE(unique1.get(), unique2.get());
  EXPECT_EQ(initial_value, unique1->value);
  EXPECT_EQ(initial_value, unique2->value);
}

TEST_F(ContainerTransientTest, resolves_const_value) {
  auto sut = Container{bind<Initialized>().in<scope::Transient>()};

  const auto value = sut.template resolve<const Initialized>();
  EXPECT_EQ(initial_value, value.value);
}

TEST_F(ContainerTransientTest, resolves_rvalue_reference) {
  auto sut = Container{bind<Initialized>().in<scope::Transient>()};

  auto&& value = sut.template resolve<Initialized&&>();
  EXPECT_EQ(initial_value, value.value);
}

// ----------------------------------------------------------------------------
// Instance Scope Tests
// ----------------------------------------------------------------------------

struct ContainerInstanceTest : ContainerTest {
  using Instance = Initialized;
  Instance external;

  using Sut = Container<
      Config<Binding<Instance, scope::Instance, provider::External<Instance>>>>;
  Sut sut = Container{bind<Instance>().to(external)};
};

TEST_F(ContainerInstanceTest, shared_ptr_wraps_external_instance) {
  // shared_ptr should wrap the external instance.
  auto shared1 = sut.template resolve<std::shared_ptr<Instance>>();
  auto shared2 = sut.template resolve<std::shared_ptr<Instance>>();
  auto& ref = sut.template resolve<Instance&>();

  EXPECT_EQ(&external, shared1.get());      // Points to external
  EXPECT_EQ(shared1.get(), shared2.get());  // Same cached shared_ptr
  EXPECT_EQ(3, shared1.use_count());        // canonical + shared1 + shared2

  // Reference and pointer refer to same instance.
  EXPECT_EQ(&external, &ref);
  EXPECT_EQ(&ref, shared1.get());
}

TEST_F(ContainerInstanceTest, canonical_shared_ptr_reference) {
  auto& canonical1 = sut.template resolve<std::shared_ptr<Instance>&>();
  auto& canonical2 = sut.template resolve<std::shared_ptr<Instance>&>();

  EXPECT_EQ(&external, canonical1.get());  // Wraps external
  EXPECT_EQ(&canonical1, &canonical2);     // Same cached shared_ptr
  EXPECT_EQ(1, canonical1.use_count());    // Only canonical exists
}

TEST_F(ContainerInstanceTest, weak_ptr_tracks_external_instance) {
  auto weak = sut.template resolve<std::weak_ptr<Instance>>();

  EXPECT_FALSE(weak.expired());
  EXPECT_EQ(&external, weak.lock().get());
}

TEST_F(ContainerInstanceTest, weak_ptr_does_not_expire_while_instance_alive) {
  auto weak = sut.template resolve<std::weak_ptr<Instance>>();

  // Even with no shared_ptr in scope, weak_ptr should not expire because it
  // tracks the canonical shared_ptr, which aliases the static.
  EXPECT_FALSE(weak.expired());

  auto shared = weak.lock();
  EXPECT_NE(nullptr, shared);
}

TEST_F(ContainerInstanceTest, weak_ptr_expires_with_canonical_shared_ptr) {
  // Resolve reference directly to canonical shared_ptr.
  auto& canonical_shared_ptr =
      sut.template resolve<std::shared_ptr<Instance>&>();
  const auto weak = sut.template resolve<std::weak_ptr<Instance>>();

  EXPECT_FALSE(weak.expired());
  canonical_shared_ptr.reset();
  EXPECT_TRUE(weak.expired());
}

TEST_F(ContainerInstanceTest, resolves_value_copy_of_external) {
  external.value = modified_value;

  auto copy = sut.template resolve<Instance>();
  EXPECT_EQ(modified_value, copy.value);

  // Verify it's a copy, not the original.
  copy.value *= 2;
  EXPECT_EQ(modified_value, external.value);
}

TEST_F(ContainerInstanceTest, resolves_mutable_reference) {
  auto& ref = sut.template resolve<Instance&>();
  EXPECT_EQ(&external, &ref);

  ref.value = modified_value;
  EXPECT_EQ(modified_value, external.value);
}

TEST_F(ContainerInstanceTest, resolves_const_reference) {
  auto sut = Container{bind<Instance>().to(external)};

  const auto& ref = sut.template resolve<const Instance&>();
  EXPECT_EQ(&external, &ref);
}

TEST_F(ContainerInstanceTest, resolves_mutable_pointer) {
  auto sut = Container{bind<Instance>().to(external)};

  auto* ptr = sut.template resolve<Instance*>();
  EXPECT_EQ(&external, ptr);

  ptr->value = modified_value;
  EXPECT_EQ(modified_value, external.value);
}

TEST_F(ContainerInstanceTest, resolves_const_pointer) {
  auto sut = Container{bind<Instance>().to(external)};

  const auto* ptr = sut.template resolve<const Instance*>();
  EXPECT_EQ(&external, ptr);
}

// ----------------------------------------------------------------------------
// Factory Binding Tests
// ----------------------------------------------------------------------------

struct ContainerFactoryTest : ContainerTest {
  struct Factory {
    auto operator()() const -> Product { return Product{initial_value}; }
  };
  Factory factory;
};

TEST_F(ContainerFactoryTest, resolves_with_factory) {
  auto sut = Container{bind<Product>().via(factory)};

  auto value = sut.template resolve<Product>();
  EXPECT_EQ(initial_value, value.value);
}

TEST_F(ContainerFactoryTest, factory_with_singleton_scope) {
  auto sut = Container{bind<Product>().via(factory).in<scope::Singleton>()};

  auto& ref1 = sut.template resolve<Product&>();
  auto& ref2 = sut.template resolve<Product&>();

  EXPECT_EQ(&ref1, &ref2);
  EXPECT_EQ(0, ref1.id);
  EXPECT_EQ(1, Counted::num_instances);
}

TEST_F(ContainerFactoryTest, factory_with_transient_scope) {
  auto sut = Container{bind<Product>().via(factory).in<scope::Transient>()};

  auto value1 = sut.template resolve<Product>();
  auto value2 = sut.template resolve<Product>();

  EXPECT_EQ(0, value1.id);
  EXPECT_EQ(1, value2.id);
  EXPECT_EQ(2, Counted::num_instances);
}

TEST_F(ContainerFactoryTest, factory_with_default_transient_scope) {
  auto sut = Container{bind<Product>().via(factory)};

  auto value = sut.template resolve<Product>();
  auto& ref = sut.template resolve<Product&>();

  EXPECT_EQ(initial_value, value.value);
  EXPECT_EQ(initial_value, ref.value);
  EXPECT_NE(value.id, ref.id);
  EXPECT_EQ(0, value.id);
  EXPECT_EQ(1, ref.id);
  EXPECT_EQ(2, Counted::num_instances);
}

TEST_F(ContainerFactoryTest, factory_with_parameters_from_container) {
  struct ProductWithDep {
    int_t combined_value;
    explicit ProductWithDep(Dependency dep) : combined_value{dep.value * 2} {}
  };

  auto factory = [](Dependency dep) { return ProductWithDep{dep}; };

  auto sut = Container{bind<Dependency>(), bind<ProductWithDep>().via(factory)};

  auto product = sut.template resolve<ProductWithDep>();
  EXPECT_EQ(initial_value * 2, product.combined_value);
}

// ----------------------------------------------------------------------------
// Interface/Implementation Binding Tests
// ----------------------------------------------------------------------------

struct ContainerInterfaceTest : ContainerTest {};

TEST_F(ContainerInterfaceTest, binds_interface_to_implementation) {
  struct Service : IService {
    int_t get_value() const override { return initial_value; }
  };

  auto sut = Container{bind<IService>().as<Service>()};

  auto& service = sut.template resolve<IService&>();
  EXPECT_EQ(initial_value, service.get_value());
}

TEST_F(ContainerInterfaceTest, interface_binding_with_singleton_scope) {
  struct Service : IService, Counted {
    int_t get_value() const override { return id; }
  };

  auto sut = Container{bind<IService>().as<Service>().in<scope::Singleton>()};

  auto& ref1 = sut.template resolve<IService&>();
  auto& ref2 = sut.template resolve<IService&>();

  EXPECT_EQ(&ref1, &ref2);
  EXPECT_EQ(0, ref1.get_value());
}

TEST_F(ContainerInterfaceTest, interface_binding_with_factory) {
  struct Service : IService {
    int_t value;
    explicit Service(int_t value) : value{value} {}
    int_t get_value() const override { return value; }
  };

  auto factory = []() { return Service{modified_value}; };

  auto sut = Container{bind<IService>().as<Service>().via(factory)};

  auto& service = sut.template resolve<IService&>();
  EXPECT_EQ(modified_value, service.get_value());
}

TEST_F(ContainerInterfaceTest, resolves_implementation_directly) {
  struct Service : IService {
    int_t get_value() const override { return initial_value; }
  };

  auto sut = Container{bind<IService>().as<Service>()};

  // Can still resolve Service directly.
  auto& impl = sut.template resolve<Service&>();
  EXPECT_EQ(initial_value, impl.get_value());
}

TEST_F(ContainerInterfaceTest, multiple_interfaces_to_implementations) {
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
// Multiple Inheritance Tests
// ----------------------------------------------------------------------------
// Caching is keyed on the To type, not the From type, so multiple interfaces
// to the same type will return the same instance.

struct ContainerMultipleInheritanceTest : ContainerTest {
  struct IService2 {
    virtual ~IService2() = default;
    virtual int_t get_value2() const = 0;
  };

  struct Service : IService, IService2 {
    int_t get_value() const override { return 1; }
    int_t get_value2() const override { return 2; }
  };
};

TEST_F(ContainerMultipleInheritanceTest, same_impl_same_instance_singleton) {
  auto sut = Container{bind<IService>().as<Service>().in<scope::Singleton>(),
                       bind<IService2>().as<Service>().in<scope::Singleton>()};

  auto& service1 = sut.template resolve<IService&>();
  auto& service2 = sut.template resolve<IService2&>();

  ASSERT_EQ(dynamic_cast<Service*>(&service1),
            dynamic_cast<Service*>(&service2));

  EXPECT_EQ(1, service1.get_value());
  EXPECT_EQ(2, service2.get_value2());
}

TEST_F(ContainerMultipleInheritanceTest,
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

TEST_F(ContainerMultipleInheritanceTest,
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

// ----------------------------------------------------------------------------
// Dependency Injection Tests
// ----------------------------------------------------------------------------

struct ContainerDependencyInjectionTest : ContainerTest {};

TEST_F(ContainerDependencyInjectionTest, resolves_single_dependency) {
  struct Service {
    int_t result;
    explicit Service(Dependency dep) : result{dep.value * 2} {}
  };

  auto sut = Container{bind<Dependency>(), bind<Service>()};

  auto service = sut.template resolve<Service>();
  EXPECT_EQ(initial_value * 2, service.result);
}

TEST_F(ContainerDependencyInjectionTest, resolves_multiple_dependencies) {
  struct Service {
    int_t sum;
    Service(Dep1 d1, Dep2 d2) : sum{d1.value + d2.value} {}
  };

  auto sut = Container{bind<Dep1>(), bind<Dep2>(), bind<Service>()};

  auto service = sut.template resolve<Service>();
  EXPECT_EQ(3, service.sum);  // 1 + 2
}

TEST_F(ContainerDependencyInjectionTest, resolves_dependency_chain) {
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

TEST_F(ContainerDependencyInjectionTest, resolves_dependency_as_reference) {
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
  struct Service {
    int_t copied_value;
    explicit Service(const Dependency& dep) : copied_value{dep.value} {}
  };

  auto sut = Container{bind<Dependency>(), bind<Service>()};

  auto service = sut.template resolve<Service>();
  EXPECT_EQ(initial_value, service.copied_value);
}

TEST_F(ContainerDependencyInjectionTest, resolves_dependency_as_shared_ptr) {
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

TEST_F(ContainerDependencyInjectionTest,
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

TEST_F(ContainerDependencyInjectionTest,
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
  EXPECT_EQ(initial_value + initial_value, service.sum);
}

// ----------------------------------------------------------------------------
// Canonical Type Resolution Tests
// ----------------------------------------------------------------------------

struct ContainerCanonicalTest : ContainerTest {};

TEST_F(ContainerCanonicalTest, const_and_non_const_resolve_same_binding) {
  struct Type : Singleton {};

  auto sut = Container{bind<Type>().in<scope::Singleton>()};

  auto& ref = sut.template resolve<Type&>();
  const auto& const_ref = sut.template resolve<const Type&>();

  EXPECT_EQ(&ref, &const_ref);
}

TEST_F(ContainerCanonicalTest, reference_and_value_resolve_same_binding) {
  struct Type : Singleton {};

  auto sut = Container{bind<Type>().in<scope::Singleton>()};

  auto& ref = sut.template resolve<Type&>();
  auto value = sut.template resolve<Type>();

  EXPECT_EQ(0, ref.id);
  EXPECT_EQ(value.id, ref.id);
}

TEST_F(ContainerCanonicalTest, pointer_and_reference_resolve_same_binding) {
  struct Type : Singleton {};

  auto sut = Container{bind<Type>().in<scope::Singleton>()};

  auto& ref = sut.template resolve<Type&>();
  auto* ptr = sut.template resolve<Type*>();

  EXPECT_EQ(&ref, ptr);
}

TEST_F(ContainerCanonicalTest, const_pointer_and_pointer_resolve_same_binding) {
  struct Type : Singleton {};

  auto sut = Container{bind<Type>().in<scope::Singleton>()};

  auto* ptr = sut.template resolve<Type*>();
  const auto* const_ptr = sut.template resolve<const Type*>();

  EXPECT_EQ(ptr, const_ptr);
}

TEST_F(ContainerCanonicalTest, shared_ptr_variations_resolve_same_binding) {
  struct Type : Singleton {};

  auto sut = Container{bind<Type>().in<scope::Singleton>()};

  auto shared = sut.template resolve<std::shared_ptr<Type>>();
  auto const_shared = sut.template resolve<std::shared_ptr<const Type>>();

  // Both should point to the same underlying object
  EXPECT_EQ(shared.get(), const_shared.get());
}

// ----------------------------------------------------------------------------
// Edge Cases and Error Conditions
// ----------------------------------------------------------------------------

struct ContainerEdgeCasesTest : ContainerTest {};

TEST_F(ContainerEdgeCasesTest, empty_container_resolves_unbound_types) {
  using Type = Initialized;

  auto sut = Container{};

  auto value = sut.template resolve<Type>();
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
  struct MultiArg {
    int_t sum;
    MultiArg(Dep1 d1, Dep2 d2, Dep3 d3) : sum{d1.value + d2.value + d3.value} {}
  };

  auto sut =
      Container{bind<Dep1>(), bind<Dep2>(), bind<Dep3>(), bind<MultiArg>()};

  auto result = sut.template resolve<MultiArg>();
  EXPECT_EQ(6, result.sum);  // 1 + 2 + 3
}

TEST_F(ContainerEdgeCasesTest, resolve_same_type_multiple_ways) {
  struct Type : Singleton {};

  auto sut = Container{bind<Type>().in<scope::Singleton>()};

  auto value = sut.template resolve<Type>();  // copy
  auto& ref = sut.template resolve<Type&>();
  auto* ptr = sut.template resolve<Type*>();
  auto shared = sut.template resolve<std::shared_ptr<Type>>();

  EXPECT_EQ(initial_value, value.value);
  EXPECT_EQ(value.value, ref.value);  // Value is a copy, but has same value.
  EXPECT_EQ(&ref, ptr);
  EXPECT_EQ(ptr, shared.get());
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

  // Can't resolve by value, but can resolve by reference.
  auto& ref = sut.template resolve<NoCopy&>();
  EXPECT_EQ(initial_value, ref.value);

  // Can resolve as pointer.
  auto* ptr = sut.template resolve<NoCopy*>();
  EXPECT_EQ(initial_value, ptr->value);
}

TEST_F(ContainerEdgeCasesTest, resolve_from_multiple_containers) {
  struct Type : ValueInitialized {
    using ValueInitialized::ValueInitialized;
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

TEST_F(ContainerMixedScopesTest, all_scopes_coexist) {
  using TransientType = Initialized;
  struct SingletonType : Singleton {};
  struct InstanceType : Initialized {};
  auto external = InstanceType{};

  auto sut = Container{bind<TransientType>().in<scope::Transient>(),
                       bind<SingletonType>().in<scope::Singleton>(),
                       bind<InstanceType>().to(external)};

  // Initialized creates new each time
  auto t1 = sut.template resolve<TransientType>();
  auto t2 = sut.template resolve<TransientType>();
  EXPECT_NE(t1.id, t2.id);

  // Singleton returns same reference
  auto& s1 = sut.template resolve<SingletonType&>();
  auto& s2 = sut.template resolve<SingletonType&>();
  EXPECT_EQ(&s1, &s2);

  // InstanceType wraps external
  auto& e1 = sut.template resolve<InstanceType&>();
  EXPECT_EQ(&external, &e1);
}

// ----------------------------------------------------------------------------
// Unbound Type Tests
// ----------------------------------------------------------------------------

struct ContainerUnboundTypeTest : ContainerTest {};

TEST_F(ContainerUnboundTypeTest, unbound_type_uses_transient_scope) {
  struct Bound {};
  struct Unbound {};
  auto sut = Container{bind<Bound>()};

  [[maybe_unused]] auto instance = sut.template resolve<Unbound>();
}

TEST_F(ContainerUnboundTypeTest, unbound_type_with_dependencies) {
  struct Service {
    int_t result;
    explicit Service(Dependency d) : result{d.value * 2} {}
  };

  auto sut = Container{bind<Dependency>()};

  auto service = sut.template resolve<Service>();
  EXPECT_EQ(initial_value * 2, service.result);
}

TEST_F(ContainerUnboundTypeTest, unbound_type_normally_transient) {
  struct Type : Counted {};

  auto sut = Container{};

  auto val1 = sut.template resolve<Type>();
  auto val2 = sut.template resolve<Type>();

  EXPECT_NE(&val1, &val2);
  EXPECT_EQ(0, val1.id);
  EXPECT_EQ(1, val2.id);
}

TEST_F(ContainerUnboundTypeTest, promoted_unbound_type_caches_references) {
  struct Type : Counted {};

  auto sut = Container{};

  auto& ref1 = sut.template resolve<Type&>();
  auto& ref2 = sut.template resolve<Type&>();

  EXPECT_EQ(&ref1, &ref2);
  EXPECT_EQ(0, ref1.id);
  EXPECT_EQ(1, Counted::num_instances);
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

TEST_F(ContainerPromotionTest, values_not_promoted) {
  using Type = Initialized;
  auto sut = Container{bind<Type>().in<scope::Transient>()};

  auto val1 = sut.template resolve<Type>();
  auto val2 = sut.template resolve<Type>();

  EXPECT_EQ(0, val1.id);
  EXPECT_EQ(1, val2.id);
  EXPECT_EQ(2, Counted::num_instances);
}

TEST_F(ContainerPromotionTest, rvalue_references_not_promoted) {
  using Type = Initialized;
  auto sut = Container{bind<Type>().in<scope::Transient>()};

  auto val1 = sut.template resolve<Type&&>();
  auto val2 = sut.template resolve<Type&&>();

  EXPECT_EQ(0, val1.id);
  EXPECT_EQ(1, val2.id);
  EXPECT_EQ(2, Counted::num_instances);
}

TEST_F(ContainerPromotionTest, references_are_promoted) {
  struct Type : Singleton {};
  auto sut = Container{bind<Type>().in<scope::Transient>()};

  auto& ref1 = sut.template resolve<Type&>();
  auto& ref2 = sut.template resolve<Type&>();

  EXPECT_EQ(&ref1, &ref2);
  EXPECT_EQ(0, ref1.id);
  EXPECT_EQ(1, Counted::num_instances);
}

TEST_F(ContainerPromotionTest, references_to_const_are_promoted) {
  struct Type : Singleton {};
  auto sut = Container{bind<Type>().in<scope::Transient>()};

  const auto& ref1 = sut.template resolve<const Type&>();
  const auto& ref2 = sut.template resolve<const Type&>();

  EXPECT_EQ(&ref1, &ref2);
  EXPECT_EQ(0, ref1.id);
  EXPECT_EQ(1, Counted::num_instances);
}

TEST_F(ContainerPromotionTest, pointers_are_promoted) {
  struct Type : Singleton {};
  auto sut = Container{bind<Type>().in<scope::Transient>()};

  auto* ptr1 = sut.template resolve<Type*>();
  auto* ptr2 = sut.template resolve<Type*>();

  EXPECT_EQ(ptr1, ptr2);
  EXPECT_EQ(0, ptr1->id);
  EXPECT_EQ(1, Counted::num_instances);
}

TEST_F(ContainerPromotionTest, pointers_to_const_are_promoted) {
  struct Type : Singleton {};
  auto sut = Container{bind<Type>().in<scope::Transient>()};

  const auto* ptr1 = sut.template resolve<const Type*>();
  const auto* ptr2 = sut.template resolve<const Type*>();

  EXPECT_EQ(ptr1, ptr2);
  EXPECT_EQ(0, ptr1->id);
  EXPECT_EQ(1, Counted::num_instances);
}

TEST_F(ContainerPromotionTest, shared_ptrs_not_promoted) {
  using Type = Initialized;
  auto sut = Container{bind<Type>().in<scope::Transient>()};

  auto shared1 = sut.template resolve<std::shared_ptr<Type>>();
  auto shared2 = sut.template resolve<std::shared_ptr<Type>>();

  EXPECT_NE(shared1.get(), shared2.get());
  EXPECT_EQ(0, shared1->id);
  EXPECT_EQ(1, shared2->id);
  EXPECT_EQ(2, Counted::num_instances);
}

TEST_F(ContainerPromotionTest, weak_ptrs_are_promoted) {
  struct Type : Singleton {};
  auto sut = Container{bind<Type>().in<scope::Transient>()};

  auto weak1 = sut.template resolve<std::weak_ptr<Type>>();
  auto weak2 = sut.template resolve<std::weak_ptr<Type>>();

  EXPECT_FALSE(weak1.expired());
  EXPECT_EQ(weak1.lock(), weak2.lock());
  EXPECT_EQ(0, weak1.lock()->id);
  EXPECT_EQ(1, Counted::num_instances);
}

TEST_F(ContainerPromotionTest, unique_ptrs_not_promoted) {
  using Type = Initialized;
  auto sut = Container{bind<Type>().in<scope::Transient>()};

  auto unique1 = sut.template resolve<std::unique_ptr<Type>>();
  auto unique2 = sut.template resolve<std::unique_ptr<Type>>();

  EXPECT_NE(unique1.get(), unique2.get());
  EXPECT_EQ(0, unique1->id);
  EXPECT_EQ(1, unique2->id);
  EXPECT_EQ(2, Counted::num_instances);
}

TEST_F(ContainerPromotionTest, multiple_promotions_different_requests) {
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

TEST_F(ContainerPromotionTest, promotion_with_dependencies) {
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

TEST_F(ContainerRelegationTest, values_are_relegated) {
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

TEST_F(ContainerRelegationTest, rvalue_references_are_relegated) {
  struct Type : Singleton {};
  auto sut = Container{bind<Type>().in<scope::Singleton>()};

  auto&& rval1 = sut.template resolve<Type&&>();
  auto&& rval2 = sut.template resolve<Type&&>();

  EXPECT_NE(&rval1, &rval2);
  EXPECT_EQ(0, rval1.id);
  EXPECT_EQ(0, rval2.id);
  EXPECT_EQ(1, Counted::num_instances);
}

TEST_F(ContainerRelegationTest, unique_ptrs_are_relegated) {
  struct Type : Singleton {};
  auto sut = Container{bind<Type>().in<scope::Singleton>()};

  auto unique1 = sut.template resolve<std::unique_ptr<Type>>();
  auto unique2 = sut.template resolve<std::unique_ptr<Type>>();

  EXPECT_NE(unique1.get(), unique2.get());
  EXPECT_EQ(0, unique1->id);
  EXPECT_EQ(1, unique2->id);
  EXPECT_EQ(2, Counted::num_instances);
}

TEST_F(ContainerRelegationTest, references_not_relegated) {
  struct Type : Singleton {};
  auto sut = Container{bind<Type>().in<scope::Singleton>()};

  // References should still be singleton
  auto& ref1 = sut.template resolve<Type&>();
  auto& ref2 = sut.template resolve<Type&>();

  EXPECT_EQ(&ref1, &ref2);
  EXPECT_EQ(0, ref1.id);
  EXPECT_EQ(1, Counted::num_instances);
}

TEST_F(ContainerRelegationTest, pointers_not_relegated) {
  struct Type : Singleton {};
  auto sut = Container{bind<Type>().in<scope::Singleton>()};

  // Pointers should still be singleton
  auto* ptr1 = sut.template resolve<Type*>();
  auto* ptr2 = sut.template resolve<Type*>();

  EXPECT_EQ(ptr1, ptr2);
  EXPECT_EQ(0, ptr1->id);
  EXPECT_EQ(1, Counted::num_instances);
}

TEST_F(ContainerRelegationTest, shared_ptr_not_relegated) {
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

TEST_F(ContainerRelegationTest,
       singleton_shared_ptr_wraps_singleton_not_relegated) {
  struct Type : Singleton {};
  auto sut = Container{bind<Type>().in<scope::Singleton>()};

  // Modify the singleton.
  auto& singleton = sut.template resolve<Type&>();
  singleton.value = modified_value;

  // shared_ptr should wrap the singleton, showing the modified value.
  auto shared = sut.template resolve<std::shared_ptr<Type>>();
  EXPECT_EQ(modified_value, shared->value);
  EXPECT_EQ(&singleton, shared.get());

  // Values are copies of the singleton with the modified value.
  auto val = sut.template resolve<Type>();
  EXPECT_EQ(modified_value, val.value);  // Copy of modified singleton
  EXPECT_NE(&singleton, &val);           // But different address

  EXPECT_EQ(1, Counted::num_instances);  // Only 1 singleton instance
}

TEST_F(ContainerRelegationTest,
       singleton_relegation_creates_copies_not_fresh_instances) {
  struct Type : Singleton {};
  auto sut = Container{bind<Type>().in<scope::Singleton>()};

  // Get singleton reference and modify it.
  auto& singleton = sut.template resolve<Type&>();
  singleton.value = modified_value;

  // Values are copies of the singleton. This creates copies of the
  // modified singleton, not fresh instances from the provider.
  auto val1 = sut.template resolve<Type>();
  auto val2 = sut.template resolve<Type>();

  // Copies of modified singleton, not default values from provider.
  EXPECT_EQ(modified_value, val1.value);
  EXPECT_EQ(modified_value, val2.value);

  // Copies are independent from each other and from singleton.
  EXPECT_NE(&singleton, &val1);
  EXPECT_NE(&singleton, &val2);
  EXPECT_NE(&val1, &val2);

  // Singleton itself is unchanged.
  EXPECT_EQ(modified_value, singleton.value);
}

TEST_F(ContainerRelegationTest, singleton_relegation_with_dependencies) {
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
  auto service1 = sut.template resolve<ServiceType>();
  auto service2 = sut.template resolve<ServiceType>();

  EXPECT_NE(&service1, &service2);          // Independent copies
  EXPECT_NE(&service1.dep, &service2.dep);  // Each copy has its own dep copy

  // Singletons created: Dependency (id=0) then Service (id=1).
  EXPECT_EQ(0, service1.dep.id);  // Copy of Dependency singleton
  EXPECT_EQ(1, service1.id);      // Copy of Service singleton

  // Both values are copies of the same singletons.
  EXPECT_EQ(0, service2.dep.id);  // Copy of same Dependency singleton
  EXPECT_EQ(1, service2.id);      // Copy of same Service singleton

  EXPECT_EQ(
      2,
      Counted::num_instances);  // 1 Service singleton + 1 Dependency singleton
}

// ----------------------------------------------------------------------------
// Hierarchical Container Tests - Basic Delegation
// ----------------------------------------------------------------------------

struct ContainerHierarchyTest : ContainerTest {
  struct Type : Initialized {};
};

TEST_F(ContainerHierarchyTest, child_finds_binding_in_parent) {
  auto parent = Container{bind<Type>()};
  auto child = Container{parent};

  auto result = child.template resolve<Type>();
  EXPECT_EQ(initial_value, result.value);
}

TEST_F(ContainerHierarchyTest, child_overrides_parent_binding) {
  auto parent_factory = []() { return Product{initial_value}; };
  auto child_factory = []() { return Product{modified_value}; };

  auto parent = Container{bind<Product>().via(parent_factory)};
  auto child = Container{parent, bind<Product>().via(child_factory)};

  auto parent_result = parent.template resolve<Product>();
  auto child_result = child.template resolve<Product>();

  EXPECT_EQ(initial_value, parent_result.value);
  EXPECT_EQ(modified_value, child_result.value);
}

TEST_F(ContainerHierarchyTest, multi_level_hierarchy) {
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

  // Child can resolve from all levels
  auto grandparent_result = child.template resolve<Grandparent>();
  auto parent_result = child.template resolve<Parent>();
  auto child_result = child.template resolve<Child>();

  EXPECT_EQ(1, grandparent_result.value);
  EXPECT_EQ(2, parent_result.value);
  EXPECT_EQ(3, child_result.value);
}

TEST_F(ContainerHierarchyTest,
       child_overrides_parent_in_multi_level_hierarchy) {
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

TEST_F(ContainerHierarchyTest, unbound_type_uses_fallback_in_hierarchy) {
  auto parent = Container{};
  auto child = Container{parent};

  // Should use fallback binding at the root level (for 'Type')
  auto result = child.template resolve<Type>();
  EXPECT_EQ(initial_value, result.value);
}

// ----------------------------------------------------------------------------
// Hierarchical Container Tests - Singleton Sharing
// ----------------------------------------------------------------------------

struct ContainerHierarchySingletonTest : ContainerTest {};

TEST_F(ContainerHierarchySingletonTest, singleton_in_parent_shared_with_child) {
  struct Type : Singleton {};

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

TEST_F(ContainerHierarchySingletonTest,
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

TEST_F(ContainerHierarchySingletonTest,
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

struct ContainerHierarchyTransientTest : ContainerTest {
  // This type is only used for transient/value resolution,
  // so it can be shared by all tests in this fixture.
  struct Type : Initialized {};
};

TEST_F(ContainerHierarchyTransientTest,
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

TEST_F(ContainerHierarchyTransientTest,
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
//
// These tests require unique local types for promoted instances.
//

struct ContainerHierarchyPromotionTest : ContainerTest {};

TEST_F(ContainerHierarchyPromotionTest, child_promotes_transient_from_parent) {
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

TEST_F(ContainerHierarchyPromotionTest,
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

TEST_F(ContainerHierarchyPromotionTest,
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

TEST_F(ContainerHierarchyPromotionTest,
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
TEST_F(ContainerHierarchyPromotionTest,
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
// Hierarchical Container Tests - Relegation in Hierarchy
// ----------------------------------------------------------------------------

struct ContainerHierarchyRelegationTest : ContainerTest {};

TEST_F(ContainerHierarchyRelegationTest,
       child_relegates_singleton_from_parent) {
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

TEST_F(ContainerHierarchyRelegationTest,
       parent_singleton_reference_differs_from_child_relegated_values) {
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

TEST_F(ContainerHierarchyRelegationTest,
       grandparent_singleton_reference_accessible_but_child_can_relegate) {
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

// ----------------------------------------------------------------------------
// Complex Hierarchical Scenarios
// ----------------------------------------------------------------------------

struct ContainerHierarchyComplexTest : ContainerTest {};

TEST_F(ContainerHierarchyComplexTest, mixed_scopes_across_hierarchy) {
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

TEST_F(ContainerHierarchyComplexTest, dependency_chain_across_hierarchy) {
  struct GrandparentDep : Singleton {};
  struct ParentDep : Initialized {
    GrandparentDep* dep;
    explicit ParentDep(GrandparentDep& d) : dep{&d} {}
  };
  struct ChildService : Singleton {
    ParentDep* dep;
    explicit ChildService(ParentDep& d) : dep{&d} {}
  };

  auto grandparent = Container{bind<GrandparentDep>().in<scope::Singleton>()};
  // ParentDep is unbound, will be promoted.
  auto parent = Container{grandparent, bind<ParentDep>()};
  // ChildService is unbound, will be promoted.
  auto child = Container{parent, bind<ChildService>()};

  auto& service = child.template resolve<ChildService&>();

  EXPECT_EQ(0, service.dep->dep->id);  // GrandparentDep (singleton)
  EXPECT_EQ(1, service.dep->id);       // ParentDep (promoted in parent)
  EXPECT_EQ(2, service.id);            // ChildService (promoted in child)
  EXPECT_EQ(3, Counted::num_instances);
}

TEST_F(ContainerHierarchyComplexTest,
       promotion_and_relegation_across_hierarchy) {
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
  EXPECT_EQ(1, child_val1.id);          // Both copies of child singleton (id 1)
  EXPECT_EQ(1, child_val2.id);          // Both copies of child singleton (id 1)

  EXPECT_EQ(2,
            Counted::num_instances);  // 1 parent (promoted) + 1 child singleton
}

TEST_F(ContainerHierarchyComplexTest,
       sibling_containers_share_parent_promotion_when_delegating) {
  struct Type : Singleton {};

  auto parent = Container{bind<Type>().in<scope::Transient>()};
  auto child1 = Container{parent};  // No binding, delegates to parent
  auto child2 = Container{parent};  // No binding, delegates to parent

  // Both children delegate to parent, share parent's promoted instance.
  auto& child1_ref = child1.template resolve<Type&>();
  auto& child2_ref = child2.template resolve<Type&>();

  EXPECT_EQ(&child1_ref, &child2_ref);  // Same instance.
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
  struct Type : Singleton {};

  auto parent = Container{bind<Type>().in<scope::Transient>()};

  // These two containers have the same type.
  auto child1 = Container(parent, bind<Type>().in<scope::Singleton>());
  auto child2 = Container(parent, bind<Type>().in<scope::Singleton>());
  static_assert(std::same_as<decltype(child1), decltype(child2)>);

  // Children with the same type have the same binding, even during promotion.
  auto& child1_ref = child1.template resolve<Type&>();
  auto& child2_ref = child2.template resolve<Type&>();

  EXPECT_EQ(&child1_ref, &child2_ref);
  EXPECT_EQ(0, child1_ref.id);
  EXPECT_EQ(0, child2_ref.id);
  EXPECT_EQ(1, Counted::num_instances);
}

// Promoted instances are real singletons.
TEST_F(ContainerHierarchyComplexTest,
       sibling_containers_with_same_promoted_type_share_singletons) {
  struct Type : Singleton {};

  auto parent = Container{bind<Type>().in<scope::Transient>()};

  // These two containers have the same type.
  auto child1 = Container(parent, bind<Type>().in<scope::Transient>());
  auto child2 = Container(parent, bind<Type>().in<scope::Transient>());
  static_assert(std::same_as<decltype(child1), decltype(child2)>);

  // Children with the same type have the same binding, even during promotion.
  auto& child1_ref = child1.template resolve<Type&>();
  auto& child2_ref = child2.template resolve<Type&>();

  EXPECT_EQ(&child1_ref, &child2_ref);
  EXPECT_EQ(0, child1_ref.id);
  EXPECT_EQ(0, child2_ref.id);
  EXPECT_EQ(1, Counted::num_instances);
}

TEST_F(ContainerHierarchyComplexTest,
       sibling_containers_using_macro_are_independent_with_own_bindings) {
  struct Type : Singleton {};

  auto parent = Container{bind<Type>().in<scope::Transient>()};

  // These two have unique types.
  auto child1 =
      dink_unique_container(parent, bind<Type>().in<scope::Singleton>());
  auto child2 =
      dink_unique_container(parent, bind<Type>().in<scope::Singleton>());
  static_assert(!std::same_as<decltype(child1), decltype(child2)>);

  // Each child has own binding, promotes separately
  auto& child1_ref = child1.template resolve<Type&>();
  auto& child2_ref = child2.template resolve<Type&>();

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
  struct Type : Singleton {};

  auto parent = Container{bind<Type>().in<scope::Singleton>()};
  auto child = Container{parent};

  auto& parent_ref = parent.template resolve<Type&>();
  auto& child_ref = child.template resolve<Type&>();

  EXPECT_EQ(&parent_ref, &child_ref);
  EXPECT_EQ(0, parent_ref.id);
  EXPECT_EQ(0, child_ref.id);
  EXPECT_EQ(1, Counted::num_instances);
}

TEST_F(ContainerHierarchyComplexTest, deep_hierarchy_with_multiple_overrides) {
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

}  // namespace
}  // namespace dink
