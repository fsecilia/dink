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
  Normal,             // Use binding's scope and provider
  ForceTransient,     // Override to transient
  ForceSingleton,     // Override to singleton (promotion)
  CanonicalSharedPtr  // Wrap via canonical shared_ptr
};

// ----------------------------------------------------------------------------
// Detail Helpers
// ----------------------------------------------------------------------------

namespace detail {

//! Determines resolution strategy based on requested type and binding status
template <typename Requested, bool has_binding, bool provides_references>
static constexpr auto determine_strategy() -> ResolutionStrategy {
  if constexpr (UniquePtr<Requested>) {
    return ResolutionStrategy::ForceTransient;
  } else if constexpr (SharedPtr<Requested> || WeakPtr<Requested>) {
    if constexpr (has_binding && !provides_references) {
      // Transient scope with shared_ptr - let it create new instances
      return ResolutionStrategy::Normal;
    } else {
      // Singleton scope or no binding - wrap via canonical shared_ptr
      return ResolutionStrategy::CanonicalSharedPtr;
    }
  } else if constexpr (std::is_lvalue_reference_v<Requested> ||
                       std::is_pointer_v<Requested>) {
    if constexpr (has_binding) {
      return provides_references ? ResolutionStrategy::Normal
                                 : ResolutionStrategy::ForceSingleton;
    } else {
      return ResolutionStrategy::ForceSingleton;
    }
  } else {
    // Value or RValue Ref
    if constexpr (has_binding) {
      return provides_references ? ResolutionStrategy::ForceTransient
                                 : ResolutionStrategy::Normal;
    } else {
      return ResolutionStrategy::ForceTransient;
    }
  }
}

//! Default policy for determining resolution strategy
struct DefaultStrategyPolicy {
  template <typename Requested, bool has_binding, bool provides_references>
  static constexpr auto determine() -> ResolutionStrategy {
    return determine_strategy<Requested, has_binding, provides_references>();
  }
};

//! Default policy for looking up bindings in config
struct DefaultBindingLookup {
  template <typename Canonical, typename Config>
  static auto find(Config& config) {
    return config.template find_binding<Canonical>();
  }
};

//! Creates a default binding when none exists
//
// Note: CanonicalSharedPtr strategy never calls this function.
template <typename Canonical, ResolutionStrategy strategy>
static constexpr auto make_default_binding() {
  if constexpr (strategy == ResolutionStrategy::ForceSingleton) {
    return Binding<Canonical, scope::Singleton, provider::Ctor<Canonical>>{
        scope::Singleton{}, provider::Ctor<Canonical>{}};
  } else if constexpr (strategy == ResolutionStrategy::ForceTransient) {
    return Binding<Canonical, scope::Transient, provider::Ctor<Canonical>>{
        scope::Transient{}, provider::Ctor<Canonical>{}};
  } else {
    static_assert(strategy == ResolutionStrategy::ForceSingleton ||
                      strategy == ResolutionStrategy::ForceTransient,
                  "make_default_binding called with unexpected strategy");
  }
}

//! Transitive provider for canonical shared_ptr caching
//
// This provider resolves the reference indirectly by calling back into the
// container to resolve the original type, then wraps it in a shared_ptr.
template <typename Constructed>
struct TransitiveSingletonSharedPtrProvider {
  using Provided = std::shared_ptr<Constructed>;

  template <typename Requested, typename Container>
  auto create(Container& container) -> std::shared_ptr<Constructed> {
    // Resolve reference using the container (follows delegation chain)
    auto& ref = container.template resolve<Constructed&>();
    // Wrap in shared_ptr with no-op deleter
    return std::shared_ptr<Constructed>{&ref, [](Constructed*) {}};
  }
};

//! Factory for creating TransitiveSingletonSharedPtrProvider instances
struct TransitiveSingletonSharedPtrProviderFactory {
  template <typename Canonical>
  auto create() const -> TransitiveSingletonSharedPtrProvider<Canonical> {
    return {};
  }
};

}  // namespace detail

