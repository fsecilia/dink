// \file
// Copyright (c) 2025 Frank Secilia
// SPDX-License-Identifier: MIT
//
// \brief Defines how types are bound to scopes and providers.

#pragma once

#include <dink/lib.hpp>
#include <dink/provider.hpp>
#include <dink/scope.hpp>
#include <utility>

namespace dink {

//! Binding triple.
//
// Binding triples are combinations of a type and 2 instances,
// <From, Scope, Provider>:
// \tparam From defines what type the binding matches
// \tparam Scope defines how the instances of the type are stored
// \tparam Provider defines how the instances are created or obtained
//
// Since all 3 types can vary, each binding tends to be a unique type.
//
// This is the final type produced by the bind DSL. It is stored in Config.
template <typename From, typename Scope, typename Provider>
struct Binding {
  using FromType = From;
  using ScopeType = Scope;
  using ProviderType = Provider;

  [[dink_no_unique_address]] Scope scope;
  [[dink_no_unique_address]] Provider provider;

  constexpr Binding(Scope scope, Provider provider) noexcept
      : scope{std::move(scope)}, provider{std::move(provider)} {}

  constexpr Binding() = default;
};

// ----------------------------------------------------------------------------
// Concepts
// ----------------------------------------------------------------------------

namespace traits {

template <typename>
struct IsConvertibleToBinding : std::false_type {};

//! Binding is itself always convertible to Binding.
template <typename From, typename Scope, typename Provider>
struct IsConvertibleToBinding<Binding<From, Scope, Provider>> : std::true_type {
};

template <typename Binding>
inline constexpr auto is_convertible_to_binding =
    IsConvertibleToBinding<Binding>::value;

}  // namespace traits

//! Matches types that are or can convert to Binding.
//
// This concept uses an extensible template that can be specialized for
// arbitrary types.
template <typename Binding>
concept IsConvertibleToBinding =
    traits::is_convertible_to_binding<std::remove_cvref_t<Binding>>;

}  // namespace dink
