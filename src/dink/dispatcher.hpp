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
#include <dink/resolution_strategy.hpp>
#include <dink/smart_pointer_traits.hpp>

namespace dink {
namespace detail {

//! Policy for looking up bindings in config.
struct BindingLookup {
  template <typename Canonical, typename Config>
  auto find(Config& config) const {
    return config.template find_binding<Canonical>();
  }
};

//! Policy for creating default bindings when none exist in config.
//
// The binding's scope doesn't matter since strategies either override it
// (OverrideScope) or ignore it entirely (CacheSharedPtr).
struct DefaultBindingFactory {
  template <typename Canonical>
  constexpr auto create() const {
    return Binding<Canonical, scope::Transient, provider::Ctor<Canonical>>{
        scope::Transient{}, provider::Ctor<Canonical>{}};
  }
};

}  // namespace detail

// ----------------------------------------------------------------------------
// StrategyFactory
// ----------------------------------------------------------------------------

//! Determines and creates the appropriate resolution strategy
struct StrategyFactory {
  //! Creates the appropriate strategy for the given resolution context
  template <typename Requested, bool has_binding,
            bool scope_provides_references>
  static constexpr auto create_strategy() {
    if constexpr (UniquePtr<Requested>) {
      return strategies::RelegateToTransient{};
    } else if constexpr (SharedPtr<Requested> || WeakPtr<Requested>) {
      if constexpr (has_binding && !scope_provides_references) {
        // Transient scope with shared_ptr - let it create new instances
        return strategies::UseBoundScope{};
      } else {
        // Singleton scope or no binding - wrap via canonical shared_ptr
        return strategies::CacheSharedPtr{};
      }
    } else if constexpr (std::is_lvalue_reference_v<Requested> ||
                         std::is_pointer_v<Requested>) {
      // Lvalue ref or pointer
      if constexpr (has_binding) {
        if constexpr (scope_provides_references) {
          return strategies::UseBoundScope{};
        } else {
          return strategies::PromoteToSingleton{};
        }
      } else {
        return strategies::PromoteToSingleton{};
      }
    } else {
      // Value or rvalue Ref
      if constexpr (has_binding) {
        if constexpr (scope_provides_references) {
          return strategies::RelegateToTransient{};
        } else {
          return strategies::UseBoundScope{};
        }
      } else {
        return strategies::RelegateToTransient{};
      }
    }
  }
};

// ----------------------------------------------------------------------------
// Dispatcher
// ----------------------------------------------------------------------------

//! Dispatches resolution requests to appropriate strategies.
template <typename BindingLookup = detail::BindingLookup,
          typename DefaultBindingFactory = detail::DefaultBindingFactory>
class Dispatcher {
 public:
  explicit Dispatcher(BindingLookup lookup_policy = {},
                      DefaultBindingFactory binding_factory = {})
      : lookup_policy_{std::move(lookup_policy)},
        default_binding_factory_{std::move(binding_factory)} {}

  //! Resolves with found binding or calls not_found_handler
  template <typename Requested, typename Container, typename Config,
            typename NotFoundHandler>
  auto resolve(Container& container, Config& config,
               NotFoundHandler&& not_found_handler)
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
      // No binding - call handler (either creates default or delegates to
      // parent)
      return not_found_handler();
    }
  }

  //! Executes resolution with a default binding
  template <typename Requested, typename Container>
  auto execute_strategy_with_default_binding(Container& container)
      -> remove_rvalue_ref_t<Requested> {
    using Canonical = Canonical<Requested>;

    auto strategy = StrategyFactory::create_strategy<Requested, false, false>();
    auto default_binding =
        default_binding_factory_.template create<Canonical>();

    return strategy.template execute<Requested>(container, default_binding);
  }

 private:
  //! Executes resolution with a specific binding
  template <typename Requested, bool has_binding,
            bool scope_provides_references, typename Container,
            typename Binding>
  auto execute_strategy(Container& container, Binding& binding)
      -> remove_rvalue_ref_t<Requested> {
    auto strategy =
        StrategyFactory::create_strategy<Requested, has_binding,
                                         scope_provides_references>();
    return strategy.template execute<Requested>(container, binding);
  }

  [[no_unique_address]] BindingLookup lookup_policy_{};
  [[no_unique_address]] DefaultBindingFactory default_binding_factory_{};
};

}  // namespace dink
