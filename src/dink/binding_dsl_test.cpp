// \file
// Copyright (c) 2025 Frank Secilia
// SPDX-License-Identifier: MIT

#include "binding_dsl.hpp"
#include <dink/test.hpp>

namespace dink {
namespace {

// ----------------------------------------------------------------------------
// Fixtures
// ----------------------------------------------------------------------------

// Arbitrary type.
struct Type {};
constexpr auto type_factory = []() constexpr { return Type{}; };
using TypeFactory = std::remove_const_t<decltype(type_factory)>;

// Abstract interface.
struct Interface {
  virtual ~Interface() = 0;
};
Interface::~Interface() {}

// Concrete implementation.
struct Implementation : Interface {
  ~Implementation() override = default;
};
constexpr auto implementation_factory = []() { return Implementation{}; };
using ImplementationFactory =
    std::remove_const_t<decltype(implementation_factory)>;

// Instance type for reference binding.
struct Instance {
  int value = 42;
};

//! cooks builder outputs into real bindings
template <IsConvertibleToBinding... Builders>
constexpr auto make_bindings(Builders&&... builders) {
  return std::tuple{Binding{std::forward<Builders>(builders)}...};
}

// ----------------------------------------------------------------------------
// Type Transition Tests - Verify builder state transitions
// ----------------------------------------------------------------------------

// bind<From>() produces BindBuilder<From>
static_assert(std::same_as<decltype(bind<Type>()), BindBuilder<Type>>);

// .via(factory) from BindBuilder produces ViaBuilder<From, From, Factory>
static_assert(std::same_as<decltype(bind<Type>().via(type_factory)),
                           ViaBuilder<Type, Type, TypeFactory>>);

// .as<To>() produces AsBuilder<From, To>
static_assert(std::same_as<decltype(bind<Interface>().as<Implementation>()),
                           AsBuilder<Interface, Implementation>>);

// .as<To>().via(factory) produces ViaBuilder<From, To, Factory>
static_assert(
    std::same_as<decltype(bind<Interface>().as<Implementation>().via(
                     implementation_factory)),
                 ViaBuilder<Interface, Implementation, ImplementationFactory>>);

// .in<Scope>() from BindBuilder produces InBuilder
static_assert(std::same_as<
              decltype(bind<Type>().in<scope::Singleton>()),
              InBuilder<Type, Type, provider::Ctor<Type>, scope::Singleton>>);

// .as<To>().in<Scope>() produces InBuilder
static_assert(
    std::same_as<
        decltype(bind<Interface>().as<Implementation>().in<scope::Transient>()),
        InBuilder<Interface, Implementation, provider::Ctor<Implementation>,
                  scope::Transient>>);

// .as<To>().via(factory).in<Scope>() produces InBuilder
static_assert(
    std::same_as<
        decltype(bind<Interface>()
                     .as<Implementation>()
                     .via(implementation_factory)
                     .in<scope::Singleton>()),
        InBuilder<Interface, Implementation,
                  provider::Factory<Implementation, ImplementationFactory>,
                  scope::Singleton>>);

// ----------------------------------------------------------------------------
// Binding Conversion Tests - Verify final Binding types
// ----------------------------------------------------------------------------

// bind<From>() converts to Binding<From, Transient, Ctor<From>>
static_assert(
    std::same_as<decltype(Binding{bind<Type>()}),
                 Binding<Type, scope::Transient, provider::Ctor<Type>>>);

// .via(factory) converts to
// Binding<From, Transient, Factory<From, decltype(factory)>>
static_assert(std::same_as<decltype(Binding{bind<Type>().via(type_factory)}),
                           Binding<Type, scope::Transient,
                                   provider::Factory<Type, TypeFactory>>>);

// .as<To>() converts to Binding<From, Transient, Ctor<To>>
static_assert(
    std::same_as<
        decltype(Binding{bind<Interface>().as<Implementation>()}),
        Binding<Interface, scope::Transient, provider::Ctor<Implementation>>>);

// .as<To>().via(factory) converts to Binding<From, Transient, Factory<To, F>>
static_assert(
    std::same_as<
        decltype(Binding{bind<Interface>().as<Implementation>().via(
            implementation_factory)}),
        Binding<Interface, scope::Transient,
                provider::Factory<Implementation, ImplementationFactory>>>);

// .in<Singleton>() converts to Binding<From, Singleton, Ctor<From>>
static_assert(
    std::same_as<decltype(Binding{bind<Type>().in<scope::Singleton>()}),
                 Binding<Type, scope::Singleton, provider::Ctor<Type>>>);

// .in<Transient>() converts to Binding<From, Transient, Ctor<From>>
static_assert(
    std::same_as<decltype(Binding{bind<Type>().in<scope::Transient>()}),
                 Binding<Type, scope::Transient, provider::Ctor<Type>>>);

// .as<To>().in<Scope>() converts to Binding<From, Scope, Ctor<To>>
static_assert(
    std::same_as<
        decltype(Binding{
            bind<Interface>().as<Implementation>().in<scope::Transient>()}),
        Binding<Interface, scope::Transient, provider::Ctor<Implementation>>>);

// .as<To>().via(f).in<Scope>() converts to Binding<From, Scope, Factory<To, F>>
static_assert(
    std::same_as<
        decltype(Binding{bind<Interface>()
                             .as<Implementation>()
                             .via(implementation_factory)
                             .in<scope::Singleton>()}),
        Binding<Interface, scope::Singleton,
                provider::Factory<Implementation, ImplementationFactory>>>);

// .to(instance) converts to Binding<From, scope::Instance,
// provider::External<T>>
static_assert([]() constexpr {
  Instance inst;
  return std::same_as<
      decltype(Binding{bind<Instance>().to(inst)}),
      Binding<Instance, scope::Instance, provider::External<Instance>>>;
}());

// ----------------------------------------------------------------------------
// Exhaustive Binding Type Tests
// ----------------------------------------------------------------------------

// All Ctor combinations with scopes
// ----------------------------------------------------------------------------

// Transient.
static_assert(
    std::same_as<decltype(Binding{bind<Type>().in<scope::Transient>()}),
                 Binding<Type, scope::Transient, provider::Ctor<Type>>>);

// Singleton.
static_assert(
    std::same_as<decltype(Binding{bind<Type>().in<scope::Singleton>()}),
                 Binding<Type, scope::Singleton, provider::Ctor<Type>>>);

// All Factory combinations with scopes
// ----------------------------------------------------------------------------

// Direct .via() with Transient.
static_assert(
    std::same_as<
        decltype(Binding{
            bind<Type>().via(type_factory).in<scope::Transient>()}),
        Binding<Type, scope::Transient, provider::Factory<Type, TypeFactory>>>);

// Direct .via() with Singleton.
static_assert(
    std::same_as<
        decltype(Binding{
            bind<Type>().via(type_factory).in<scope::Singleton>()}),
        Binding<Type, scope::Singleton, provider::Factory<Type, TypeFactory>>>);

// .as<>().via() with Transient.
static_assert(
    std::same_as<
        decltype(Binding{bind<Interface>()
                             .as<Implementation>()
                             .via(implementation_factory)
                             .in<scope::Transient>()}),
        Binding<Interface, scope::Transient,
                provider::Factory<Implementation, ImplementationFactory>>>);

// .as<>().via() with Singleton.
static_assert(
    std::same_as<
        decltype(Binding{bind<Interface>()
                             .as<Implementation>()
                             .via(implementation_factory)
                             .in<scope::Singleton>()}),
        Binding<Interface, scope::Singleton,
                provider::Factory<Implementation, ImplementationFactory>>>);

// ----------------------------------------------------------------------------
// FromType Alias Test
// ----------------------------------------------------------------------------

static_assert(
    std::same_as<
        Binding<Type, scope::Transient, provider::Ctor<Type>>::FromType, Type>);

static_assert(
    std::same_as<
        Binding<Type, scope::Singleton, provider::Ctor<Type>>::FromType, Type>);

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

// ----------------------------------------------------------------------------
// Documentation Examples
// ----------------------------------------------------------------------------

// Verify the examples from the Entry Point comments work.
struct DocumentationExamples : Test {};

TEST_F(DocumentationExamples, ExampleUsage) {
  Instance inst;
  auto factory = []() { return Implementation{}; };

  // Each line from the documentation should compile verbatim.
  [[maybe_unused]] auto b1 = Binding{bind<Type>()};
  [[maybe_unused]] auto b2 = Binding{bind<Type>().via(type_factory)};
  [[maybe_unused]] auto b3 = Binding{bind<Interface>().as<Implementation>()};
  [[maybe_unused]] auto b4 =
      Binding{bind<Interface>().as<Implementation>().via(factory)};
  [[maybe_unused]] auto b5 = Binding{bind<Interface>().in<scope::Singleton>()};
  [[maybe_unused]] auto b6 = Binding{bind<Instance>().to(inst)};
}

}  // namespace
}  // namespace dink
