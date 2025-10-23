// \file
// Copyright (c) 2025 Frank Secilia
// SPDX-License-Identifier: MIT

#pragma once

#include <dink/lib.hpp>
#include <dink/arity.hpp>
#include <dink/resolver.hpp>
#include <dink/scope.hpp>
#include <dink/smart_pointer_traits.hpp>
#include <memory>
#include <utility>

namespace dink {

//! Invokes a ctor or factory by replacing an index sequence.
//
// Invoker replaces each value in an index sequence with the output of a
// ResolverFactory, then uses the replaced sequence to either invoke
// ConstructedFactory, or if void, Constructed's ctor directly.
template <typename Constructed, typename ConstructedFactory,
          typename ResolverFactory, typename IndexSequence>
class Invoker;

//! Factory specialization.
template <typename Constructed, typename ConstructedFactory,
          typename ResolverFactory, std::size_t... indices>
class Invoker<Constructed, ConstructedFactory, ResolverFactory,
              std::index_sequence<indices...>> {
 public:
  template <typename DependencyChain, scope::Lifetime min_lifetime,
            typename Container>
  constexpr auto create_value(Container& container) const -> Constructed {
    return constructed_factory_(
        resolver_factory_
            .template create<Container, DependencyChain, min_lifetime,
                             Constructed, sizeof...(indices), indices>(
                container)...);
  }

  template <typename DependencyChain, scope::Lifetime min_lifetime,
            typename Container>
  constexpr auto create_shared(Container& container) const
      -> std::shared_ptr<Constructed> {
    return std::make_shared<Constructed>(constructed_factory_(
        resolver_factory_
            .template create<Container, DependencyChain, min_lifetime,
                             Constructed, sizeof...(indices), indices>(
                container)...));
  }

  template <typename DependencyChain, scope::Lifetime min_lifetime,
            typename Container>
  constexpr auto create_unique(Container& container) const
      -> std::unique_ptr<Constructed> {
    return std::make_unique<Constructed>(constructed_factory_(
        resolver_factory_
            .template create<Container, DependencyChain, min_lifetime,
                             Constructed, sizeof...(indices), indices>(
                container)...));
  }

  template <typename Requested, typename DependencyChain,
            scope::Lifetime min_lifetime, typename Container>
  constexpr auto create(Container& container) const -> Requested {
    if constexpr (SharedPtr<Requested>) {
      return create_shared<DependencyChain, min_lifetime>(container);
    } else if constexpr (UniquePtr<Requested>) {
      return create_unique<DependencyChain, min_lifetime>(container);
    } else {
      return create_value<DependencyChain, min_lifetime>(container);
    }
  }

  explicit constexpr Invoker(ConstructedFactory constructed_factory,
                             ResolverFactory resolver_factory) noexcept
      : constructed_factory_{std::move(constructed_factory)},
        resolver_factory_{std::move(resolver_factory)} {}

 private:
  ConstructedFactory constructed_factory_{};
  ResolverFactory resolver_factory_{};
};

//! Ctor specialization.
template <typename Constructed, typename ResolverFactory,
          std::size_t... indices>
class Invoker<Constructed, void, ResolverFactory,
              std::index_sequence<indices...>> {
 public:
  template <typename DependencyChain, scope::Lifetime min_lifetime,
            typename Container>
  constexpr auto create_value(Container& container) const -> Constructed {
    return Constructed{
        resolver_factory_
            .template create<Container, DependencyChain, min_lifetime,
                             Constructed, sizeof...(indices), indices>(
                container)...};
  }

  template <typename DependencyChain, scope::Lifetime min_lifetime,
            typename Container>
  constexpr auto create_shared(Container& container) const
      -> std::shared_ptr<Constructed> {
    return std::make_shared<Constructed>(
        resolver_factory_
            .template create<Container, DependencyChain, min_lifetime,
                             Constructed, sizeof...(indices), indices>(
                container)...);
  }

  template <typename DependencyChain, scope::Lifetime min_lifetime,
            typename Container>
  constexpr auto create_unique(Container& container) const
      -> std::unique_ptr<Constructed> {
    return std::make_unique<Constructed>(
        resolver_factory_
            .template create<Container, DependencyChain, min_lifetime,
                             Constructed, sizeof...(indices), indices>(
                container)...);
  }

  template <typename Requested, typename DependencyChain,
            scope::Lifetime min_lifetime, typename Container>
  constexpr auto create(Container& container) const -> Requested {
    if constexpr (SharedPtr<Requested>) {
      return create_shared<DependencyChain, min_lifetime>(container);
    } else if constexpr (UniquePtr<Requested>) {
      return create_unique<DependencyChain, min_lifetime>(container);
    } else {
      return create_value<DependencyChain, min_lifetime>(container);
    }
  }

  explicit constexpr Invoker(ResolverFactory resolver_factory) noexcept
      : resolver_factory_{std::move(resolver_factory)} {}

 private:
  ResolverFactory resolver_factory_{};
};

//! Creates invokers.
template <template <typename Constructed, typename ConstructedFactory,
                    typename ResolverFactory,
                    typename IndexSequence> typename Invoker>
struct InvokerFactory {
  // Factory specialization.
  template <typename Constructed, typename ConstructedFactory,
            typename ResolverFactory>
  constexpr auto create(ConstructedFactory constructed_factory) -> auto {
    return Invoker<
        Constructed, ConstructedFactory, ResolverFactory,
        std::make_index_sequence<arity<Constructed, ConstructedFactory>>>{
        std::move(constructed_factory), ResolverFactory{}};
  }

  // Ctor specialization.
  template <typename Constructed, typename ResolverFactory>
  constexpr auto create() -> auto {
    return Invoker<Constructed, void, ResolverFactory,
                   std::make_index_sequence<arity<Constructed, void>>>{
        ResolverFactory()};
  }
};

}  // namespace dink
