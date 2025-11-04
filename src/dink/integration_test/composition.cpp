/*
  Copyright (c) 2025 Frank Secilia \n
  SPDX-License-Identifier: MIT
*/

#include "integration_test.hpp"

namespace dink::container {
namespace {

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
