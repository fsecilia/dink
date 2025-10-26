// \file
// Copyright (c) 2025 Frank Secilia
// SPDX-License-Identifier: MIT

#pragma once

#include <dink/lib.hpp>
#include <dink/provider.hpp>
#include <dink/scope.hpp>
#include <utility>

namespace dink {

// Forward declarations
template <typename From>
class BindBuilder;

template <typename From, typename To>
class AsBuilder;

template <typename From, typename To, typename Factory>
class ViaBuilder;

template <typename From, typename InstanceType>
class ToBuilder;

template <typename From, typename To, typename Provider,
          template <typename> typename ScopeTemplate>
class InBuilder;

// ----------------------------------------------------------------------------
// Binding - Final type stored in container
// ----------------------------------------------------------------------------

template <typename From, typename Scope>
struct Binding {
  using FromType = From;
  Scope scope;

  explicit constexpr Binding(Scope scope) noexcept : scope{std::move(scope)} {}
};

// ----------------------------------------------------------------------------
// BindBuilder - Initial state after bind<From>()
// ----------------------------------------------------------------------------

template <typename From>
class BindBuilder {
 public:
  // Specify To type (From -> To mapping)
  template <typename To>
  constexpr auto as() && -> AsBuilder<From, To> {
    return {};
  }

  // Bind to instance reference (terminal for scope)
  template <typename Instance>
  constexpr auto to(Instance& instance) && -> ToBuilder<From, Instance> {
    return ToBuilder<From, Instance>{instance};
  }

  // Specify scope with Ctor<From> provider
  template <template <typename> typename ScopeTemplate>
  constexpr auto
  in() && -> InBuilder<From, From, provider::Ctor<From>, ScopeTemplate> {
    return InBuilder<From, From, provider::Ctor<From>, ScopeTemplate>{
        provider::Ctor<From>{}};
  }

  // Default conversion: Deduced<Ctor<From>>
  constexpr operator Binding<From, scope::Deduced<provider::Ctor<From>>>() && {
    return Binding<From, scope::Deduced<provider::Ctor<From>>>{
        scope::Deduced{provider::Ctor<From>{}}};
  }
};

// ----------------------------------------------------------------------------
// AsBuilder - After .as<To>()
// ----------------------------------------------------------------------------

template <typename From, typename To>
class AsBuilder {
 public:
  // Specify factory callable
  template <typename Factory>
  constexpr auto via(Factory factory) && -> ViaBuilder<From, To, Factory> {
    return ViaBuilder<From, To, Factory>{std::move(factory)};
  }

  // Specify scope with Ctor<To> provider
  template <template <typename> typename ScopeTemplate>
  constexpr auto
  in() && -> InBuilder<From, To, provider::Ctor<To>, ScopeTemplate> {
    return InBuilder<From, To, provider::Ctor<To>, ScopeTemplate>{
        provider::Ctor<To>{}};
  }

  // Default conversion: Deduced<Ctor<To>>
  constexpr operator Binding<From, scope::Deduced<provider::Ctor<To>>>() && {
    return Binding<From, scope::Deduced<provider::Ctor<To>>>{
        scope::Deduced{provider::Ctor<To>{}}};
  }
};

// ----------------------------------------------------------------------------
// ViaBuilder - After .as<To>().via(factory)
// ----------------------------------------------------------------------------

template <typename From, typename To, typename Factory>
class ViaBuilder {
 public:
  // Specify scope with Factory<To, Factory> provider
  template <template <typename> typename ScopeTemplate>
  constexpr auto in() && -> InBuilder<From, To, provider::Factory<To, Factory>,
                                      ScopeTemplate> {
    return InBuilder<From, To, provider::Factory<To, Factory>, ScopeTemplate>{
        provider::Factory<To, Factory>{std::move(factory_)}};
  }

  // Default conversion: Deduced<Factory<To, Factory>>
  constexpr
  operator Binding<From, scope::Deduced<provider::Factory<To, Factory>>>() && {
    return Binding<From, scope::Deduced<provider::Factory<To, Factory>>>{
        scope::Deduced{provider::Factory<To, Factory>{std::move(factory_)}}};
  }

  explicit constexpr ViaBuilder(Factory factory) noexcept
      : factory_{std::move(factory)} {}

