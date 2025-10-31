// \file
// Copyright (c) 2025 Frank Secilia
// SPDX-License-Identifier: MIT

#include "binding.hpp"
#include <dink/test.hpp>

namespace dink {
namespace {

// ----------------------------------------------------------------------------
// Fixtures
// ----------------------------------------------------------------------------

// Interface type
struct Interface {};

// Concrete implementation
struct Implementation {};

// Another concrete type
struct Concrete {};

// Instance type for reference binding
struct Instance {
  int value = 42;
};

// Constexpr factory for compile-time tests
constexpr auto test_factory = []() constexpr { return Implementation{}; };

// Another constexpr factory
constexpr auto concrete_factory = []() constexpr { return Concrete{}; };

// Normalized factory types (removes const from lambda type)
using TestFactory = std::remove_const_t<decltype(test_factory)>;
using ConcreteFactory = std::remove_const_t<decltype(concrete_factory)>;

// ----------------------------------------------------------------------------
// Type Transition Tests - Verify builder state transitions
// ----------------------------------------------------------------------------

// bind<From>() produces BindBuilder<From>
static_assert(
    std::same_as<decltype(bind<Interface>()), BindBuilder<Interface>>);

// .as<To>() produces AsBuilder<From, To>
static_assert(std::same_as<decltype(bind<Interface>().as<Implementation>()),
                           AsBuilder<Interface, Implementation>>);

// .as<To>().via(factory) produces ViaBuilder<From, To, Factory>
static_assert(std::same_as<decltype(bind<Interface>().as<Implementation>().via(
                               test_factory)),
                           ViaBuilder<Interface, Implementation, TestFactory>>);

// .in<Scope>() from BindBuilder produces InBuilder
static_assert(
    std::same_as<decltype(bind<Interface>().in<scope::Singleton>()),
                 InBuilder<Interface, Interface, provider::Ctor<Interface>,
                           scope::Singleton>>);

// .as<To>().in<Scope>() produces InBuilder
static_assert(
    std::same_as<
        decltype(bind<Interface>().as<Implementation>().in<scope::Transient>()),
        InBuilder<Interface, Implementation, provider::Ctor<Implementation>,
                  scope::Transient>>);

// .as<To>().via(factory).in<Scope>() produces InBuilder
static_assert(
    std::same_as<decltype(bind<Interface>()
                              .as<Implementation>()
                              .via(test_factory)
                              .in<scope::Singleton>()),
                 InBuilder<Interface, Implementation,
                           provider::Factory<Implementation, TestFactory>,
                           scope::Singleton>>);

// ----------------------------------------------------------------------------
// Binding Conversion Tests - Verify final Binding types
// ----------------------------------------------------------------------------

// bind<From>() converts to Binding<From, Transient, Ctor<From>>
static_assert(std::same_as<
              decltype(Binding{bind<Interface>()}),
              Binding<Interface, scope::Transient, provider::Ctor<Interface>>>);

// .as<To>() converts to Binding<From, Transient, Ctor<To>>
static_assert(
    std::same_as<
        decltype(Binding{bind<Interface>().as<Implementation>()}),
        Binding<Interface, scope::Transient, provider::Ctor<Implementation>>>);

// .as<To>().via(factory) converts to Binding<From, Transient, Factory<To, F>>
static_assert(
    std::same_as<decltype(Binding{
                     bind<Interface>().as<Implementation>().via(test_factory)}),
                 Binding<Interface, scope::Transient,
                         provider::Factory<Implementation, TestFactory>>>);

// .in<Singleton>() converts to Binding<From, Singleton, Ctor<From>>
static_assert(std::same_as<
              decltype(Binding{bind<Interface>().in<scope::Singleton>()}),
              Binding<Interface, scope::Singleton, provider::Ctor<Interface>>>);

// .in<Transient>() converts to Binding<From, Transient, Ctor<From>>
static_assert(std::same_as<
              decltype(Binding{bind<Interface>().in<scope::Transient>()}),
              Binding<Interface, scope::Transient, provider::Ctor<Interface>>>);

// .as<To>().in<Scope>() converts to Binding<From, Scope, Ctor<To>>
static_assert(
    std::same_as<
        decltype(Binding{
            bind<Interface>().as<Implementation>().in<scope::Transient>()}),
        Binding<Interface, scope::Transient, provider::Ctor<Implementation>>>);

// .as<To>().via(f).in<Scope>() converts to Binding<From, Scope, Factory<To, F>>
static_assert(
    std::same_as<decltype(Binding{bind<Interface>()
                                      .as<Implementation>()
                                      .via(test_factory)
                                      .in<scope::Singleton>()}),
                 Binding<Interface, scope::Singleton,
                         provider::Factory<Implementation, TestFactory>>>);

// .to(instance) converts to Binding<From, Instance<T>, Instance<T>>
// Note: This is a placeholder - needs proper provider type for external refs
static_assert([]() constexpr {
  Instance inst;
  return std::same_as<
      decltype(Binding{bind<Interface>().to(inst)}),
      Binding<Interface, scope::Instance, provider::External<Instance>>>;
}());

// ----------------------------------------------------------------------------
// Exhaustive Binding Type Tests
// ----------------------------------------------------------------------------

// All Ctor combinations with scopes
static_assert(std::same_as<
              decltype(Binding{bind<Interface>().in<scope::Transient>()}),
              Binding<Interface, scope::Transient, provider::Ctor<Interface>>>);

static_assert(std::same_as<
              decltype(Binding{bind<Interface>().in<scope::Singleton>()}),
              Binding<Interface, scope::Singleton, provider::Ctor<Interface>>>);

// All Factory combinations with scopes
static_assert(
    std::same_as<decltype(Binding{bind<Interface>()
                                      .as<Implementation>()
                                      .via(test_factory)
                                      .in<scope::Transient>()}),
                 Binding<Interface, scope::Transient,
                         provider::Factory<Implementation, TestFactory>>>);

static_assert(
    std::same_as<decltype(Binding{bind<Interface>()
                                      .as<Implementation>()
                                      .via(test_factory)
                                      .in<scope::Singleton>()}),
                 Binding<Interface, scope::Singleton,
                         provider::Factory<Implementation, TestFactory>>>);

// ----------------------------------------------------------------------------
// Tuple Construction Test - Real-world usage pattern
// ----------------------------------------------------------------------------

// Test that bindings can be stored in a tuple (forces rvalue conversions)
static_assert([]() constexpr {
  Instance inst;

  auto bindings = make_bindings(
      bind<Interface>(), bind<Interface>().as<Implementation>(),
      bind<Interface>().as<Implementation>().via(test_factory),
      bind<Concrete>()
          .as<Concrete>()
          .via(concrete_factory)
          .in<scope::Singleton>(),
      bind<Interface>().in<scope::Singleton>(),
      bind<Interface>().as<Implementation>().in<scope::Transient>(),
      bind<Instance>().to(inst));

  // Verify tuple element types
  using Tuple = decltype(bindings);

  static_assert(
      std::same_as<
          std::tuple_element_t<0, Tuple>,
          Binding<Interface, scope::Transient, provider::Ctor<Interface>>>);

  static_assert(std::same_as<std::tuple_element_t<1, Tuple>,
                             Binding<Interface, scope::Transient,
                                     provider::Ctor<Implementation>>>);

  static_assert(
      std::same_as<std::tuple_element_t<2, Tuple>,
                   Binding<Interface, scope::Transient,
                           provider::Factory<Implementation, TestFactory>>>);

  static_assert(
      std::same_as<std::tuple_element_t<3, Tuple>,
                   Binding<Concrete, scope::Singleton,
                           provider::Factory<Concrete, ConcreteFactory>>>);

  static_assert(
      std::same_as<
          std::tuple_element_t<4, Tuple>,
          Binding<Interface, scope::Singleton, provider::Ctor<Interface>>>);

  static_assert(std::same_as<std::tuple_element_t<5, Tuple>,
                             Binding<Interface, scope::Transient,
                                     provider::Ctor<Implementation>>>);

  static_assert(
      std::same_as<
          std::tuple_element_t<6, Tuple>,
          Binding<Instance, scope::Instance, provider::External<Instance>>>);

  return true;
}());

// ----------------------------------------------------------------------------
// FromType Alias Test
// ----------------------------------------------------------------------------

static_assert(std::same_as<Binding<Interface, scope::Transient,
                                   provider::Ctor<Interface>>::FromType,
                           Interface>);

static_assert(std::same_as<Binding<Concrete, scope::Singleton,
                                   provider::Ctor<Concrete>>::FromType,
                           Concrete>);

// ----------------------------------------------------------------------------
// Runtime Tests (for non-constexpr factories)
// ----------------------------------------------------------------------------

struct RuntimeTest : Test {};

TEST_F(RuntimeTest, BindWithRuntimeFactory) {
  // Runtime factory (not constexpr)
  auto runtime_factory = []() { return Implementation{}; };

  // Should still compile and deduce types correctly
  auto binding =
      Binding{bind<Interface>().as<Implementation>().via(runtime_factory)};

  using Expected =
      Binding<Interface, scope::Transient,
              provider::Factory<Implementation, decltype(runtime_factory)>>;

  static_assert(std::same_as<decltype(binding), Expected>);
}

TEST_F(RuntimeTest, BindToInstanceReference) {
  auto instance = Instance{};

  [[maybe_unused]] auto binding = Binding{bind<Instance>().to(instance)};

  using Expected =
      Binding<Instance, scope::Instance, provider::External<Instance>>;
  static_assert(std::same_as<decltype(binding), Expected>);
}

TEST_F(RuntimeTest, HeterogeneousTuple) {
  Instance inst;
  auto runtime_factory = []() { return Implementation{}; };

  // Mix of constexpr and runtime bindings
  [[maybe_unused]] auto bindings =
      make_bindings(bind<Interface>(),
                    bind<Interface>().as<Implementation>().via(runtime_factory),
                    bind<Instance>().to(inst));

  EXPECT_EQ(std::tuple_size_v<decltype(bindings)>, 3);
}

// ----------------------------------------------------------------------------
// Documentation Examples
// ----------------------------------------------------------------------------

// Verify the examples from the implementation comments work
struct DocumentationExamples : Test {};

TEST_F(DocumentationExamples, ExampleUsage) {
  Instance inst;
  auto factory = []() { return Implementation{}; };

  // Each line from the documentation should compile verbatim.
  [[maybe_unused]] auto b1 = Binding{bind<Interface>()};
  [[maybe_unused]] auto b2 = Binding{bind<Interface>().as<Implementation>()};
  [[maybe_unused]] auto b3 =
      Binding{bind<Interface>().as<Implementation>().via(factory)};
  [[maybe_unused]] auto b4 = Binding{bind<Interface>().in<scope::Singleton>()};
  [[maybe_unused]] auto b5 = Binding{bind<Instance>().to(inst)};
}

}  // namespace
}  // namespace dink
