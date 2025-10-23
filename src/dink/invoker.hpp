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
// ResolverSequence, then uses the replaced sequence to either invoke
// ConstructedFactory, or if void, Constructed's ctor directly.
template <typename Constructed, typename ConstructedFactory,
          typename ResolverSequence, typename IndexSequence>
class Invoker;

//! Factory specialization.
template <typename Constructed, typename ConstructedFactory,
          typename ResolverSequence, std::size_t... indices>
class Invoker<Constructed, ConstructedFactory, ResolverSequence,
              std::index_sequence<indices...>> {
 public:
  template <typename DependencyChain, scope::Lifetime min_lifetime,
            typename Container>
  constexpr auto create_value(Container& container) const -> Constructed {
    return constructed_factory_(
        resolver_sequence_
            .template create_element<DependencyChain, min_lifetime, Constructed,
                                     sizeof...(indices), indices>(
                container)...);
  }

  template <typename DependencyChain, scope::Lifetime min_lifetime,
            typename Container>
  constexpr auto create_shared(Container& container) const
      -> std::shared_ptr<Constructed> {
    return std::make_shared<Constructed>(constructed_factory_(
        resolver_sequence_
            .template create_element<DependencyChain, min_lifetime, Constructed,
                                     sizeof...(indices), indices>(
                container)...));
  }

  template <typename DependencyChain, scope::Lifetime min_lifetime,
            typename Container>
  constexpr auto create_unique(Container& container) const
      -> std::unique_ptr<Constructed> {
    return std::make_unique<Constructed>(constructed_factory_(
        resolver_sequence_
            .template create_element<DependencyChain, min_lifetime, Constructed,
                                     sizeof...(indices), indices>(
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
                             ResolverSequence resolver_sequence) noexcept
      : constructed_factory_{std::move(constructed_factory)},
        resolver_sequence_{std::move(resolver_sequence)} {}

 private:
  ConstructedFactory constructed_factory_{};
  ResolverSequence resolver_sequence_{};
};

//! Ctor specialization.
template <typename Constructed, typename ResolverSequence,
          std::size_t... indices>
class Invoker<Constructed, void, ResolverSequence,
              std::index_sequence<indices...>> {
 public:
  template <typename DependencyChain, scope::Lifetime min_lifetime,
            typename Container>
  constexpr auto create_value(Container& container) const -> Constructed {
    return Constructed{
        resolver_sequence_
            .template create_element<DependencyChain, min_lifetime, Constructed,
                                     sizeof...(indices), indices>(
                container)...};
  }

  template <typename DependencyChain, scope::Lifetime min_lifetime,
            typename Container>
  constexpr auto create_shared(Container& container) const
      -> std::shared_ptr<Constructed> {
    return std::make_shared<Constructed>(
        resolver_sequence_
            .template create_element<DependencyChain, min_lifetime, Constructed,
                                     sizeof...(indices), indices>(
                container)...);
  }

  template <typename DependencyChain, scope::Lifetime min_lifetime,
            typename Container>
  constexpr auto create_unique(Container& container) const
      -> std::unique_ptr<Constructed> {
    return std::make_unique<Constructed>(
        resolver_sequence_
            .template create_element<DependencyChain, min_lifetime, Constructed,
                                     sizeof...(indices), indices>(
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

  explicit constexpr Invoker(ResolverSequence resolver_sequence) noexcept
      : resolver_sequence_{std::move(resolver_sequence)} {}

 private:
  ResolverSequence resolver_sequence_{};
};

//! Creates invokers.
template <template <typename Constructed, typename ConstructedFactory,
                    typename ResolverSequence,
                    typename IndexSequence> typename Invoker,
          typename ResolverSequenceFactory = ResolverSequenceFactory<>>
class InvokerFactory {
 public:
  // Factory specialization.
  template <typename Constructed, typename ConstructedFactory,
            typename ResolverSequence>
  constexpr auto create(ConstructedFactory constructed_factory) -> auto {
    return Invoker<
        Constructed, ConstructedFactory, ResolverSequence,
        std::make_index_sequence<arity<Constructed, ConstructedFactory>>>{
        std::move(constructed_factory), resolver_sequence_factory_.create()};
  }

  // Ctor specialization.
  template <typename Constructed, typename ResolverSequence>
  constexpr auto create() -> auto {
    return Invoker<Constructed, void, ResolverSequence,
                   std::make_index_sequence<arity<Constructed, void>>>{
        resolver_sequence_factory_.create()};
  }

  explicit constexpr InvokerFactory(
      ResolverSequenceFactory resolver_sequence_factory) noexcept
      : resolver_sequence_factory_{std::move(resolver_sequence_factory)} {}

  constexpr InvokerFactory() = default;

 private:
  ResolverSequenceFactory resolver_sequence_factory_{};
};

}  // namespace dink
