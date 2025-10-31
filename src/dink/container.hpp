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

//! Identifies valid container types
//
// A container's primary function is to resolve instances of the requested
// type.
template <typename Container>
concept IsContainer = requires(Container& container) {
  {
    container.template resolve<meta::ConceptProbe>()
  } -> std::same_as<meta::ConceptProbe>;
};

//! Identifies types valid for parent container parameters.
//
// Parent container parameters can be a container or void.
template <typename Container>
concept IsParentContainer =
    IsContainer<Container> || std::same_as<Container, void>;

//! Identifies types valid for tag parameters.
//
// Given the way deduction works, the tag type cannot be a binding, config,
// or another container, or deducing a container type becomes ambiguous.
template <typename Tag>
concept IsTag = !IsBinding<Tag> && !IsConfig<Tag> && !IsContainer<Tag>;

//! Identifies types valid for tag arguments.
//
// The actual argument cannot be instnatiated as void.
template <typename Tag>
concept IsTagArg = IsTag<Tag> && !std::same_as<Tag, void>;

// ----------------------------------------------------------------------------
// Container
// ----------------------------------------------------------------------------

//! Hierarchical DI Container
//
// Container is the user-facing facade that contains a config, dispatcher, and
// optional parent. It presents a resolve() method that can construct any
// constructible type.
//
// By default, values are constructed on the fly, transiently. They can be
// configured to be cached in the container by binding them in a config.
//
// Regardless of configuration, requests for values, rvalue references, and
// unique_ptrs always produce new instances. Requests for lvalue references,
// pointers, or weak_ptrs always produce a value cached by the container.
// Requests for shared_ptrs produce new instances, unless their element type is
// configured to return references, in which case they alias the managed
// reference.
//
// In general, it should work intuitively. If you ask for a value, you get a
// value. If you ask for a reference, you get a cached reference. The rest are
// details.
//
// This type supports optional tagging. Two containers with the same config
// have the same type. Because caches are keyed by <container, provider>, two
// containers with the same config will share caches. A tag can be used to
// distinguish between two otherwise identical container types. By specifying a
// tag, the caches can be separated.
//
// Generally, if you need a tag, the specific tag type is unimportant as long
// as it is unique. In this case, you can use meta::UniqueType<>.
// dink_unique_container() simplifies this definition.
template <IsTag Tag, IsConfig Config, typename Dispatcher,
          IsParentContainer Parent = void>
class Container;

//! Partial specialization where Parent = void produces a root container.
template <IsTag Tag, IsConfig Config, typename Dispatcher>
class Container<Tag, Config, Dispatcher, void> {
 public:
  Container() noexcept = default;

  //! Construct from bindings.
  template <IsBinding... Bindings>
  explicit Container(Bindings&&... bindings) noexcept
      : Container{Config{std::forward<Bindings>(bindings)...}, Dispatcher{}} {}

  //! Construct from bindings.
  template <IsTagArg ActualTag, IsBinding... Bindings>
  explicit Container(ActualTag, Bindings&&... bindings) noexcept
      : Container{Config{std::forward<Bindings>(bindings)...}, Dispatcher{}} {}

  //! Construct from components.
  Container(Config config, Dispatcher dispatcher) noexcept
      : dispatcher_{std::move(dispatcher)}, config_{std::move(config)} {}

  //! Construct from components with tag.
  template <IsTagArg ActualTag>
  Container(ActualTag, Config config, Dispatcher dispatcher) noexcept
      : Container{std::move(config), std::move(dispatcher)} {}

  Container(const Container&) = delete;
  auto operator=(const Container&) const -> Container& = delete;
  Container(Container&&) = default;
  auto operator=(Container&&) const -> Container& = default;

  //! Resolve a dependency.
  template <typename Requested>
  auto resolve() -> remove_rvalue_ref_t<Requested> {
    return dispatcher_.template resolve<Requested>(*this, config_, nullptr);
  }

 private:
  [[dink_no_unique_address]] Dispatcher dispatcher_{};
  Config config_{};
};

//! No specialization produces a child container.
template <IsTag Tag, IsConfig Config, typename Dispatcher,
          IsParentContainer Parent>
