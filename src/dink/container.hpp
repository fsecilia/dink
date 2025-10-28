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

namespace dink {

// ----------------------------------------------------------------------------
// Resolution Strategy
// ----------------------------------------------------------------------------

enum class ResolutionStrategy {
  UseBoundScope,        // Use binding's scope and provider
  RelegateToTransient,  // Override scope to transient, use bound provider
  PromoteToSingleton,   // Override scope to singleton, use bound provider
  CacheSharedPtr        // Cache shared_ptr to singleton
};

// ----------------------------------------------------------------------------
// Detail Helpers
// ----------------------------------------------------------------------------

namespace detail {

//! Default policy for looking up bindings in config
struct BindingLookup {
  template <typename Canonical, typename Config>
  auto find(Config& config) const {
    return config.template find_binding<Canonical>();
  }
};

//! Default policy for creating default bindings when none exist in config
//
// The binding's scope doesn't matter since executors either override it
// (OverrideScope) or ignore it entirely (CacheSharedPtr).
struct BindingFactory {
  template <typename Canonical>
  constexpr auto create() const {
    return Binding<Canonical, scope::Transient, provider::Ctor<Canonical>>{
        scope::Transient{}, provider::Ctor<Canonical>{}};
  }
};

//! Transitive provider for canonical shared_ptr caching
//
// This provider resolves the reference indirectly by calling back into the
// container to resolve the original type, then wraps it in a shared_ptr.
template <typename Constructed>
struct CachedSharedPtrProvider {
  using Provided = std::shared_ptr<Constructed>;

  template <typename Requested, typename Container>
  auto create(Container& container) -> std::shared_ptr<Constructed> {
    // Resolve reference using the container (follows delegation chain)
    auto& ref = container.template resolve<Constructed&>();
    // Wrap in shared_ptr with no-op deleter
    return std::shared_ptr<Constructed>{&ref, [](Constructed*) {}};
  }
};

//! Factory for creating CachedSharedPtrProvider instances
struct CachedSharedPtrProviderFactory {
  template <typename Canonical>
  auto create() const -> CachedSharedPtrProvider<Canonical> {
    return {};
  }
};

}  // namespace detail

// ----------------------------------------------------------------------------
// Execution Implementations
// ----------------------------------------------------------------------------

namespace execution {

//! Executes using binding's scope and provider directly
struct UseBoundScope {
  template <typename Requested, typename Container, typename Binding>
  auto execute(Container& container, Binding& binding) const
      -> remove_rvalue_ref_t<Requested> {
    return binding.scope.template resolve<Requested>(container,
                                                     binding.provider);
  }
};

//! Executes by forcing a specific scope type
template <typename Scope>
struct OverrideScope {
  [[no_unique_address]] Scope scope{};

  template <typename Requested, typename Container, typename Binding>
  auto execute(Container& container, Binding& binding) const
      -> remove_rvalue_ref_t<Requested> {
    return scope.template resolve<Requested>(container, binding.provider);
  }
};

//! Executes by wrapping reference in canonical shared_ptr
//
// Note: This strategy doesn't use the binding parameter - it creates a new
// transitive provider that wraps a recursive resolution.
template <typename Scope, typename ProviderFactory>
struct CacheSharedPtr {
  [[no_unique_address]] Scope scope{};
  [[no_unique_address]] ProviderFactory provider_factory{};

  template <typename Requested, typename Container, typename Binding>
  auto execute(Container& container, Binding& /*binding*/) const
      -> remove_rvalue_ref_t<Requested> {
    using Canonical = Canonical<Requested>;
    auto provider = provider_factory.template create<Canonical>();
    return scope.template resolve<Requested>(container, provider);
  }
};

}  // namespace execution

// ----------------------------------------------------------------------------
// Strategy Executor - Selects execution implementation by strategy enum
// ----------------------------------------------------------------------------

template <ResolutionStrategy strategy>
struct StrategyExecutor;

template <>
struct StrategyExecutor<ResolutionStrategy::UseBoundScope>
    : execution::UseBoundScope {};

template <>
struct StrategyExecutor<ResolutionStrategy::RelegateToTransient>
    : execution::OverrideScope<scope::Transient> {};

template <>
struct StrategyExecutor<ResolutionStrategy::PromoteToSingleton>
    : execution::OverrideScope<scope::Singleton> {};

template <>
struct StrategyExecutor<ResolutionStrategy::CacheSharedPtr>
    : execution::CacheSharedPtr<scope::Singleton,
                                detail::CachedSharedPtrProviderFactory> {};

// ----------------------------------------------------------------------------
// Strategy Executor Factory
// ----------------------------------------------------------------------------

template <template <ResolutionStrategy> typename StrategyExecutorTemplate>
struct StrategyExecutorFactory {
  template <ResolutionStrategy strategy>
  auto create() const -> StrategyExecutorTemplate<strategy> {
    return {};
  }
};

// ----------------------------------------------------------------------------
// Strategy Policy - Determines strategy and creates appropriate executor
// ----------------------------------------------------------------------------

template <typename ExecutorFactory = StrategyExecutorFactory<StrategyExecutor>>
struct StrategyPolicy {
  [[no_unique_address]] ExecutorFactory executor_factory_{};

