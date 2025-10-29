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

// ----------------------------------------------------------------------------
// Factory Functions
// ----------------------------------------------------------------------------

//! Macro to generate unique container types.
//
// Two containers with the same config have the same type. Because singletons
// are keyed by <container, provider>, two containers with the same config will
// share singletons, which is quite astonishing. There is no internal
// workaround - the container type must be made unique - dink_unique_container()
// creates a container with a tag unique to the instance.
//
// This is a macro so unique_type<> is generated once at each call site. There
// is no other way in the language to generate this once per instance. All
// other methods generate it once per type.
#define dink_unique_container(...) \
  Container { unique_type<>{}, __VA_ARGS__ }

}  // namespace dink
