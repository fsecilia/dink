// \file
// Copyright (c) 2025 Frank Secilia
// SPDX-License-Identifier: MIT
//
// \brief User-facing facade presenting the top level .resolve().

#pragma once

#include <dink/lib.hpp>
#include <dink/binding.hpp>
#include <dink/cache.hpp>
#include <dink/config.hpp>
#include <dink/dispatcher.hpp>
#include <dink/meta.hpp>

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
concept IsTag =
    !IsConvertibleToBinding<Tag> && !IsConfig<Tag> && !IsContainer<Tag>;

//! Identifies types valid for tag arguments.
//
// The actual argument cannot be instantiated as void.
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
template <IsConfig Config = Config<>, typename Cache = cache::Type,
          typename Dispatcher = Dispatcher<>, IsParentContainer Parent = void,
          IsTag Tag = void>
  requires(!IsConvertibleToBinding<Cache>)
class Container;

//! Partial specialization where Parent = void produces a root container.
template <IsConfig Config, typename Cache, typename Dispatcher, IsTag Tag>
  requires(!IsConvertibleToBinding<Cache>)
class Container<Config, Cache, Dispatcher, void, Tag> {
 public:
  Container() noexcept = default;

  //! Construct from bindings.
  template <IsConvertibleToBinding... Bindings>
  explicit Container(Bindings&&... bindings) noexcept
      : Container{Cache{}, std::forward<Bindings>(bindings)...} {}

  //! Construct from cache and bindings.
  template <IsConvertibleToBinding... Bindings>
  explicit Container(Cache cache, Bindings&&... bindings) noexcept
      : Container{std::move(cache), Dispatcher{},
                  Config{std::forward<Bindings>(bindings)...}} {}

  //! Construct from components.
  Container(Cache cache, Dispatcher dispatcher, Config config) noexcept
      : cache_{std::move(cache)},
        dispatcher_{std::move(dispatcher)},
        config_{std::move(config)} {}

  //! Construct from bindings with tag.
  template <IsTagArg ActualTag, IsConvertibleToBinding... Bindings>
  explicit Container(ActualTag, Bindings&&... bindings) noexcept
      : Container{std::forward<Bindings>(bindings)...} {}

  //! Construct from tag, cache, and bindings.
  template <IsTagArg ActualTag, IsConvertibleToBinding... Bindings>
  explicit Container(ActualTag, Cache cache, Bindings&&... bindings) noexcept
      : Container{std::move(cache), std::forward<Bindings>(bindings)...} {}

  //! Construct from tag and components.
  template <IsTagArg ActualTag>
  Container(ActualTag, Cache cache, Dispatcher dispatcher,
            Config config) noexcept
      : Container{std::move(cache), std::move(dispatcher), std::move(config)} {}

  Container(const Container&) = delete;
  auto operator=(const Container&) const -> Container& = delete;
  Container(Container&&) = default;
  auto operator=(Container&&) const -> Container& = default;

  //! Resolve a dependency.
  template <typename Requested>
  auto resolve() -> meta::RemoveRvalueRef<Requested> {
    return dispatcher_.template resolve<Requested>(*this, config_, nullptr);
  }

  //! Get or create cached entry.
  template <typename Provider>
  auto get_or_create(Provider& provider) -> Provider::Provided& {
    return cache_.get_or_create(*this, provider);
  }

 private:
  [[dink_no_unique_address]] Cache cache_{};
  [[dink_no_unique_address]] Dispatcher dispatcher_{};
  Config config_{};
};

//! No specialization produces a child container.
template <IsConfig Config, typename Cache, typename Dispatcher,
          IsParentContainer Parent, IsTag Tag>
  requires(!IsConvertibleToBinding<Cache>)
class Container {
 public:
  //! Construct from parent only.
  explicit Container(Parent& parent) noexcept : Container{parent, Cache{}} {}

  //! Construct from bindings.
  template <IsConvertibleToBinding... Bindings>
  explicit Container(Parent& parent, Bindings&&... bindings) noexcept
      : Container{parent, Cache{}, std::forward<Bindings>(bindings)...} {}

  //! Construct from cache and bindings.
  template <IsConvertibleToBinding... Bindings>
  explicit Container(Parent& parent, Cache cache,
                     Bindings&&... bindings) noexcept
      : Container{parent, std::move(cache), Dispatcher{},
                  Config{std::forward<Bindings>(bindings)...}} {}

  //! Construct from components.
  Container(Parent& parent, Cache cache, Dispatcher dispatcher,
            Config config) noexcept
      : cache_{std::move(cache)},
        dispatcher_{std::move(dispatcher)},
        config_{std::move(config)},
        parent_{&parent} {}

  //! Construct from tag and bindings.
  template <IsTagArg ActualTag, IsConvertibleToBinding... Bindings>
  explicit Container(ActualTag, Parent& parent, Bindings&&... bindings) noexcept
      : Container{parent, std::forward<Bindings>(bindings)...} {}

