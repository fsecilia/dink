// \file
// Copyright (c) 2025 Frank Secilia
// SPDX-License-Identifier: MIT

#pragma once

#include <dink/lib.hpp>
#include <dink/invoker.hpp>
#include <dink/smart_pointer_traits.hpp>

namespace dink {

//! provider that invokes a Constructed's ctor
template <typename Constructed, typename InvokerFactory = InvokerFactory>
class CtorProvider {
 public:
  using Provided = Constructed;

  template <typename Container, typename Requested, typename DependencyChain,
            scope::Lifetime min_lifetime>
  constexpr auto create(Container& container) -> auto {
    const auto invoker =
        invoker_factory_.template create<Container, DependencyChain,
                                         min_lifetime, Constructed, void>();
    return invoker.template create<Requested>(container);
  }

  explicit constexpr CtorProvider(InvokerFactory invoker_factory = {}) noexcept
      : invoker_factory_{std::move(invoker_factory)} {}

 private:
  [[no_unique_address]] InvokerFactory invoker_factory_{};
};

}  // namespace dink
