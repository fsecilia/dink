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
// Container - Forward Declaration
// ----------------------------------------------------------------------------

template <IsConfig Config, typename Dispatcher, typename Parent = void>
class Container;

// ----------------------------------------------------------------------------
// Container - Root Specialization (Parent = void)
// ----------------------------------------------------------------------------

template <IsConfig Config, typename Dispatcher>
class Container<Config, Dispatcher, void> {
  Config config_{};
  [[no_unique_address]] Dispatcher dispatcher_{};

 public:
  //! Resolve a dependency
  template <typename Requested>
  auto resolve() -> remove_rvalue_ref_t<Requested> {
    return dispatcher_.template resolve<Requested>(*this, config_, nullptr);
  }

  //! Construct from bindings
  template <IsBinding... Bindings>
  explicit Container(Bindings&&... bindings) noexcept
      : config_{make_bindings(std::forward<Bindings>(bindings)...)} {}

  //! Construct from config directly
  explicit Container(Config config) noexcept : config_{std::move(config)} {}

  //! Construct from config and dispatcher (for testing)
  explicit Container(Config config, Dispatcher dispatcher) noexcept
      : config_{std::move(config)}, dispatcher_{std::move(dispatcher)} {}
};

// ----------------------------------------------------------------------------
// Container - Child Specialization (Parent != void)
// ----------------------------------------------------------------------------

template <IsConfig Config, typename Dispatcher, typename Parent>
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

  //! Construct from parent and bindings
  template <IsBinding... Bindings>
  explicit Container(Parent& parent, Bindings&&... bindings) noexcept
      : config_{make_bindings(std::forward<Bindings>(bindings)...)},
        parent_{&parent} {}

  //! Construct from parent and config directly
  explicit Container(Parent& parent, Config config) noexcept
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

// Deduction guide - converts builders to Bindings, then deduces Config
template <IsBinding... Builders>
Container(Builders&&...)
    -> Container<detail::ConfigFromTuple<
                     decltype(make_bindings(std::declval<Builders>()...))>,
                 Dispatcher<>, void>;

// Deduction guide for Config directly (root container)
template <IsConfig ConfigType>
Container(ConfigType) -> Container<ConfigType, Dispatcher<>, void>;

// Deduction guide for child container with parent and bindings
template <IsContainer ParentContainer, IsBinding... Builders>
Container(ParentContainer&, Builders&&...)
    -> Container<detail::ConfigFromTuple<
                     decltype(make_bindings(std::declval<Builders>()...))>,
                 Dispatcher<>, ParentContainer>;

// Deduction guide for child container with parent and config
template <IsContainer ParentContainer, IsConfig ConfigType>
Container(ParentContainer&, ConfigType)
    -> Container<ConfigType, Dispatcher<>, ParentContainer>;

}  // namespace dink