  //! Creates the appropriate executor for the given resolution context
  template <typename Requested, bool has_binding,
            bool scope_provides_references>
  auto create_executor() const {
    constexpr auto strategy =
        determine<Requested, has_binding, scope_provides_references>();
    return executor_factory_.template create<strategy>();
  }

  explicit constexpr StrategyPolicy(
      ExecutorFactory executor_factory = ExecutorFactory{})
      : executor_factory_{std::move(executor_factory)} {}

 private:
  //! Determines resolution strategy based on requested type, binding, and scope
  template <typename Requested, bool has_binding,
            bool scope_provides_references>
  static constexpr auto determine() -> ResolutionStrategy {
    if constexpr (UniquePtr<Requested>) {
      return ResolutionStrategy::RelegateToTransient;
    } else if constexpr (SharedPtr<Requested> || WeakPtr<Requested>) {
      if constexpr (has_binding && !scope_provides_references) {
        // Transient scope with shared_ptr - let it create new instances
        return ResolutionStrategy::UseBoundScope;
      } else {
        // Singleton scope or no binding - wrap via canonical shared_ptr
        return ResolutionStrategy::CacheSharedPtr;
      }
    } else if constexpr (std::is_lvalue_reference_v<Requested> ||
                         std::is_pointer_v<Requested>) {
      if constexpr (has_binding) {
        return scope_provides_references
                   ? ResolutionStrategy::UseBoundScope
                   : ResolutionStrategy::PromoteToSingleton;
      } else {
        return ResolutionStrategy::PromoteToSingleton;
      }
    } else {
      // Value or RValue Ref
      if constexpr (has_binding) {
        return scope_provides_references
                   ? ResolutionStrategy::RelegateToTransient
                   : ResolutionStrategy::UseBoundScope;
      } else {
        return ResolutionStrategy::RelegateToTransient;
      }
    }
  }
};

// ----------------------------------------------------------------------------
// Dispatcher - Dispatches resolution requests to appropriate executors
// ----------------------------------------------------------------------------

template <typename StrategyPolicy = StrategyPolicy<>,
          typename BindingLookup = detail::BindingLookup,
          typename BindingFactory = detail::BindingFactory>
class Dispatcher {
  [[no_unique_address]] StrategyPolicy strategy_policy_{};
  [[no_unique_address]] BindingLookup lookup_policy_{};
  [[no_unique_address]] BindingFactory binding_factory_{};

  // Allow Container to access execute_with_default_binding for root container
  // case
  template <IsConfig, typename, typename>
  friend class Container;

 public:
  explicit Dispatcher(StrategyPolicy strategy_policy = StrategyPolicy{},
                      BindingLookup lookup_policy = {},
                      BindingFactory binding_factory = {})
      : strategy_policy_{std::move(strategy_policy)},
        lookup_policy_{std::move(lookup_policy)},
        binding_factory_{std::move(binding_factory)} {}

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

      return execute<Requested, has_binding, provides_references>(container,
                                                                  *binding_ptr);
    } else {
      // No binding - call handler (either creates default or delegates to
      // parent)
      return not_found_handler();
    }
  }

 private:
  //! Executes resolution with a specific binding
  template <typename Requested, bool has_binding,
            bool scope_provides_references, typename Container,
            typename Binding>
  auto execute(Container& container, Binding& binding)
      -> remove_rvalue_ref_t<Requested> {
    auto executor =
        strategy_policy_.template create_executor<Requested, has_binding,
                                                  scope_provides_references>();
    return executor.template execute<Requested>(container, binding);
  }

  //! Executes resolution with a default binding
  template <typename Requested, typename Container>
  auto execute_with_default_binding(Container& container)
      -> remove_rvalue_ref_t<Requested> {
    using Canonical = Canonical<Requested>;

    auto executor =
        strategy_policy_.template create_executor<Requested, false, false>();
    auto default_binding = binding_factory_.template create<Canonical>();

    return executor.template execute<Requested>(container, default_binding);
  }
};

