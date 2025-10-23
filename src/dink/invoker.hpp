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

//! Factory that consumes indices to produce Resolvers.
template <typename Resolver, typename SingleArgResolver>
struct ResolverFactory {
  //! Creates a resolver, choosing the return type based on arity.
  //
  // For arity 1, this returns a \c SingleArgResolver. For all other
  // arities, it returns a \c Resolver.
  template <std::size_t arity, std::size_t index>
  constexpr auto create(auto& container) const noexcept -> auto {
    if constexpr (arity == 1)
      return SingleArgResolver{Resolver{container}};
    else
      return Resolver{container};
  }
};

//! Invokes a ctor or factory by replacing an index sequence.
//
// Invoker replaces each value in an index sequence with the output of
// an indexed factory, then uses the replaced sequence to either invoke
// ConstructedFactory, or if void, Constructed's ctor directly.
template <typename Constructed, typename ConstructedFactory,
          typename IndexedFactory, typename IndexSequence>
class Invoker;

//! Factory specialization.
template <typename Constructed, typename ConstructedFactory,
          typename IndexedFactory, std::size_t... indices>
class Invoker<Constructed, ConstructedFactory, IndexedFactory,
              std::index_sequence<indices...>> {
 public:
  constexpr auto create_value(auto& container) const -> Constructed {
    return constructed_factory_(
        indexed_factory_.template create<sizeof...(indices), indices>(
            container)...);
  }

  constexpr auto create_shared(auto& container) const
      -> std::shared_ptr<Constructed> {
    return std::make_shared<Constructed>(constructed_factory_(
        indexed_factory_.template create<sizeof...(indices), indices>(
            container)...));
  }

  constexpr auto create_unique(auto& container) const
      -> std::unique_ptr<Constructed> {
    return std::make_unique<Constructed>(constructed_factory_(
        indexed_factory_.template create<sizeof...(indices), indices>(
            container)...));
  }

  template <typename Requested>
  constexpr auto create(auto& container) const -> Requested {
    if constexpr (SharedPtr<Requested>) {
      return create_shared(container);
    } else if constexpr (UniquePtr<Requested>) {
      return create_unique(container);
    } else {
      return create_value(container);
    }
  }

  explicit constexpr Invoker(ConstructedFactory constructed_factory,
                             IndexedFactory indexed_factory) noexcept
      : constructed_factory_{std::move(constructed_factory)},
        indexed_factory_{std::move(indexed_factory)} {}

 private:
  ConstructedFactory constructed_factory_{};
  IndexedFactory indexed_factory_{};
};

//! Ctor specialization.
template <typename Constructed, typename IndexedFactory, std::size_t... indices>
class Invoker<Constructed, void, IndexedFactory,
              std::index_sequence<indices...>> {
 public:
  constexpr auto create_value(auto& container) const -> Constructed {
    return Constructed{
        indexed_factory_.template create<sizeof...(indices), indices>(
            container)...};
  }

  constexpr auto create_shared(auto& container) const
      -> std::shared_ptr<Constructed> {
    return std::make_shared<Constructed>(
        indexed_factory_.template create<sizeof...(indices), indices>(
            container)...);
  }

  constexpr auto create_unique(auto& container) const
      -> std::unique_ptr<Constructed> {
    return std::make_unique<Constructed>(
        indexed_factory_.template create<sizeof...(indices), indices>(
            container)...);
  }

  template <typename Requested>
  constexpr auto create(auto& container) const -> Requested {
    if constexpr (SharedPtr<Requested>) {
      return create_shared(container);
    } else if constexpr (UniquePtr<Requested>) {
      return create_unique(container);
    } else {
      return create_value(container);
    }
  }

  explicit constexpr Invoker(IndexedFactory indexed_factory) noexcept
      : indexed_factory_{std::move(indexed_factory)} {}

 private:
  IndexedFactory indexed_factory_{};
};

//! Invoker type for factory-based construction.
//
// Deduces arity from ConstructedFactory's call operator, creates resolvers
// for each parameter, and invokes the factory with resolved arguments.
template <typename Container, typename DependencyChain,
          scope::Lifetime min_lifetime, typename Constructed,
          typename ConstructedFactory>
using FactoryInvoker = Invoker<
    Constructed, ConstructedFactory,
    ResolverFactory<
        Resolver<Container, DependencyChain, min_lifetime>,
        SingleArgResolver<Constructed,
                          Resolver<Container, DependencyChain, min_lifetime>>>,
    std::make_index_sequence<dink::arity<Constructed, ConstructedFactory>>>;

//! Invoker type for direct construction.
//
// Deduces arity from Constructed's constructors, creates resolvers for each
// parameter, and directly constructs the instance with resolved arguments.
template <typename Container, typename DependencyChain,
          scope::Lifetime min_lifetime, typename Constructed>
using CtorInvoker = Invoker<
    Constructed, void,
    ResolverFactory<
        Resolver<Container, DependencyChain, min_lifetime>,
        SingleArgResolver<Constructed,
                          Resolver<Container, DependencyChain, min_lifetime>>>,
    std::make_index_sequence<dink::arity<Constructed, void>>>;

//! Creates FactoryInvoker and CtorInvoker.
struct InvokerFactory {
  // Factory specialization.
  template <typename Container, typename DependencyChain,
            scope::Lifetime min_lifetime, typename Constructed,
            typename ConstructedFactory>
  constexpr auto create(ConstructedFactory constructed_factory)
      -> FactoryInvoker<Container, DependencyChain, min_lifetime, Constructed,
                        ConstructedFactory> {
    using Resolver = Resolver<Container, DependencyChain, min_lifetime>;
    using SingleArgResolver = SingleArgResolver<Constructed, Resolver>;
    using IndexedResolverFactory = ResolverFactory<Resolver, SingleArgResolver>;

    static constexpr auto arity = dink::arity<Constructed, ConstructedFactory>;
    using Invoker =
        Invoker<Constructed, ConstructedFactory, IndexedResolverFactory,
                std::make_index_sequence<arity>>;

    return Invoker{std::move(constructed_factory), IndexedResolverFactory{}};
  }

  // Ctor specialization.
  template <typename Container, typename DependencyChain,
            scope::Lifetime min_lifetime, typename Constructed>
  constexpr auto create()
      -> CtorInvoker<Container, DependencyChain, min_lifetime, Constructed> {
    using Resolver = Resolver<Container, DependencyChain, min_lifetime>;
    using SingleArgResolver = SingleArgResolver<Constructed, Resolver>;
    using IndexedResolverFactory = ResolverFactory<Resolver, SingleArgResolver>;

    static constexpr auto arity = dink::arity<Constructed, void>;
    using Invoker = Invoker<Constructed, void, IndexedResolverFactory,
                            std::make_index_sequence<arity>>;

    return Invoker{IndexedResolverFactory()};
  }
};

}  // namespace dink
