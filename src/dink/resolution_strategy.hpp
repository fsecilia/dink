// \file
// Copyright (c) 2025 Frank Secilia
// SPDX-License-Identifier: MIT

#pragma once

#include <dink/lib.hpp>
#include <dink/canonical.hpp>
#include <dink/remove_rvalue_ref.hpp>
#include <dink/scope.hpp>
#include <dink/smart_pointer_traits.hpp>
#include <memory>

namespace dink {
namespace detail {

//! Transitive provider for canonical shared_ptr caching.
//
// This provider resolves the reference indirectly by calling back into the
// container to resolve the original type, then wraps it in a shared_ptr.
template <typename Constructed>
struct SharedPtrFromRefProvider {
  using Provided = std::shared_ptr<Constructed>;

  template <typename Requested, typename Container>
  auto create(Container& container) -> std::shared_ptr<Constructed> {
    // Resolve reference using the container (follows delegation chain)
    auto& ref = container.template resolve<Constructed&>();
    // Wrap in shared_ptr with no-op deleter
    return std::shared_ptr<Constructed>{&ref, [](Constructed*) {}};
  }
};

//! Factory for creating CachedSharedPtrProvider instances.
struct SharedPtrFromRefProviderFactory {
  template <typename Canonical>
  auto create() const -> SharedPtrFromRefProvider<Canonical> {
    return {};
  }
};

}  // namespace detail

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
// Execution Implementations
// ----------------------------------------------------------------------------

namespace strategies {

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

}  // namespace strategies

// ----------------------------------------------------------------------------
// StrategySelector - Selects strategy by enum
// ----------------------------------------------------------------------------

template <ResolutionStrategy resolution_strategy>
struct StrategySelector;

template <>
struct StrategySelector<ResolutionStrategy::UseBoundScope>
    : strategies::UseBoundScope {};

template <>
struct StrategySelector<ResolutionStrategy::RelegateToTransient>
    : strategies::OverrideScope<scope::Transient> {};

template <>
struct StrategySelector<ResolutionStrategy::PromoteToSingleton>
    : strategies::OverrideScope<scope::Singleton> {};

template <>
struct StrategySelector<ResolutionStrategy::CacheSharedPtr>
    : strategies::CacheSharedPtr<scope::Singleton,
                                 detail::SharedPtrFromRefProviderFactory> {};

// ----------------------------------------------------------------------------
// Strategy Factory
// ----------------------------------------------------------------------------

template <template <ResolutionStrategy> typename StrategySelector>
struct StrategyFactory {
  template <ResolutionStrategy strategy>
  auto create() const -> StrategySelector<strategy> {
    return {};
  }
};

}  // namespace dink
