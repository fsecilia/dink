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
// Detail Helpers (shared by all dispatchers)
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

//! Creates a default binding when none exists
template <typename Canonical, ResolutionStrategy strategy>
static constexpr auto make_default_binding() {
  if constexpr (strategy == ResolutionStrategy::ForceSingleton) {
    return Binding<Canonical, scope::Singleton, provider::Ctor<Canonical>>{
        scope::Singleton{}, provider::Ctor<Canonical>{}};
  } else if constexpr (strategy == ResolutionStrategy::ForceTransient) {
    return Binding<Canonical, scope::Transient, provider::Ctor<Canonical>>{
        scope::Transient{}, provider::Ctor<Canonical>{}};
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

}  // namespace detail

// ----------------------------------------------------------------------------
// Strategy Executor
// ----------------------------------------------------------------------------

//! Executes a resolution strategy with a binding
template <ResolutionStrategy strategy>
struct StrategyExecutor {
  template <typename Requested, typename Container, typename Binding>
  static auto execute(Container& container, Binding& binding)
      -> remove_rvalue_ref_t<Requested> {
    if constexpr (strategy == ResolutionStrategy::Normal) {
      return binding.scope.template resolve<Requested>(container,
                                                       binding.provider);
    } else if constexpr (strategy == ResolutionStrategy::ForceTransient) {
      const auto transient = scope::Transient{};
      return transient.template resolve<Requested>(container, binding.provider);
    } else if constexpr (strategy == ResolutionStrategy::ForceSingleton) {
      const auto singleton = scope::Singleton{};
      return singleton.template resolve<Requested>(container, binding.provider);
    } else if constexpr (strategy == ResolutionStrategy::CanonicalSharedPtr) {
      // Resolve reference through container, wrap in shared_ptr
      using Canonical = Canonical<Requested>;
      using Provider = detail::TransitiveSingletonSharedPtrProvider<Canonical>;
      auto provider = Provider{};
      const auto singleton = scope::Singleton{};
      return singleton.template resolve<Requested>(container, provider);
    }
  }
};

// ----------------------------------------------------------------------------
// Flat Dispatcher (for root containers)
// ----------------------------------------------------------------------------

template <template <ResolutionStrategy> typename StrategyExecutorTemplate>
class FlatDispatcher {
 public:
  template <typename Requested, typename Container, typename Config>
  static auto resolve(Container& container, Config& config)
      -> remove_rvalue_ref_t<Requested> {
    using Canonical = Canonical<Requested>;

    auto binding_ptr = config.template find_binding<Canonical>();
    constexpr bool has_binding =
        !std::is_same_v<decltype(binding_ptr), std::nullptr_t>;

    if constexpr (has_binding) {
      using Binding = std::remove_cvref_t<decltype(*binding_ptr)>;
      constexpr bool provides_references =
          Binding::ScopeType::provides_references;
      constexpr auto strategy =
          detail::determine_strategy<Requested, has_binding,
                                     provides_references>();

      return StrategyExecutorTemplate<strategy>::template execute<Requested>(
          container, *binding_ptr);
    } else {
      constexpr auto strategy =
          detail::determine_strategy<Requested, has_binding, false>();

      auto default_binding =
          detail::make_default_binding<Canonical, strategy>();
      return StrategyExecutorTemplate<strategy>::template execute<Requested>(
          container, default_binding);
    }
  }
};

// ----------------------------------------------------------------------------
// Hierarchical Dispatcher (for child containers)
// ----------------------------------------------------------------------------

template <template <ResolutionStrategy> typename StrategyExecutorTemplate,
          typename ParentContainer>
class HierarchicalDispatcher {
 public:
  template <typename Requested, typename Container, typename Config>
  static auto resolve(Container& container, Config& config,
                      ParentContainer& parent)
      -> remove_rvalue_ref_t<Requested> {
    using Canonical = Canonical<Requested>;

    auto binding_ptr = config.template find_binding<Canonical>();
    constexpr bool has_binding =
        !std::is_same_v<decltype(binding_ptr), std::nullptr_t>;

    if constexpr (has_binding) {
      // Found locally - resolve here
      using Binding = std::remove_cvref_t<decltype(*binding_ptr)>;
      constexpr bool provides_references =
          Binding::ScopeType::provides_references;
      constexpr auto strategy =
          detail::determine_strategy<Requested, has_binding,
                                     provides_references>();

      return StrategyExecutorTemplate<strategy>::template execute<Requested>(
          container, *binding_ptr);
    } else {
      // Not found locally - delegate to parent
      return parent.template resolve<Requested>();
    }
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

template <IsConfig Config, typename Parent = void,
          template <ResolutionStrategy> typename StrategyExecutorTemplate =
              StrategyExecutor>
class Container;

// ----------------------------------------------------------------------------
// Container - Root Specialization (Parent = void)
// ----------------------------------------------------------------------------

template <IsConfig Config,
          template <ResolutionStrategy> typename StrategyExecutorTemplate>
class Container<Config, void, StrategyExecutorTemplate> {
 private:
  using Dispatcher = FlatDispatcher<StrategyExecutorTemplate>;
  Config config_{};

 public:
  //! Resolve a dependency
  template <typename Requested>
  auto resolve() -> remove_rvalue_ref_t<Requested> {
    return Dispatcher::template resolve<Requested>(*this, config_);
  }

  //! Construct from bindings
  template <IsBinding... Bindings>
  explicit Container(Bindings&&... bindings) noexcept
      : config_{make_bindings(std::forward<Bindings>(bindings)...)} {}

  //! Construct from config directly
  explicit Container(Config config) noexcept : config_{std::move(config)} {}
};

// ----------------------------------------------------------------------------
// Container - Child Specialization (Parent != void)
// ----------------------------------------------------------------------------

template <IsConfig Config, typename Parent,
          template <ResolutionStrategy> typename StrategyExecutorTemplate>
class Container {
 private:
  using Dispatcher = HierarchicalDispatcher<StrategyExecutorTemplate, Parent>;
  Config config_{};
  Parent* parent_;

 public:
  //! Resolve a dependency
  template <typename Requested>
  auto resolve() -> remove_rvalue_ref_t<Requested> {
    return Dispatcher::template resolve<Requested>(*this, config_, *parent_);
  }

  //! Construct from parent and bindings
  template <IsBinding... Bindings>
  explicit Container(Parent& parent, Bindings&&... bindings) noexcept
      : config_{make_bindings(std::forward<Bindings>(bindings)...)},
        parent_{&parent} {}

  //! Construct from parent and config directly
  explicit Container(Parent& parent, Config config) noexcept
      : config_{std::move(config)}, parent_{&parent} {}
};

// ----------------------------------------------------------------------------
// Deduction Guides
// ----------------------------------------------------------------------------

// Deduction guide - converts builders to Bindings, then deduces Config
template <typename... Builders>
Container(Builders&&...)
    -> Container<detail::ConfigFromTuple<
                     decltype(make_bindings(std::declval<Builders>()...))>,
                 void, StrategyExecutor>;

// Deduction guide for Config directly (root container)
template <IsConfig ConfigType>
Container(ConfigType) -> Container<ConfigType, void, StrategyExecutor>;

// Deduction guide for child container with parent and bindings
template <IsContainer ParentContainer, typename... Builders>
Container(ParentContainer&, Builders&&...)
    -> Container<detail::ConfigFromTuple<
                     decltype(make_bindings(std::declval<Builders>()...))>,
                 ParentContainer, StrategyExecutor>;

// Deduction guide for child container with parent and config
template <IsContainer ParentContainer, IsConfig ConfigType>
Container(ParentContainer&, ConfigType)
    -> Container<ConfigType, ParentContainer, StrategyExecutor>;

}  // namespace dink