class Container {
 public:
  //! Construct from parent only.
  explicit Container(Parent& parent) noexcept
      : Container{parent, Config{}, Dispatcher{}} {}

  //! Construct from parent and bindings.
  template <IsBinding... Bindings>
  explicit Container(Parent& parent, Bindings&&... bindings) noexcept
      : Container{parent, Config{std::forward<Bindings>(bindings)...},
                  Dispatcher{}} {}

  //! Construct from parent and bindings with tag.
  template <IsTagArg ActualTag, IsBinding... Bindings>
  explicit Container(ActualTag, Parent& parent, Bindings&&... bindings) noexcept
      : Container{parent, Config{std::forward<Bindings>(bindings)...},
                  Dispatcher{}} {}

  //! Construct from parent and components.
  Container(Parent& parent, Config config, Dispatcher dispatcher) noexcept
      : dispatcher_{std::move(dispatcher)},
        config_{std::move(config)},
        parent_{&parent} {}

  //! Construct from parent and components with tag.
  template <IsTagArg ActualTag>
  Container(ActualTag, Parent& parent, Config config,
            Dispatcher dispatcher) noexcept
      : Container{parent, std::move(config), std::move(dispatcher)} {}

  Container(const Container&) = delete;
  auto operator=(const Container&) const -> Container& = delete;
  Container(Container&&) = default;
  auto operator=(Container&&) const -> Container& = default;

  //! Resolve a dependency.
  template <typename Requested>
  auto resolve() -> remove_rvalue_ref_t<Requested> {
    return dispatcher_.template resolve<Requested>(*this, config_, parent_);
  }

 private:
  [[dink_no_unique_address]] Dispatcher dispatcher_{};
  Config config_{};
  Parent* parent_{};
};

// ----------------------------------------------------------------------------
// Deduction Guides
// ----------------------------------------------------------------------------

//! Intercepted copy constructor.
//
// Containers are move-only. Trying to copy a container creates a child.
template <IsTag Tag, IsConfig Config, typename Dispatcher,
          IsParentContainer Parent>
Container(Container<Tag, Config, Dispatcher, Parent>&)
    -> Container<Tag, dink::Config<>, dink::Dispatcher<>,
                 Container<Tag, Config, Dispatcher, Parent>>;

//! Root container from builders.
template <IsBinding... Builders>
Container(Builders&&...)
    -> Container<void, decltype(Config{std::declval<Builders>()...}),
                 Dispatcher<>, void>;

//! Root container from builders with tag.
template <IsTagArg Tag, IsBinding... Builders>
Container(Tag, Builders&&...)
    -> Container<Tag, decltype(Config{std::declval<Builders>()...}),
                 Dispatcher<>, void>;

//! Child container from parent.
template <IsParentContainer Parent>
Container(Parent& parent)
    -> Container<void, Config<>, Dispatcher<>, std::remove_cvref_t<Parent>>;

//! Child container from parent with tag.
template <IsTagArg Tag, IsParentContainer Parent>
Container(Tag, Parent& parent)
    -> Container<Tag, Config<>, Dispatcher<>, std::remove_cvref_t<Parent>>;

//! Child container from parent and bindings.
template <IsParentContainer Parent, IsBinding... Builders>
Container(Parent& parent, Builders&&...)
    -> Container<void, decltype(Config{std::declval<Builders>()...}),
                 Dispatcher<>, std::remove_cvref_t<Parent>>;

//! Child container from parent and bindings with tag.
template <IsTagArg Tag, IsParentContainer Parent, IsBinding... Builders>
Container(Tag, Parent& parent, Builders&&...)
    -> Container<Tag, decltype(Config{std::declval<Builders>()...}),
                 Dispatcher<>, std::remove_cvref_t<Parent>>;

// ----------------------------------------------------------------------------
// Factory Functions
// ----------------------------------------------------------------------------

//! Macro to generate unique container types.
//
// This is a macro so unique_type<> is generated once at each call site. There
// is no other way in the language to generate this once per instance. All
// other methods generate it once per type.
#define dink_unique_container(...) \
  Container {                      \
    ::dink::meta::UniqueType<> {}  \
    __VA_OPT__(, ) __VA_ARGS__     \
  }

}  // namespace dink
