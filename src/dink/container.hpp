// \file
// Copyright (c) 2025 Frank Secilia
// SPDX-License-Identifier: MIT

#pragma once

#include <dink/lib.hpp>
#include <dink/binding.hpp>
#include <dink/config.hpp>
#include <dink/dispatcher.hpp>
#include <dink/meta.hpp>
#include <dink/remove_rvalue_ref.hpp>

namespace dink {

// ----------------------------------------------------------------------------
// Concepts
// ----------------------------------------------------------------------------

template <typename Container>
concept IsContainer = requires(Container& container) {
  {
    container.template resolve<meta::ConceptProbe>()
  } -> std::same_as<meta::ConceptProbe>;
};

// ----------------------------------------------------------------------------
// unique_type - Factory for generating unique lambda-based tags
// ----------------------------------------------------------------------------

//! Generates a unique tag using a lambda's closure type
//
// Each call returns a different lambda with a unique closure type. This
// provides compile-time uniqueness without macros or non-standard features.
template <typename impl = decltype([] {})>
using unique_type = impl;

// ----------------------------------------------------------------------------
// Container - Forward Declaration
// ----------------------------------------------------------------------------

template <IsConfig Config, typename Dispatcher, typename Parent = void,
          typename Tag = unique_type<>>
class Container;

// ----------------------------------------------------------------------------
// Container - Root Specialization (Parent = void)
// ----------------------------------------------------------------------------

template <IsConfig Config, typename Dispatcher, typename Tag>
class Container<Config, Dispatcher, void, Tag> {
  Config config_{};
  [[no_unique_address]] Dispatcher dispatcher_{};

 public:
  //! Resolve a dependency
  template <typename Requested>
  auto resolve() -> remove_rvalue_ref_t<Requested> {
    return dispatcher_.template resolve<Requested>(*this, config_, nullptr);
  }

  //! Construct from bindings (no tag)
  template <IsBinding... Bindings>
  explicit Container(Bindings&&... bindings) noexcept
      : config_{make_bindings(std::forward<Bindings>(bindings)...)} {}

  //! Construct from config directly (no tag)
  explicit Container(Config config) noexcept : config_{std::move(config)} {}

  //! Construct from config and dispatcher (for testing)
  explicit Container(Config config, Dispatcher dispatcher) noexcept
      : config_{std::move(config)}, dispatcher_{std::move(dispatcher)} {}
};

// ----------------------------------------------------------------------------
// Container - Child Specialization (Parent != void)
// ----------------------------------------------------------------------------

template <IsConfig Config, typename Dispatcher, typename Parent, typename Tag>
class Container {
  Config config_{};
  Parent* parent_;
  [[no_unique_address]] Dispatcher dispatcher_{};

 public:
  //! Resolve a dependency
  template <typename Requested>
  auto resolve() -> remove_rvalue_ref_t<Requested> {
    return dispatcher_.template resolve<Requested>(*this, config_, parent_);
  }

  //! Construct from parent and bindings (no tag - default behavior)
  template <IsBinding... Bindings>
  explicit Container(Parent& parent, Bindings&&... bindings) noexcept
      : config_{make_bindings(std::forward<Bindings>(bindings)...)},
        parent_{&parent} {}

  //! Construct from tag, parent, and bindings (explicit unique tag)
  template <typename UniqueTag, IsBinding... Bindings>
    requires(!IsContainer<UniqueTag>)
  explicit Container(UniqueTag, Parent& parent, Bindings&&... bindings) noexcept
      : config_{make_bindings(std::forward<Bindings>(bindings)...)},
        parent_{&parent} {}

  //! Construct from parent and config directly (no tag)
  explicit Container(Parent& parent, Config config) noexcept
      : config_{std::move(config)}, parent_{&parent} {}

  //! Construct from tag, parent, and config (explicit unique tag)
  template <typename UniqueTag>
  explicit Container(UniqueTag, Parent& parent, Config config) noexcept
      : config_{std::move(config)}, parent_{&parent} {}

  //! Construct from parent, config, and dispatcher (for testing)
  explicit Container(Parent& parent, Config config,
                     Dispatcher dispatcher) noexcept
      : config_{std::move(config)},
        parent_{&parent},
        dispatcher_{std::move(dispatcher)} {}
};

// ----------------------------------------------------------------------------
// Deduction Guides
// ----------------------------------------------------------------------------

// Root container from builders
template <IsBinding... Builders>
Container(Builders&&...)
    -> Container<detail::ConfigFromTuple<
                     decltype(make_bindings(std::declval<Builders>()...))>,
                 Dispatcher<>, void>;

// Root container from config
template <IsConfig ConfigType>
Container(ConfigType) -> Container<ConfigType, Dispatcher<>, void>;

// Child container from parent and builders (no tag - default sharing)
template <IsContainer ParentContainer, IsBinding... Builders>
Container(ParentContainer&, Builders&&...)
    -> Container<detail::ConfigFromTuple<
                     decltype(make_bindings(std::declval<Builders>()...))>,
                 Dispatcher<>, ParentContainer, unique_type<>>;

// Child container from tag, parent, and builders (explicit unique)
template <typename UniqueTag, IsContainer ParentContainer,
          IsBinding... Builders>
Container(UniqueTag, ParentContainer&, Builders&&...)
    -> Container<detail::ConfigFromTuple<
                     decltype(make_bindings(std::declval<Builders>()...))>,
                 Dispatcher<>, ParentContainer, UniqueTag>;

// Child container from parent and config (no tag)
template <IsContainer ParentContainer, IsConfig ConfigType>
Container(ParentContainer&, ConfigType)
    -> Container<ConfigType, Dispatcher<>, ParentContainer, unique_type<>>;

// Child container from tag, parent, and config (explicit unique)
template <typename UniqueTag, IsContainer ParentContainer, IsConfig ConfigType>
Container(UniqueTag, ParentContainer&, ConfigType)
    -> Container<ConfigType, Dispatcher<>, ParentContainer, UniqueTag>;

// ----------------------------------------------------------------------------
// Factory Functions
// ----------------------------------------------------------------------------

//! Root container, no parent.
//
// This produces a container with a unique tag every time.
template <typename... Builders>
auto make_container(Builders&&... builders) {
  using Config = detail::ConfigFromTuple<decltype(make_bindings(
      std::declval<Builders>()...))>;
  return Container<Config, Dispatcher<>, void, unique_type<>>{
      std::forward<Builders>(builders)...};
}

//! Child container, first arg is parent.
//
// This produces a container with a unique tag every time.
template <IsContainer Parent, typename... Builders>
auto make_container(Parent& parent, Builders&&... builders) {
  using Config = detail::ConfigFromTuple<decltype(make_bindings(
      std::declval<Builders>()...))>;
  return Container<Config, Dispatcher<>, Parent, unique_type<>>{
      parent, std::forward<Builders>(builders)...};
}
}  // namespace dink
