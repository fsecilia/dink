// \file
// Copyright (c) 2025 Frank Secilia
// SPDX-License-Identifier: MIT
//
// \brief Routes requests to strategies.

#pragma once

#include <dink/lib.hpp>
#include <dink/bind.hpp>
#include <dink/canonical.hpp>
#include <dink/config.hpp>
#include <dink/meta.hpp>
#include <dink/provider.hpp>
#include <dink/strategy.hpp>

namespace dink {
namespace defaults {

//! Looks up bindings in config.
struct BindingLocator {
  template <typename FromType, typename Config>
  constexpr auto find(Config& config) const -> auto {
    return config.template find_binding<FromType>();
  }
};

//! Creates effective bindings for unbound types.
struct FallbackBindingFactory {
  template <typename FromType>
  constexpr auto create() const -> auto {
    return Binding<FromType, scope::Transient, provider::Ctor<FromType>>{};
  }
};

}  // namespace defaults

// ----------------------------------------------------------------------------
// Dispatcher
// ----------------------------------------------------------------------------

//! Dispatches resolution requests to appropriate strategies.
template <typename BindingLocator = defaults::BindingLocator,
          typename FallbackBindingFactory = defaults::FallbackBindingFactory,
          typename StrategyFactory = StrategyFactory>
class Dispatcher {
 public:
  explicit Dispatcher(BindingLocator binding_locator = {},
                      FallbackBindingFactory fallback_binding_factory = {},
                      StrategyFactory strategy_factory = {})
      : binding_locator_{std::move(binding_locator)},
        fallback_binding_factory_{std::move(fallback_binding_factory)},
        strategy_factory_{std::move(strategy_factory)} {}

  //! Resolves with found binding, delegates to parent, or uses fallback.
  template <typename Requested, typename Container, typename Config,
            typename ParentPtr>
  auto resolve(Container& container, Config& config, ParentPtr parent)
      -> meta::RemoveRvalueRef<Requested> {
    using Canonical = Canonical<Requested>;

    // Look up binding.
    auto binding = binding_locator_.template find<Canonical>(config);
    constexpr bool has_binding =
        !std::is_same_v<decltype(binding), std::nullptr_t>;

    if constexpr (has_binding) {
      // Found binding - execute with it.
      using Binding = std::remove_cvref_t<decltype(*binding)>;
      constexpr auto scope_provides_references =
          Binding::ScopeType::provides_references;

      return execute_strategy<Requested, has_binding,
                              scope_provides_references>(container, *binding);
    } else if constexpr (std::same_as<ParentPtr, std::nullptr_t>) {
      // no binding, no parent, use fallback bindings
      auto fallback_binding =
          fallback_binding_factory_.template create<Canonical>();
      return execute_strategy<Requested, false, false>(container,
                                                       fallback_binding);
    } else {
      // no binding, but still have parent to try.
      return parent->template resolve<Requested>();
    }
  }

 private:
  //! Executes strategy with given binding.
  template <typename Requested, bool has_binding,
            bool scope_provides_references, typename Container,
            typename Binding>
  auto execute_strategy(Container& container, Binding& binding)
      -> meta::RemoveRvalueRef<Requested> {
    auto strategy =
        strategy_factory_.template create<Requested, has_binding,
                                          scope_provides_references>();
    return strategy.template execute<Requested>(container, binding);
  }

  [[dink_no_unique_address]] BindingLocator binding_locator_{};
  [[dink_no_unique_address]] FallbackBindingFactory fallback_binding_factory_{};
  [[dink_no_unique_address]] StrategyFactory strategy_factory_{};
};

}  // namespace dink
