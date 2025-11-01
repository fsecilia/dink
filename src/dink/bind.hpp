// \file
// Copyright (c) 2025 Frank Secilia
// SPDX-License-Identifier: MIT
//
// \brief Defines binding triples and a fluent api to produce them.
//
// This module uses a type-state DSL to encode rules for generating valid
// bindings; not all {scope, provider} pairs are useful, so the DSL guides
// construction to useful combinations.

// State Graph Mermaid Chart
// ----------------------------------------------------------------------------
// stateDiagram-v2
//     [*] --> BindBuilder : bind<From>()
//
//     BindBuilder --> AsBuilder : .as<To>()
//     BindBuilder --> InBuilder : .in<Scope>()
//     BindBuilder --> ToBuilder : .to(instance)
//     BindBuilder --> [*] : (implicit conversion to Binding)
//
//     AsBuilder --> [*] : (implicit conversion to Binding)
//     AsBuilder --> ViaBuilder : .via(factory)
//     AsBuilder --> InBuilder : .in<Scope>()
//
//     ViaBuilder --> [*] : (implicit conversion to Binding)
//     ViaBuilder --> InBuilder : .in<Scope>()
//
//     ToBuilder --> [*] : (implicit conversion to Binding)
//     InBuilder --> [*] : (implicit conversion to Binding)

#pragma once

#include <dink/lib.hpp>
#include <dink/binding.hpp>
#include <dink/provider.hpp>
#include <dink/scope.hpp>
#include <utility>

namespace dink {

template <typename From>
class BindBuilder;

template <typename From, typename To>
class AsBuilder;

template <typename From, typename To, typename Factory>
class ViaBuilder;

template <typename From, typename InstanceType>
class ToBuilder;

template <typename From, typename To, typename Provider, typename Scope>
class InBuilder;

// ----------------------------------------------------------------------------
// BindBuilder
// ----------------------------------------------------------------------------

//! Initial state after bind<From>().
template <typename From>
class BindBuilder {
 public:
  // Specify To type (From -> To mapping)
  template <typename To>
  constexpr auto as() && -> AsBuilder<From, To> {
    static_assert(!std::is_abstract_v<To>);
    return {};
  }

  // Bind to instance reference (terminal for scope)
  template <typename Instance>
  constexpr auto to(Instance& instance) && -> ToBuilder<From, Instance> {
    return ToBuilder<From, Instance>{instance};
  }

  // Specify scope with Ctor<From> provider
  template <typename Scope>
  constexpr auto in() && -> InBuilder<From, From, provider::Ctor<From>, Scope> {
    return {};
  }

  // Default conversion: Transient<Ctor<From>>
  constexpr
  operator Binding<From, scope::Transient, provider::Ctor<From>>() && {
    return {};
  }
};

// ----------------------------------------------------------------------------
// AsBuilder
// ----------------------------------------------------------------------------

//! State after .as<To>().
template <typename From, typename To>
class AsBuilder {
 public:
  // Specify factory callable
  template <typename Factory>
  constexpr auto via(Factory factory) && -> ViaBuilder<From, To, Factory> {
    return ViaBuilder<From, To, Factory>{std::move(factory)};
  }

  // Specify scope with Ctor<To> provider
  template <typename Scope>
  constexpr auto in() && -> InBuilder<From, To, provider::Ctor<To>, Scope> {
    return InBuilder<From, To, provider::Ctor<To>, Scope>{provider::Ctor<To>{}};
  }

  // Default conversion: Transient<Ctor<To>>
  constexpr operator Binding<From, scope::Transient, provider::Ctor<To>>() && {
    return {};
  }
};

// ----------------------------------------------------------------------------
// ViaBuilder
// ----------------------------------------------------------------------------

//! State after .as<To>().via(factory).
template <typename From, typename To, typename Factory>
class ViaBuilder {
 public:
  // Specify scope with Factory<To, Factory> provider
  template <typename Scope>
  constexpr auto
  in() && -> InBuilder<From, To, provider::Factory<To, Factory>, Scope> {
    return InBuilder<From, To, provider::Factory<To, Factory>, Scope>{
        provider::Factory<To, Factory>{std::move(factory_)}};
  }

  // Default conversion: Transient<Factory<To, Factory>>
  constexpr operator Binding<From, scope::Transient,
                             provider::Factory<To, Factory>>() && {
    return {{}, provider::Factory<To, Factory>{std::move(factory_)}};
  }

  explicit constexpr ViaBuilder(Factory factory) noexcept
      : factory_{std::move(factory)} {}

