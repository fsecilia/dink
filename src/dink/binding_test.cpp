// \file
// Copyright (c) 2025 Frank Secilia
// SPDX-License-Identifier: MIT

#include "binding.hpp"
#include <dink/test.hpp>

namespace dink {
namespace {

// ----------------------------------------------------------------------------
// Test Fixtures
// ----------------------------------------------------------------------------

// Interface type
struct IFoo {};

// Concrete implementation
struct ConcreteFoo {};

// Another concrete type
struct Bar {};

// Instance type for reference binding
struct Instance {
  int value = 42;
};

// Constexpr factory for compile-time tests
constexpr auto test_factory = []() constexpr { return ConcreteFoo{}; };

// Another constexpr factory
constexpr auto bar_factory = []() constexpr { return Bar{}; };

// Normalized factory types (removes const from lambda type)
using TestFactory = std::remove_const_t<decltype(test_factory)>;
using BarFactory = std::remove_const_t<decltype(bar_factory)>;

// ----------------------------------------------------------------------------
// Type Transition Tests - Verify builder state transitions
// ----------------------------------------------------------------------------

// bind<From>() produces BindBuilder<From>
static_assert(std::same_as<decltype(bind<IFoo>()), BindBuilder<IFoo>>);

// .as<To>() produces AsBuilder<From, To>
static_assert(std::same_as<decltype(bind<IFoo>().as<ConcreteFoo>()),
                           AsBuilder<IFoo, ConcreteFoo>>);

// .as<To>().via(factory) produces ViaBuilder<From, To, Factory>
static_assert(
    std::same_as<decltype(bind<IFoo>().as<ConcreteFoo>().via(test_factory)),
                 ViaBuilder<IFoo, ConcreteFoo, TestFactory>>);

// .in<Scope>() from BindBuilder produces InBuilder
static_assert(std::same_as<
              decltype(bind<IFoo>().in<scope::Singleton>()),
              InBuilder<IFoo, IFoo, provider::Ctor<IFoo>, scope::Singleton>>);

// .as<To>().in<Scope>() produces InBuilder
static_assert(std::same_as<
              decltype(bind<IFoo>().as<ConcreteFoo>().in<scope::Transient>()),
              InBuilder<IFoo, ConcreteFoo, provider::Ctor<ConcreteFoo>,
                        scope::Transient>>);

// .as<To>().via(factory).in<Scope>() produces InBuilder
static_assert(
    std::same_as<decltype(bind<IFoo>()
                              .as<ConcreteFoo>()
                              .via(test_factory)
                              .in<scope::Singleton>()),
                 InBuilder<IFoo, ConcreteFoo,
                           provider::Factory<ConcreteFoo, TestFactory>,
                           scope::Singleton>>);

// ----------------------------------------------------------------------------
// Binding Conversion Tests - Verify final Binding types
// ----------------------------------------------------------------------------

// bind<From>() converts to Binding<From, Transient, Ctor<From>>
static_assert(
    std::same_as<decltype(Binding{bind<IFoo>()}),
                 Binding<IFoo, scope::Transient, provider::Ctor<IFoo>>>);

// .as<To>() converts to Binding<From, Transient, Ctor<To>>
static_assert(
    std::same_as<decltype(Binding{bind<IFoo>().as<ConcreteFoo>()}),
                 Binding<IFoo, scope::Transient, provider::Ctor<ConcreteFoo>>>);

// .as<To>().via(factory) converts to Binding<From, Transient, Factory<To, F>>
static_assert(
    std::same_as<decltype(Binding{
                     bind<IFoo>().as<ConcreteFoo>().via(test_factory)}),
                 Binding<IFoo, scope::Transient,
                         provider::Factory<ConcreteFoo, TestFactory>>>);

// .in<Singleton>() converts to Binding<From, Singleton, Ctor<From>>
static_assert(
    std::same_as<decltype(Binding{bind<IFoo>().in<scope::Singleton>()}),
                 Binding<IFoo, scope::Singleton, provider::Ctor<IFoo>>>);

// .in<Transient>() converts to Binding<From, Transient, Ctor<From>>
static_assert(
    std::same_as<decltype(Binding{bind<IFoo>().in<scope::Transient>()}),
                 Binding<IFoo, scope::Transient, provider::Ctor<IFoo>>>);

// .as<To>().in<Scope>() converts to Binding<From, Scope, Ctor<To>>
static_assert(
    std::same_as<decltype(Binding{
                     bind<IFoo>().as<ConcreteFoo>().in<scope::Transient>()}),
                 Binding<IFoo, scope::Transient, provider::Ctor<ConcreteFoo>>>);

// .as<To>().via(f).in<Scope>() converts to Binding<From, Scope, Factory<To, F>>
static_assert(
    std::same_as<decltype(Binding{bind<IFoo>()
                                      .as<ConcreteFoo>()
                                      .via(test_factory)
                                      .in<scope::Singleton>()}),
                 Binding<IFoo, scope::Singleton,
                         provider::Factory<ConcreteFoo, TestFactory>>>);

// .to(instance) converts to Binding<From, Instance<T>, Instance<T>>
// Note: This is a placeholder - needs proper provider type for external refs
static_assert([]() constexpr {
  Instance inst;
  return std::same_as<
      decltype(Binding{bind<IFoo>().to(inst)}),
      Binding<IFoo, scope::Instance<Instance>, scope::Instance<Instance>>>;
}());

// ----------------------------------------------------------------------------
// Exhaustive Binding Type Tests
// ----------------------------------------------------------------------------

// All Ctor combinations with scopes
static_assert(
    std::same_as<decltype(Binding{bind<IFoo>().in<scope::Transient>()}),
                 Binding<IFoo, scope::Transient, provider::Ctor<IFoo>>>);

static_assert(
    std::same_as<decltype(Binding{bind<IFoo>().in<scope::Singleton>()}),
                 Binding<IFoo, scope::Singleton, provider::Ctor<IFoo>>>);

// All Factory combinations with scopes
static_assert(
    std::same_as<decltype(Binding{bind<IFoo>()
                                      .as<ConcreteFoo>()
                                      .via(test_factory)
                                      .in<scope::Transient>()}),
                 Binding<IFoo, scope::Transient,
                         provider::Factory<ConcreteFoo, TestFactory>>>);

static_assert(
    std::same_as<decltype(Binding{bind<IFoo>()
                                      .as<ConcreteFoo>()
                                      .via(test_factory)
                                      .in<scope::Singleton>()}),
                 Binding<IFoo, scope::Singleton,
                         provider::Factory<ConcreteFoo, TestFactory>>>);

// ----------------------------------------------------------------------------
// Tuple Construction Test - Real-world usage pattern
// ----------------------------------------------------------------------------

// Test that bindings can be stored in a tuple (forces rvalue conversions)
static_assert([]() constexpr {
  Instance inst;

  auto bindings = make_bindings(
      bind<IFoo>(), bind<IFoo>().as<ConcreteFoo>(),
      bind<IFoo>().as<ConcreteFoo>().via(test_factory),
      bind<Bar>().as<Bar>().via(bar_factory).in<scope::Singleton>(),
      bind<IFoo>().in<scope::Singleton>(),
      bind<IFoo>().as<ConcreteFoo>().in<scope::Transient>(),
      bind<Instance>().to(inst));

  // Verify tuple element types
  using Tuple = decltype(bindings);

  static_assert(
      std::same_as<std::tuple_element_t<0, Tuple>,
                   Binding<IFoo, scope::Transient, provider::Ctor<IFoo>>>);

  static_assert(std::same_as<
                std::tuple_element_t<1, Tuple>,
                Binding<IFoo, scope::Transient, provider::Ctor<ConcreteFoo>>>);

  static_assert(
      std::same_as<std::tuple_element_t<2, Tuple>,
                   Binding<IFoo, scope::Transient,
                           provider::Factory<ConcreteFoo, TestFactory>>>);

  static_assert(
      std::same_as<
          std::tuple_element_t<3, Tuple>,
          Binding<Bar, scope::Singleton, provider::Factory<Bar, BarFactory>>>);

  static_assert(
      std::same_as<std::tuple_element_t<4, Tuple>,
                   Binding<IFoo, scope::Singleton, provider::Ctor<IFoo>>>);

  static_assert(std::same_as<
                std::tuple_element_t<5, Tuple>,
                Binding<IFoo, scope::Transient, provider::Ctor<ConcreteFoo>>>);

  static_assert(std::same_as<std::tuple_element_t<6, Tuple>,
                             Binding<Instance, scope::Instance<Instance>,
                                     scope::Instance<Instance>>>);

  return true;
}());

// ----------------------------------------------------------------------------
// FromType Alias Test
// ----------------------------------------------------------------------------

static_assert(
    std::same_as<
        Binding<IFoo, scope::Transient, provider::Ctor<IFoo>>::FromType, IFoo>);

static_assert(
    std::same_as<Binding<Bar, scope::Singleton, provider::Ctor<Bar>>::FromType,
                 Bar>);

// ----------------------------------------------------------------------------
// Runtime Tests (for non-constexpr factories)
// ----------------------------------------------------------------------------

struct RuntimeTest : Test {};

TEST_F(RuntimeTest, BindWithRuntimeFactory) {
  // Runtime factory (not constexpr)
  auto runtime_factory = []() { return ConcreteFoo{}; };

  // Should still compile and deduce types correctly
  auto binding = Binding{bind<IFoo>().as<ConcreteFoo>().via(runtime_factory)};

  using Expected =
      Binding<IFoo, scope::Transient,
              provider::Factory<ConcreteFoo, decltype(runtime_factory)>>;

  static_assert(std::same_as<decltype(binding), Expected>);
}

TEST_F(RuntimeTest, BindToInstanceReference) {
  Instance inst{99};

  auto binding = Binding{bind<Instance>().to(inst)};

  using Expected =
      Binding<Instance, scope::Instance<Instance>, scope::Instance<Instance>>;
  static_assert(std::same_as<decltype(binding), Expected>);
}

TEST_F(RuntimeTest, HeterogeneousTuple) {
  Instance inst;
  auto runtime_factory = []() { return ConcreteFoo{}; };

  // Mix of constexpr and runtime bindings
  auto bindings = make_bindings(
      bind<IFoo>(), bind<IFoo>().as<ConcreteFoo>().via(runtime_factory),
      bind<Instance>().to(inst));

  EXPECT_EQ(std::tuple_size_v<decltype(bindings)>, 3);
}

// ----------------------------------------------------------------------------
// Documentation Examples
// ----------------------------------------------------------------------------

// These are the examples from the implementation comments - verify they work
struct DocumentationExamples : Test {};

TEST_F(DocumentationExamples, AllBasicPatterns) {
  Instance inst;
  auto factory = []() { return ConcreteFoo{}; };

  // Each line from the documentation should compile
  [[maybe_unused]] auto b1 = Binding{bind<IFoo>()};
  [[maybe_unused]] auto b2 = Binding{bind<IFoo>().as<ConcreteFoo>()};
  [[maybe_unused]] auto b3 =
      Binding{bind<IFoo>().as<ConcreteFoo>().via(factory)};
  [[maybe_unused]] auto b4 = Binding{bind<IFoo>().in<scope::Singleton>()};
  [[maybe_unused]] auto b5 = Binding{bind<Instance>().to(inst)};
}

// ----------------------------------------------------------------------------
// Invalid Sequences (documented for reference - uncomment to verify rejection)
// ----------------------------------------------------------------------------

/*
// These should NOT compile - verify manually:

// Can't call .as() twice
// bind<IFoo>().as<ConcreteFoo>().as<Bar>();

// ToBuilder is terminal - no .in()
// Instance inst;
// bind<IFoo>().to(inst).in<scope::Singleton>();

// InBuilder is terminal - no .via()
// bind<IFoo>().in<scope::Singleton>().via(test_factory);

// Can't call .via() without .as<>() first
// bind<IFoo>().via(test_factory);

// Can't call .in() twice
// bind<IFoo>().in<scope::Singleton>().in<scope::Transient>();
*/

}  // namespace
}  // namespace dink