  //! Construct from tag, cache, and bindings.
  template <IsTagArg ActualTag, IsConvertibleToBinding... Bindings>
  explicit Container(ActualTag, Parent& parent, Cache cache,
                     Bindings&&... bindings) noexcept
      : Container{parent, std::move(cache),
                  std::forward<Bindings>(bindings)...} {}

  //! Construct from tag and components.
  template <IsTagArg ActualTag>
  Container(ActualTag, Parent& parent, Cache cache, Dispatcher dispatcher,
            Config config) noexcept
      : Container{parent, std::move(cache), std::move(dispatcher),
                  std::move(config)} {}

  Container(const Container&) = delete;
  auto operator=(const Container&) const -> Container& = delete;
  Container(Container&&) = default;
  auto operator=(Container&&) const -> Container& = default;

  //! Resolve a dependency.
  template <typename Requested>
  auto resolve() -> meta::RemoveRvalueRef<Requested> {
    return dispatcher_.template resolve<Requested>(*this, config_, parent_);
  }

  //! Get or create cached entry.
  template <typename Provider>
  auto get_or_create(Provider& provider) -> Provider::Provided& {
    return cache_.get_or_create(*this, provider);
  }

 private:
  [[dink_no_unique_address]] Cache cache_{};
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
template <IsConfig Config, typename Cache, typename Dispatcher,
          IsParentContainer Parent, IsTag Tag>
  requires(!IsConvertibleToBinding<Cache>)
Container(Container<Config, Cache, Dispatcher, Parent, Tag>&)
    -> Container<dink::Config<>, cache::Type, dink::Dispatcher<>,
                 Container<Config, Cache, Dispatcher, Parent, Tag>, Tag>;

//! Root container from bindings.
template <IsConvertibleToBinding... Bindings>
Container(Bindings&&...)
    -> Container<decltype(Config{std::declval<Bindings>()...}), cache::Type,
                 Dispatcher<>, void, void>;

//! Root container from bindings with tag.
template <IsTagArg Tag, IsConvertibleToBinding... Bindings>
Container(Tag, Bindings&&...)
    -> Container<decltype(Config{std::declval<Bindings>()...}), cache::Type,
                 Dispatcher<>, void, Tag>;

//! Child container from parent.
template <IsParentContainer Parent>
Container(Parent& parent) -> Container<Config<>, cache::Type, Dispatcher<>,
                                       std::remove_cvref_t<Parent>, void>;

//! Child container from parent and bindings.
template <IsParentContainer Parent, IsConvertibleToBinding... Bindings>
Container(Parent& parent, Bindings&&...)
    -> Container<decltype(Config{std::declval<Bindings>()...}), cache::Type,
                 Dispatcher<>, std::remove_cvref_t<Parent>, void>;

//! Child container from parent, cache, and bindings.
template <IsParentContainer Parent, typename Cache,
          IsConvertibleToBinding... Bindings>
  requires(!IsConvertibleToBinding<Cache>)
Container(Parent& parent, Cache cache, Bindings&&...)
    -> Container<decltype(Config{std::declval<Bindings>()...}), Cache,
                 Dispatcher<>, std::remove_cvref_t<Parent>, void>;

//! Child container from parent with tag.
template <IsTagArg Tag, IsParentContainer Parent>
Container(Tag, Parent& parent) -> Container<Config<>, cache::Type, Dispatcher<>,
                                            std::remove_cvref_t<Parent>, Tag>;

//! Child container from parent and bindings with tag.
template <IsTagArg Tag, IsParentContainer Parent,
          IsConvertibleToBinding... Bindings>
Container(Tag, Parent& parent, Bindings&&...)
    -> Container<decltype(Config{std::declval<Bindings>()...}), cache::Type,
                 Dispatcher<>, std::remove_cvref_t<Parent>, Tag>;

//! Child container from parent, cache, and bindings with tag.
template <IsTagArg Tag, IsParentContainer Parent, typename Cache,
          IsConvertibleToBinding... Bindings>
  requires(!IsConvertibleToBinding<Cache>)
Container(Tag, Parent& parent, Cache cache, Bindings&&...)
    -> Container<decltype(Config{std::declval<Bindings>()...}), Cache,
                 Dispatcher<>, std::remove_cvref_t<Parent>, Tag>;

// ----------------------------------------------------------------------------
// Factory Functions
// ----------------------------------------------------------------------------

//! Macro to generate unique container types.
//
// This is a macro so UniqueType<> is generated once at each call site. There
// is no other way in the language to generate this once per instance. All
// other methods generate it once per type.
#define dink_unique_container(...) \
  Container {                      \
    ::dink::meta::UniqueType<> {}  \
    __VA_OPT__(, ) __VA_ARGS__     \
  }

}  // namespace dink
