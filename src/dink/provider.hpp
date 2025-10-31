// \file
// Copyright (c) 2025 Frank Secilia
// SPDX-License-Identifier: MIT
//
// \brief Defines how managed instances are constructed or located.

#pragma once

#include <dink/lib.hpp>
#include <dink/invoker.hpp>
#include <dink/meta.hpp>

namespace dink::provider {

//! Invokes Constructed's ctor directly.
template <typename Constructed,
          typename InvokerFactory = InvokerFactory<Invoker>>
class Ctor {
 public:
  using Provided = Constructed;

  template <typename Requested, typename Container>
  constexpr auto create(Container& container) -> auto {
    const auto invoker = invoker_factory_.template create<Constructed, void>();
    return invoker.template create<Requested>(container);
  }

  explicit constexpr Ctor(InvokerFactory invoker_factory) noexcept
      : invoker_factory_{std::move(invoker_factory)} {}

  constexpr Ctor() = default;

 private:
  [[dink_no_unique_address]] InvokerFactory invoker_factory_{};
};

//! Invokes ConstructedFactory to produce a Constructed.
template <typename Constructed, typename ConstructedFactory,
          typename InvokerFactory = InvokerFactory<Invoker>>
class Factory {
 public:
  using Provided = Constructed;

  template <typename Requested, typename Container>
  constexpr auto create(Container& container) -> auto {
    const auto invoker =
        invoker_factory_.template create<Constructed, ConstructedFactory>();
    return invoker.template create<Requested>(container, constructed_factory_);
  }

  explicit constexpr Factory(ConstructedFactory constructed_factory,
                             InvokerFactory invoker_factory = {}) noexcept
      : constructed_factory_{std::move(constructed_factory)},
        invoker_factory_{std::move(invoker_factory)} {}

  constexpr Factory() = default;

 private:
  [[dink_no_unique_address]] ConstructedFactory constructed_factory_{};
  [[dink_no_unique_address]] InvokerFactory invoker_factory_{};
};

//! Wraps an external reference.
template <typename Instance>
class External {
 public:
  using Provided = Instance;

  template <typename Requested, typename Container>
  constexpr auto create(Container& /*container*/) const -> Instance& {
    // Always return reference to the external instance
    return *instance_;
  }

  explicit constexpr External(Instance& instance) noexcept
      : instance_{&instance} {}

 private:
  Instance* instance_;
};

}  // namespace dink::provider