 private:
  Factory factory_;
};

// ----------------------------------------------------------------------------
// ToBuilder - After .to(instance) - Terminal state
// ----------------------------------------------------------------------------

template <typename From, typename InstanceType>
class ToBuilder {
 public:
  // Only conversion available - Instance scope is implicit
  constexpr operator Binding<From, scope::Instance<InstanceType>>() && {
    return Binding<From, scope::Instance<InstanceType>>{
        scope::Instance<InstanceType>{instance_}};
  }

  explicit constexpr ToBuilder(InstanceType& instance) noexcept
      : instance_{instance} {}

 private:
  InstanceType& instance_;
};

// ----------------------------------------------------------------------------
// InBuilder - After .in<Scope>() - Terminal state
// ----------------------------------------------------------------------------

template <typename From, typename To, typename Provider,
          template <typename> typename ScopeTemplate>
class InBuilder {
 public:
  // Only conversion available - explicit scope specified
  constexpr operator Binding<From, ScopeTemplate<Provider>>() && {
    return Binding<From, ScopeTemplate<Provider>>{
        ScopeTemplate<Provider>{std::move(provider_)}};
  }

  explicit constexpr InBuilder(Provider provider) noexcept
      : provider_{std::move(provider)} {}

 private:
  Provider provider_;
};

// ----------------------------------------------------------------------------
// Deduction Guides - Enable CTAD for Binding construction
// ----------------------------------------------------------------------------

// Deduction guide for BindBuilder
template <typename From>
Binding(BindBuilder<From>&&)
    -> Binding<From, scope::Deduced<provider::Ctor<From>>>;

// Deduction guide for AsBuilder
template <typename From, typename To>
Binding(AsBuilder<From, To>&&)
    -> Binding<From, scope::Deduced<provider::Ctor<To>>>;

// Deduction guide for ViaBuilder
template <typename From, typename To, typename Factory>
Binding(ViaBuilder<From, To, Factory>&&)
    -> Binding<From, scope::Deduced<provider::Factory<To, Factory>>>;

// Deduction guide for ToBuilder
template <typename From, typename InstanceType>
Binding(ToBuilder<From, InstanceType>&&)
    -> Binding<From, scope::Instance<InstanceType>>;

// Deduction guide for InBuilder
template <typename From, typename To, typename Provider,
          template <typename> typename ScopeTemplate>
Binding(InBuilder<From, To, Provider, ScopeTemplate>&&)
    -> Binding<From, ScopeTemplate<Provider>>;

// ----------------------------------------------------------------------------
// Helper Functions
// ----------------------------------------------------------------------------

//! Creates a tuple of bindings by forcing conversion of each builder.
//
// This helper explicitly constructs Binding objects from builders, which
// enables storing heterogeneous bindings in a tuple. Without this,
// std::make_tuple would store the builder types themselves rather than
// converting them to Binding types.
//
// Example usage:
//   auto bindings = make_bindings(
//     bind<IFoo>(),
//     bind<IBar>().as<Bar>().in<scope::Singleton>(),
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
//   bind<IFoo>()                              // Deduced<Ctor<IFoo>>
//   bind<IFoo>().as<Foo>()                    // Deduced<Ctor<Foo>>
//   bind<IFoo>().as<Foo>().via(factory)       // Deduced<Factory<Foo, F>>
//   bind<IFoo>().in<scope::Singleton>()       // Singleton<Ctor<IFoo>>
//   bind<IFoo>().to(instance)                 // Instance<decltype(instance)>
template <typename From>
constexpr auto bind() -> BindBuilder<From> {
  return {};
}

// ----------------------------------------------------------------------------
// Concepts
// ----------------------------------------------------------------------------

namespace detail {

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

template <typename From, typename To, typename Provider,
          template <typename> typename ScopeTemplate>
struct IsBinding<InBuilder<From, To, Provider, ScopeTemplate>>
    : std::true_type {};

template <typename From, typename Scope>
struct IsBinding<Binding<From, Scope>> : std::true_type {};

}  // namespace detail

template <typename T>
concept IsBinding = detail::IsBinding<std::remove_cvref_t<T>>::value;

}  // namespace dink
