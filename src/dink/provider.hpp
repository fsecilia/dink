// \file
// Copyright (c) 2025 Frank Secilia
// SPDX-License-Identifier: MIT

#pragma once

#include <dink/lib.hpp>
#include <dink/invoker.hpp>
#include <dink/smart_pointer_traits.hpp>

namespace dink {

//! provider that invokes Constructed's ctor directly
template <typename Constructed,
          typename InvokerFactory = InvokerFactory<Invoker>>
class CtorProvider {
 public:
  using Provided = Constructed;

  template <typename Requested, typename DependencyChain,
            scope::Lifetime min_lifetime, typename Container>
  constexpr auto create(Container& container) -> auto {
    const auto invoker =
        invoker_factory_.template create<Container, DependencyChain,
                                         min_lifetime, Constructed, void>();
    return invoker.template create<Requested>(container);
  }

  explicit constexpr CtorProvider(InvokerFactory invoker_factory = {}) noexcept
      : invoker_factory_{std::move(invoker_factory)} {}

 private:
  [[dink_no_unique_address]] InvokerFactory invoker_factory_{};
};

//! provider that invokes ConstructedFactory to produce a Constructed
template <typename Constructed, typename ConstructedFactory,
          typename InvokerFactory = InvokerFactory<Invoker>>
class FactoryProvider {
 public:
  using Provided = Constructed;

  template <typename Requested, typename DependencyChain,
            scope::Lifetime min_lifetime, typename Container>
  constexpr auto create(Container& container) -> auto {
    const auto invoker =
        invoker_factory_
            .template create<Container, DependencyChain, min_lifetime,
                             Constructed, ConstructedFactory>();
    return invoker.template create<Requested>(container, constructed_factory_);
  }

  explicit constexpr FactoryProvider(
      ConstructedFactory constructed_factory,
      InvokerFactory invoker_factory = {}) noexcept
      : constructed_factory_{std::move(constructed_factory)},
        invoker_factory_{std::move(invoker_factory)} {}

 private:
  [[dink_no_unique_address]] ConstructedFactory constructed_factory_{};
  [[dink_no_unique_address]] InvokerFactory invoker_factory_{};
};

}  // namespace dink