// ----------------------------------------------------------------------------
// Execution Implementations
// ----------------------------------------------------------------------------

namespace execution {

//! Executes using binding's scope and provider directly
struct Normal {
  template <typename Requested, typename Container, typename Binding>
  auto execute(Container& container, Binding& binding) const
      -> remove_rvalue_ref_t<Requested> {
    return binding.scope.template resolve<Requested>(container,
                                                     binding.provider);
  }
};

//! Executes by forcing a specific scope type
template <typename Scope>
struct ForceScoped {
  [[no_unique_address]] Scope scope{};

  template <typename Requested, typename Container, typename Binding>
  auto execute(Container& container, Binding& binding) const
      -> remove_rvalue_ref_t<Requested> {
    return scope.template resolve<Requested>(container, binding.provider);
  }
};

//! Executes by wrapping reference in canonical shared_ptr
//
// Note: This strategy doesn't use a binding - it creates a new transitive
// provider that wraps a recursive resolution.
template <typename Scope, typename ProviderFactory>
struct CanonicalSharedPtr {
  [[no_unique_address]] Scope scope{};
  [[no_unique_address]] ProviderFactory provider_factory{};

  template <typename Requested, typename Container>
  auto execute(Container& container) const -> remove_rvalue_ref_t<Requested> {
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
struct StrategyExecutor<ResolutionStrategy::Normal> : execution::Normal {};

template <>
struct StrategyExecutor<ResolutionStrategy::ForceTransient>
    : execution::ForceScoped<scope::Transient> {};

template <>
struct StrategyExecutor<ResolutionStrategy::ForceSingleton>
    : execution::ForceScoped<scope::Singleton> {};

template <>
struct StrategyExecutor<ResolutionStrategy::CanonicalSharedPtr>
    : execution::CanonicalSharedPtr<
          scope::Singleton,
          detail::TransitiveSingletonSharedPtrProviderFactory> {};

// ----------------------------------------------------------------------------
// Strategy Executor Factory
// ----------------------------------------------------------------------------

template <template <ResolutionStrategy> typename StrategyExecutorTemplate>
struct DefaultStrategyExecutorFactory {
  template <ResolutionStrategy strategy>
  auto create() const -> StrategyExecutorTemplate<strategy> {
    return {};
  }
};

// ----------------------------------------------------------------------------
// Binding Resolver - Common binding lookup and execution logic
// ----------------------------------------------------------------------------

template <template <ResolutionStrategy> typename StrategyExecutorTemplate,
          typename ExecutorFactory,
          typename StrategyPolicy = detail::DefaultStrategyPolicy,
          typename BindingLookup = detail::DefaultBindingLookup>
class BindingResolver {
  [[no_unique_address]] ExecutorFactory executor_factory_{};
  [[no_unique_address]] StrategyPolicy strategy_policy_{};
  [[no_unique_address]] BindingLookup lookup_policy_{};

 public:
  explicit BindingResolver(ExecutorFactory executor_factory = {},
                           StrategyPolicy strategy_policy = {},
                           BindingLookup lookup_policy = {})
      : executor_factory_{std::move(executor_factory)},
        strategy_policy_{std::move(strategy_policy)},
        lookup_policy_{std::move(lookup_policy)} {}

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
      // Found binding - execute with determined strategy
      using Binding = std::remove_cvref_t<decltype(*binding_ptr)>;
      constexpr bool provides_references =
          Binding::ScopeType::provides_references;
      constexpr auto strategy =
          StrategyPolicy::template determine<Requested, has_binding,
                                             provides_references>();

      auto executor = executor_factory_.template create<strategy>();

      if constexpr (strategy == ResolutionStrategy::CanonicalSharedPtr) {
        // CanonicalSharedPtr doesn't use the binding
        return executor.template execute<Requested>(container);
      } else {
        // All other strategies execute with the binding
        return executor.template execute<Requested>(container, *binding_ptr);
      }
    } else {
      // No binding - call provided handler, passing factory for executor
      // creation
      return not_found_handler(executor_factory_);
    }
  }
};

// ----------------------------------------------------------------------------
// Flat Dispatcher (for root containers)
// ----------------------------------------------------------------------------

template <typename BindingResolver>
class FlatDispatcher {
  [[no_unique_address]] BindingResolver resolver_;

