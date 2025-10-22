// \file
// Copyright (c) 2025 Frank Secilia
// SPDX-License-Identifier: MIT

#pragma once

#include <dink/lib.hpp>
#include <dink/arity.hpp>
#include <dink/resolver.hpp>
#include <dink/scope.hpp>

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

}  // namespace dink
