// \file
// Copyright (c) 2025 Frank Secilia
// SPDX-License-Identifier: MIT

#pragma once

#include <dink/lib.hpp>
#include <dink/binding.hpp>
#include <dink/canonical.hpp>
#include <dink/config.hpp>
#include <dink/meta.hpp>
#include <dink/provider.hpp>
#include <dink/remove_rvalue_ref.hpp>
#include <dink/smart_pointer_traits.hpp>
#include <dink/strategy.hpp>

namespace dink {
namespace detail {

//! Default policy for looking up bindings in config.
struct BindingLocator {
  template <typename Canonical, typename Config>
  auto find(Config& config) const {
    return config.template find_binding<Canonical>();
  }
};

//! Default policy for creating bindings when none exist in config.
struct FallbackBindingFactory {
  template <typename Canonical>
  constexpr auto create() const {
    return Binding<Canonical, scope::Transient, provider::Ctor<Canonical>>{
        scope::Transient{}, provider::Ctor<Canonical>{}};
  }
};

}  // namespace detail

// ----------------------------------------------------------------------------
// Dispatcher
// ----------------------------------------------------------------------------

//! Dispatches resolution requests to appropriate strategies.
template <typename StrategyFactory = StrategyFactory,
          typename BindingLocator = detail::BindingLocator,
          typename FallbackBindingFactory = detail::FallbackBindingFactory>
class Dispatcher {
 public:
  explicit Dispatcher(StrategyFactory strategy_factory = {},
                      BindingLocator lookup_policy = {},
                      FallbackBindingFactory fallback_binding_factory = {})
      : strategy_factory_{std::move(strategy_factory)},
        lookup_policy_{std::move(lookup_policy)},
        fallback_binding_factory_{std::move(fallback_binding_factory)} {}

  //! Resolves with found binding or calls not_found_handler
  template <typename Requested, typename Container, typename Config,
            typename Parent>
  auto resolve(Container& container, Config& config, Parent parent)
      -> remove_rvalue_ref_t<Requested> {
    using Canonical = Canonical<Requested>;

    auto binding_ptr = lookup_policy_.template find<Canonical>(config);
    constexpr bool has_binding =
        !std::is_same_v<decltype(binding_ptr), std::nullptr_t>;

    if constexpr (has_binding) {
      // Found binding - execute with it
      using Binding = std::remove_cvref_t<decltype(*binding_ptr)>;
      constexpr bool provides_references =
          Binding::ScopeType::provides_references;

      return execute_strategy<Requested, has_binding, provides_references>(
          container, *binding_ptr);
    } else {
      // no binding; try delegating to parent
      if constexpr (std::same_as<Parent, std::nullptr_t>) {
        // no binding, no parent, use fallback bindings
        return execute_strategy_with_fallback_binding<Requested>(container);
      } else {
        return delegate_to_parent<Requested>(*parent);
      }
    }
  }

 private:
  //! Executes strategy with a specific binding
  template <typename Requested, bool has_binding,
            bool scope_provides_references, typename Container,
            typename Binding>
  auto execute_strategy(Container& container, Binding& binding)
      -> remove_rvalue_ref_t<Requested> {
    auto strategy =
        strategy_factory_.template create<Requested, has_binding,
                                          scope_provides_references>();
    return strategy.template execute<Requested>(container, binding);
  }

  //! Executes strategy with a default binding
  template <typename Requested, typename Container>
  auto execute_strategy_with_fallback_binding(Container& container)
      -> remove_rvalue_ref_t<Requested> {
    using Canonical = Canonical<Requested>;

    auto strategy =
        strategy_factory_.template create<Requested, false, false>();
    auto fallback_binding =
        fallback_binding_factory_.template create<Canonical>();

    return strategy.template execute<Requested>(container, fallback_binding);
  }

  template <typename Requested>
  auto delegate_to_parent(auto& parent) -> remove_rvalue_ref_t<Requested> {
    return parent.template resolve<Requested>();
  }

  [[no_unique_address]] StrategyFactory strategy_factory_{};
  [[no_unique_address]] BindingLocator lookup_policy_{};
  [[no_unique_address]] FallbackBindingFactory fallback_binding_factory_{};
};

}  // namespace dink
