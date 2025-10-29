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
template <typename Impl = decltype([] {})>
struct UniqueType {};

namespace traits {

template <typename UniqueType>
struct IsUniqueType : std::false_type {};

template <typename Impl>
struct IsUniqueType<UniqueType<Impl>> : std::true_type {};

template <typename UniqueType>
inline constexpr auto is_unique_type = IsUniqueType<UniqueType>::value;

}  // namespace traits

template <typename UniqueType>
concept IsUniqueType = traits::is_unique_type<UniqueType>;

// ----------------------------------------------------------------------------
// Container - Forward Declaration
// ----------------------------------------------------------------------------

template <IsConfig Config, typename Dispatcher, typename Parent = void,
          typename Tag = UniqueType<>>
class Container;

// ----------------------------------------------------------------------------
// Container - Root Specialization (Parent = void)
// ----------------------------------------------------------------------------

template <IsConfig Config, typename Dispatcher, typename Tag>
class Container<Config, Dispatcher, void, Tag> {
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

  //! Construct from bindings (with tag)
  template <IsUniqueType UniqueType, IsBinding... Bindings>
  Container(UniqueType, Bindings&&... bindings) noexcept
      : config_{make_bindings(std::forward<Bindings>(bindings)...)} {}

  //! Construct from config directly (no tag)
  explicit Container(Config config) noexcept : config_{std::move(config)} {}

  //! Construct from config and dispatcher (for testing)
  Container(Config config, Dispatcher dispatcher) noexcept
      : dispatcher_{std::move(dispatcher)}, config_{std::move(config)} {}

 private:
  [[no_unique_address]] Dispatcher dispatcher_{};
  Config config_{};
};

// ----------------------------------------------------------------------------
// Container - Child Specialization (Parent != void)
// ----------------------------------------------------------------------------

template <IsConfig Config, typename Dispatcher, typename Parent, typename Tag>
class Container {
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
  template <IsUniqueType UniqueType, IsBinding... Bindings>
    requires(!IsContainer<UniqueType>)
  Container(UniqueType, Parent& parent, Bindings&&... bindings) noexcept
      : config_{make_bindings(std::forward<Bindings>(bindings)...)},
        parent_{&parent} {}

  //! Construct from parent and config directly (no tag)
  Container(Parent& parent, Config config) noexcept
      : config_{std::move(config)}, parent_{&parent} {}

  //! Construct from tag, parent, and config (explicit unique tag)
  template <IsUniqueType UniqueType>
  Container(UniqueType, Parent& parent, Config config) noexcept
      : config_{std::move(config)}, parent_{&parent} {}

  //! Construct from parent, config, and dispatcher.
  Container(Parent& parent, Config config, Dispatcher dispatcher) noexcept
      : dispatcher_{std::move(dispatcher)},
        config_{std::move(config)},
        parent_{&parent} {}

 private:
  [[no_unique_address]] Dispatcher dispatcher_{};
  Config config_{};
  Parent* parent_{};
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

// Root container from builders (with tag)
template <IsUniqueType UniqueType, IsBinding... Builders>
Container(UniqueType, Builders&&...)
    -> Container<detail::ConfigFromTuple<
                     decltype(make_bindings(std::declval<Builders>()...))>,
                 Dispatcher<>, void, UniqueType>;

// Child container from parent and builders (no tag)
template <IsContainer ParentContainer, IsBinding... Builders>
Container(ParentContainer&, Builders&&...)
    -> Container<detail::ConfigFromTuple<
                     decltype(make_bindings(std::declval<Builders>()...))>,
                 Dispatcher<>, ParentContainer>;

// Child container from tag, parent, and builders (with tag)
template <IsUniqueType UniqueType, IsContainer ParentContainer,
          IsBinding... Builders>
Container(UniqueType, ParentContainer&, Builders&&...)
    -> Container<detail::ConfigFromTuple<
                     decltype(make_bindings(std::declval<Builders>()...))>,
                 Dispatcher<>, ParentContainer, UniqueType>;

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
  Container {                      \
    UniqueType<> {}                \
    __VA_OPT__(, ) __VA_ARGS__     \
  }

}  // namespace dink
