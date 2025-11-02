// \file
// Copyright (c) 2025 Frank Secilia
// SPDX-License-Identifier: MIT
//
// \brief Contextual strategies to produce or locate instances.

#pragma once

#include <dink/lib.hpp>
#include <dink/canonical.hpp>
#include <dink/meta.hpp>
#include <dink/scope.hpp>
#include <memory>

namespace dink {
namespace strategy_impls {

//! Uses scope and provider from binding directly.
struct BoundScopeWithBoundProvider {
  template <typename Requested, typename Container, typename Binding>
  auto execute(Container& container, Binding& binding) const
      -> meta::RemoveRvalueRef<Requested> {
    return binding.scope.template resolve<Requested>(container,
                                                     binding.provider);
  }
};

//! Uses provider from binding, but overrides scope.
template <typename Scope>
struct LocalScopeWithBoundProvider {
  [[dink_no_unique_address]] Scope scope{};

  //! Resolves local scope using provider from given binding.
  template <typename Requested, typename Container, typename Binding>
  auto execute(Container& container, Binding& binding) const
      -> meta::RemoveRvalueRef<Requested> {
    return scope.template resolve<Requested>(container, binding.provider);
  }
};

//! Overrides scope and provider.
template <typename Scope, typename ProviderFactory>
struct LocalScopeWithLocalProvider {
  [[dink_no_unique_address]] Scope scope{};
  [[dink_no_unique_address]] ProviderFactory provider_factory{};

  //! Resolves local scope using provider from local factory.
  template <typename Requested, typename Container, typename Binding>
  auto execute(Container& container, Binding&) const
      -> meta::RemoveRvalueRef<Requested> {
    using Canonical = Canonical<Requested>;
    auto provider = provider_factory.template create<Canonical>();
    return scope.template resolve<Requested>(container, provider);
  }
};

}  // namespace strategy_impls

namespace aliasing_shared_ptr {

//! \brief Creates a shared_ptr aliased to a container-managed object.
//
// The shared_ptr does not alias another shared_ptr, but a reference that the
// container owns. It doesn't use the aliasing constructor; it uses a no-op
// deleter.
template <typename Constructed>
struct Provider {
  using Provided = std::shared_ptr<Constructed>;

  //! \brief Resolves a reference and wraps it in a non-owning shared_ptr.
  //
  // This provider resolves the object as a cached reference from the
  // container, then wraps that reference in a std::shared_ptr with a no-op
  // deleter.
  template <typename Requested, typename Container>
  auto create(Container& container) -> std::shared_ptr<Constructed> {
    static_assert(meta::IsSharedPtr<Requested> || meta::IsWeakPtr<Requested>);

    // Resolve reference naturally using the container.
    auto& ref = container.template resolve<Constructed&>();

    // Wrap reference in shared_ptr with no-op deleter.
    return std::shared_ptr<Constructed>{&ref, [](Constructed*) {}};
  }
};

//! Substitutable factory for creating Provider instances.
template <template <typename> class Provider = Provider>
struct ProviderFactory {
  template <typename Canonical>
  constexpr auto create() const -> Provider<Canonical> {
    return {};
  }
};

}  // namespace aliasing_shared_ptr

// ----------------------------------------------------------------------------
// Strategies
// ----------------------------------------------------------------------------

namespace strategies {

//! Resolves using binding directly, with no overrides.
using UseBinding = strategy_impls::BoundScopeWithBoundProvider;

//! Overrides scope with transient.
using RelegateToTransient = strategy_impls::BoundScopeWithBoundProvider;

//! Overrides scope with singleton.
using PromoteToSingleton =
    strategy_impls::LocalScopeWithBoundProvider<scope::Singleton>;

//! Wraps cached reference in a aliased shared_ptr.
//
// When resolving a shared_ptr to a reference, the bound scope and provider are
// used indirectly by recursing into the container. Recursing is performed by
// overriding both the scope and provider.
using CacheSharedPtr = strategy_impls::LocalScopeWithLocalProvider<
    scope::Singleton, aliasing_shared_ptr::ProviderFactory<>>;

}  // namespace strategies

// ----------------------------------------------------------------------------
// StrategyFactory
// ----------------------------------------------------------------------------

//! Compile-time dispatcher for dependency resolution strategies.
struct StrategyFactory {
  //! Instantiates strategy chosen by dispatch logic.
  //
  // This function is one big decision tree based on the requested type,
  // whether a binding was found or not, and whether the scope provides
  // references or transient values.
  //
  // This is where promotion and relegation are decided.
  //
  // \returns strategy of varying types
  // The return type varies with the strategy chosen. The strategy types
  // themselves are unrelated.
  template <typename Requested, bool has_binding,
            bool scope_provides_references>
  constexpr auto create() const noexcept -> auto {
    if constexpr (meta::IsUniquePtr<Requested>) {
      // unique_ptr; always transient. Relegate if necessary.
      return strategies::RelegateToTransient{};
    } else if constexpr (meta::IsSharedPtr<Requested> ||
                         meta::IsWeakPtr<Requested>) {
      // shared_ptr or weak_ptr.
      if constexpr (meta::IsSharedPtr<Requested> && has_binding &&
                    !scope_provides_references) {
        // shared_ptr bound transient; use the binding.
        return strategies::UseBinding{};
      } else {
        // weak_ptr, reference scope, or no binding; cache shared_ptr.
        return strategies::CacheSharedPtr{};
      }
    } else if constexpr (std::is_lvalue_reference_v<Requested> ||
                         std::is_pointer_v<Requested>) {
      // Lvalue ref or pointer.
      if constexpr (has_binding && scope_provides_references) {
        // Already bound to reference scope; use the binding.
        return strategies::UseBinding{};
      } else {
        // No binding or a value-providing scope; must promote.
        return strategies::PromoteToSingleton{};
      }
    } else {
      // Value or rvalue ref.
      if constexpr (has_binding) {
        // Use binding; will copy singleton or use transient.
        return strategies::UseBinding{};
      } else {
        // No binding; must relegate.
        return strategies::RelegateToTransient{};
      }
    }
  }
};

}  // namespace dink
