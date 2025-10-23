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
template <template <typename Container, typename DependencyChain,
                    scope::Lifetime min_lifetime> typename ResolverTemplate,
          template <typename Constructed,
                    typename Resolver> typename SingleArgResolverTemplate>
struct ResolverFactory {
  //! Creates a resolver, choosing the return type based on arity.
  //
  // For arity 1, this returns a \c SingleArgResolver. For all other
  // arities, it returns a \c Resolver.
  template <typename Container, typename DependencyChain,
            scope::Lifetime min_lifetime, typename Constructed,
            std::size_t arity, std::size_t index>
  constexpr auto create(auto& container) const noexcept -> auto {
    using Resolver = ResolverTemplate<Container, DependencyChain, min_lifetime>;
    using SingleArgResolver = SingleArgResolverTemplate<Constructed, Resolver>;
    if constexpr (arity == 1) {
      return SingleArgResolver{Resolver{container}};
    } else {
      return Resolver{container};
    }
  }
};

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

//! Invoker type for factory-based construction.
//
// Deduces arity from ConstructedFactory's call operator, creates resolvers
// for each parameter, and invokes the factory with resolved arguments.
template <typename Container, typename DependencyChain,
          scope::Lifetime min_lifetime, typename Constructed,
          typename ConstructedFactory>
using FactoryInvoker = Invoker<
    Constructed, ConstructedFactory,
    ResolverFactory<Resolver, SingleArgResolver>,
    std::make_index_sequence<dink::arity<Constructed, ConstructedFactory>>>;

//! Invoker type for direct construction.
//
// Deduces arity from Constructed's constructors, creates resolvers for each
// parameter, and directly constructs the instance with resolved arguments.
template <typename Container, typename DependencyChain,
          scope::Lifetime min_lifetime, typename Constructed>
using CtorInvoker =
    Invoker<Constructed, void, ResolverFactory<Resolver, SingleArgResolver>,
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
    using ResolverFactory = ResolverFactory<Resolver, SingleArgResolver>;

    static constexpr auto arity = dink::arity<Constructed, ConstructedFactory>;
    using Invoker = Invoker<Constructed, ConstructedFactory, ResolverFactory,
                            std::make_index_sequence<arity>>;

    return Invoker{std::move(constructed_factory), ResolverFactory{}};
  }

  // Ctor specialization.
  template <typename Container, typename DependencyChain,
            scope::Lifetime min_lifetime, typename Constructed>
  constexpr auto create()
      -> CtorInvoker<Container, DependencyChain, min_lifetime, Constructed> {
    using ResolverFactory = ResolverFactory<Resolver, SingleArgResolver>;

    static constexpr auto arity = dink::arity<Constructed, void>;
    using Invoker = Invoker<Constructed, void, ResolverFactory,
                            std::make_index_sequence<arity>>;

    return Invoker{ResolverFactory()};
  }
};

}  // namespace dink