 public:
  explicit FlatDispatcher(BindingResolver resolver = BindingResolver{})
      : resolver_{std::move(resolver)} {}

  template <typename Requested, typename Container, typename Config>
  auto resolve(Container& container, Config& config)
      -> remove_rvalue_ref_t<Requested> {
    return resolver_.template resolve<Requested>(
        container, config,
        [&container](auto& factory) -> remove_rvalue_ref_t<Requested> {
          using Canonical = Canonical<Requested>;
          // Use the strategy policy that's part of the factory's resolver
          constexpr auto strategy =
              detail::DefaultStrategyPolicy::template determine<Requested,
                                                                false, false>();

          auto executor = factory.template create<strategy>();

          if constexpr (strategy == ResolutionStrategy::CanonicalSharedPtr) {
            // CanonicalSharedPtr doesn't need a binding
            return executor.template execute<Requested>(container);
          } else {
            // Other strategies need a default binding
            auto default_binding =
                detail::make_default_binding<Canonical, strategy>();
            return executor.template execute<Requested>(container,
                                                        default_binding);
          }
        });
  }
};

// ----------------------------------------------------------------------------
// Hierarchical Dispatcher (for child containers)
// ----------------------------------------------------------------------------

template <typename BindingResolver, typename ParentContainer>
class HierarchicalDispatcher {
  [[no_unique_address]] BindingResolver resolver_;

 public:
  explicit HierarchicalDispatcher(BindingResolver resolver = BindingResolver{})
      : resolver_{std::move(resolver)} {}

  template <typename Requested, typename Container, typename Config>
  auto resolve(Container& container, Config& config, ParentContainer& parent)
      -> remove_rvalue_ref_t<Requested> {
    return resolver_.template resolve<Requested>(
        container, config, [&parent](auto&) -> remove_rvalue_ref_t<Requested> {
          // Delegate to parent - don't need factory for this
          return parent.template resolve<Requested>();
        });
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
    return dispatcher_.template resolve<Requested>(*this, config_);
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
    return dispatcher_.template resolve<Requested>(*this, config_, *parent_);
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
using DefaultExecutorFactory = DefaultStrategyExecutorFactory<StrategyExecutor>;
using DefaultResolver =
    BindingResolver<StrategyExecutor, DefaultExecutorFactory,
                    DefaultStrategyPolicy, DefaultBindingLookup>;
using DefaultFlatDispatcher = FlatDispatcher<DefaultResolver>;
}  // namespace detail

// Deduction guide - converts builders to Bindings, then deduces Config
template <typename... Builders>
Container(Builders&&...)
    -> Container<detail::ConfigFromTuple<
                     decltype(make_bindings(std::declval<Builders>()...))>,
                 detail::DefaultFlatDispatcher, void>;

// Deduction guide for Config directly (root container)
template <IsConfig ConfigType>
Container(ConfigType)
    -> Container<ConfigType, detail::DefaultFlatDispatcher, void>;

// Deduction guide for child container with parent and bindings
template <IsContainer ParentContainer, typename... Builders>
Container(ParentContainer&, Builders&&...) -> Container<
    detail::ConfigFromTuple<
        decltype(make_bindings(std::declval<Builders>()...))>,
    HierarchicalDispatcher<detail::DefaultResolver, ParentContainer>,
    ParentContainer>;

// Deduction guide for child container with parent and config
template <IsContainer ParentContainer, IsConfig ConfigType>
Container(ParentContainer&, ConfigType) -> Container<
    ConfigType,
    HierarchicalDispatcher<detail::DefaultResolver, ParentContainer>,
    ParentContainer>;

}  // namespace dink