// ----------------------------------------------------------------------------
// Concepts
// ----------------------------------------------------------------------------

template <typename Container>
concept IsContainer = requires(Container& container) {
  {
    container.template resolve<meta::ConceptProbe>()
  } -> std::same_as<meta::ConceptProbe>;
};

// ----------------------------------------------------------------------------
// Container - Forward Declaration
// ----------------------------------------------------------------------------

template <IsConfig Config, typename Dispatcher, typename Parent = void>
class Container;

// ----------------------------------------------------------------------------
// Container - Root Specialization (Parent = void)
// ----------------------------------------------------------------------------

template <IsConfig Config, typename Dispatcher>
class Container<Config, Dispatcher, void> {
  Config config_{};
  [[no_unique_address]] Dispatcher dispatcher_{};

 public:
  //! Resolve a dependency
  template <typename Requested>
  auto resolve() -> remove_rvalue_ref_t<Requested> {
    return dispatcher_.template resolve<Requested>(
        *this, config_,
        [this]<typename R = Requested>() -> remove_rvalue_ref_t<R> {
          return dispatcher_.template execute_with_default_binding<R>(*this);
        });
  }

  //! Construct from bindings
  template <IsBinding... Bindings>
  explicit Container(Bindings&&... bindings) noexcept
      : config_{make_bindings(std::forward<Bindings>(bindings)...)} {}

  //! Construct from config directly
  explicit Container(Config config) noexcept : config_{std::move(config)} {}

  //! Construct from config and dispatcher (for testing)
  explicit Container(Config config, Dispatcher dispatcher) noexcept
      : config_{std::move(config)}, dispatcher_{std::move(dispatcher)} {}
};

// ----------------------------------------------------------------------------
// Container - Child Specialization (Parent != void)
// ----------------------------------------------------------------------------

template <IsConfig Config, typename Dispatcher, typename Parent>
class Container {
  Config config_{};
  Parent* parent_;
  [[no_unique_address]] Dispatcher dispatcher_{};

 public:
  //! Resolve a dependency
  template <typename Requested>
  auto resolve() -> remove_rvalue_ref_t<Requested> {
    return dispatcher_.template resolve<Requested>(
        *this, config_, [this]() -> remove_rvalue_ref_t<Requested> {
          return parent_->template resolve<Requested>();
        });
  }

  //! Construct from parent and bindings
  template <IsBinding... Bindings>
  explicit Container(Parent& parent, Bindings&&... bindings) noexcept
      : config_{make_bindings(std::forward<Bindings>(bindings)...)},
        parent_{&parent} {}

  //! Construct from parent and config directly
  explicit Container(Parent& parent, Config config) noexcept
      : config_{std::move(config)}, parent_{&parent} {}

  //! Construct from parent, config, and dispatcher (for testing)
  explicit Container(Parent& parent, Config config,
                     Dispatcher dispatcher) noexcept
      : config_{std::move(config)},
        parent_{&parent},
        dispatcher_{std::move(dispatcher)} {}
};

// ----------------------------------------------------------------------------
// Deduction Guides
// ----------------------------------------------------------------------------

// Helper aliases for default pipeline components
namespace detail {
using DefaultDispatcher =
    Dispatcher<StrategyPolicy<>, BindingLookup, BindingFactory>;
}  // namespace detail

// Deduction guide - converts builders to Bindings, then deduces Config
template <typename... Builders>
Container(Builders&&...)
    -> Container<detail::ConfigFromTuple<
                     decltype(make_bindings(std::declval<Builders>()...))>,
                 detail::DefaultDispatcher, void>;

// Deduction guide for Config directly (root container)
template <IsConfig ConfigType>
Container(ConfigType) -> Container<ConfigType, detail::DefaultDispatcher, void>;

// Deduction guide for child container with parent and bindings
template <IsContainer ParentContainer, typename... Builders>
Container(ParentContainer&, Builders&&...)
    -> Container<detail::ConfigFromTuple<
                     decltype(make_bindings(std::declval<Builders>()...))>,
                 detail::DefaultDispatcher, ParentContainer>;

// Deduction guide for child container with parent and config
template <IsContainer ParentContainer, IsConfig ConfigType>
Container(ParentContainer&, ConfigType)
    -> Container<ConfigType, detail::DefaultDispatcher, ParentContainer>;

}  // namespace dink
