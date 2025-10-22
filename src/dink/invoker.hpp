// \file
// Copyright (c) 2025 Frank Secilia
// SPDX-License-Identifier: MIT

#pragma once

#include <dink/lib.hpp>
#include <dink/arity.hpp>
#include <dink/resolver.hpp>
#include <dink/scope.hpp>
#include <memory>
#include <utility>

namespace dink {

//! Factory that consumes indices to produce Resolvers.
template <typename Resolver, typename SingleArgResolver>
struct IndexedResolverFactory {
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
// SequencedInvoker replaces each value in an index sequence with the output of
// an indexed factory, then uses the replaced sequence to either invoke
// Constructed's ctor, or invoke a factory that produces a Constructed, passed
// as an argument.
template <typename Constructed, typename IndexedFactory, typename IndexSequence>
class SequencedInvoker;

//! Partial specialization to unpack the index sequence.
template <typename Constructed, typename IndexedFactory, std::size_t... indices>
class SequencedInvoker<Constructed, IndexedFactory,
                       std::index_sequence<indices...>> {
 public:
  // Constructor Invokers
  // --------------------------------------------------------------------------
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

  // Factory Invokers
  // --------------------------------------------------------------------------
  constexpr auto create_value(auto& container, auto& constructed_factory) const
      -> Constructed {
    return constructed_factory(
        indexed_factory_.template create<sizeof...(indices), indices>(
            container)...);
  }

  constexpr auto create_shared(auto& container, auto& constructed_factory) const
      -> std::shared_ptr<Constructed> {
    return std::make_shared<Constructed>(constructed_factory(
        indexed_factory_.template create<sizeof...(indices), indices>(
            container)...));
  }

  constexpr auto create_unique(auto& container, auto& constructed_factory) const
      -> std::unique_ptr<Constructed> {
    return std::make_unique<Constructed>(constructed_factory(
        indexed_factory_.template create<sizeof...(indices), indices>(
            container)...));
  }

  // Constructors
  // --------------------------------------------------------------------------
  explicit constexpr SequencedInvoker(IndexedFactory indexed_factory) noexcept
      : indexed_factory_{std::move(indexed_factory)} {}

  SequencedInvoker() = default;

 private:
  IndexedFactory indexed_factory_{};
};

}  // namespace dink