 private:
  Factory factory_;
};

// ----------------------------------------------------------------------------
// ToBuilder
// ----------------------------------------------------------------------------

//! State after .to(instance) - Terminal
template <typename From, typename InstanceType>
class ToBuilder {
 public:
  // Conversion using scope::External with provider::External
  constexpr operator Binding<From, scope::Instance,
                             provider::External<InstanceType>>() && {
    return {scope::Instance{}, provider::External<InstanceType>{instance_}};
  }

  explicit constexpr ToBuilder(InstanceType& instance) noexcept
      : instance_{instance} {}

 private:
  InstanceType& instance_;
};

// ----------------------------------------------------------------------------
// InBuilder
// ----------------------------------------------------------------------------

//! State after .in<Scope>() - Terminal
template <typename From, typename To, typename Provider, typename Scope>
class InBuilder {
 public:
  // Only conversion available - explicit scope specified
  constexpr operator Binding<From, Scope, Provider>() && {
    return {{}, std::move(provider_)};
  }

  explicit constexpr InBuilder(Provider provider) noexcept
      : provider_{std::move(provider)} {}

  constexpr InBuilder() = default;

 private:
  Provider provider_;
};

// ----------------------------------------------------------------------------
// Deduction Guides
// ----------------------------------------------------------------------------

template <typename From>
Binding(BindBuilder<From>&&)
    -> Binding<From, scope::Transient, provider::Ctor<From>>;

template <typename From, typename To>
Binding(AsBuilder<From, To>&&)
    -> Binding<From, scope::Transient, provider::Ctor<To>>;

template <typename From, typename To, typename Factory>
Binding(ViaBuilder<From, To, Factory>&&)
    -> Binding<From, scope::Transient, provider::Factory<To, Factory>>;

template <typename From, typename InstanceType>
Binding(ToBuilder<From, InstanceType>&&)
    -> Binding<From, scope::Instance, provider::External<InstanceType>>;

template <typename From, typename To, typename Provider, typename Scope>
Binding(InBuilder<From, To, Provider, Scope>&&)
    -> Binding<From, Scope, Provider>;

// ----------------------------------------------------------------------------
// Factory Functions
// ----------------------------------------------------------------------------

//! Creates a tuple of bindings by forcing conversion of each builder.
//
// This explicitly constructs Binding objects from builders, which enables
// storing heterogeneous bindings in a tuple. Without this, std::make_tuple
// would store the builder types themselves rather than converting them to
// Binding types.
//
// Example usage:
//   auto bindings = make_bindings(
//     bind<Type>(),
//     bind<Interface>().as<Implementation>().in<scope::Singleton>(),
//     bind<Config>().to(config_instance)
//   );
template <typename... Builders>
constexpr auto make_bindings(Builders&&... builders) {
  return std::make_tuple(Binding{std::forward<Builders>(builders)}...);
}

// ----------------------------------------------------------------------------
// Entry Point
// ----------------------------------------------------------------------------

//! Creates a binding configuration builder for type From.
//
// This is the entry point for the fluent binding API. Returns a builder
// that can be configured through method chaining.
//
// Example usage:
//   bind<Interface>() -> Transient<Ctor<Interface>>
//   bind<Interface>().as<Implementation>() -> Transient<Ctor<Implementation>>
//   bind<Interface>().as<Implementation>().via(factory)
//     -> Transient<Factory<Implementation, decltype(factory)>>
//   bind<Interface>().in<scope::Singleton>() -> Singleton<Ctor<Interface>>
//   bind<Interface>().to(instance) -> Instance<decltype(instance)>
template <typename From>
constexpr auto bind() -> BindBuilder<From> {
  return {};
}

// ----------------------------------------------------------------------------
// Concepts
// ----------------------------------------------------------------------------

namespace traits {

template <typename>
struct IsBinding : std::false_type {};

template <typename From>
struct IsBinding<BindBuilder<From>> : std::true_type {};

template <typename From, typename To>
struct IsBinding<AsBuilder<From, To>> : std::true_type {};

template <typename From, typename To, typename Factory>
struct IsBinding<ViaBuilder<From, To, Factory>> : std::true_type {};

template <typename From, typename InstanceType>
struct IsBinding<ToBuilder<From, InstanceType>> : std::true_type {};

template <typename From, typename To, typename Provider, typename Scope>
struct IsBinding<InBuilder<From, To, Provider, Scope>> : std::true_type {};

template <typename From, typename Scope, typename Provider>
struct IsBinding<Binding<From, Scope, Provider>> : std::true_type {};

template <typename Binding>
inline constexpr auto is_binding = IsBinding<Binding>::value;

}  // namespace traits

template <typename Binding>
concept IsBinding = traits::is_binding<std::remove_cvref_t<Binding>>;

}  // namespace dink
