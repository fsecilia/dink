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

// ----------------------------------------------------------------------------
// Strategies
// ----------------------------------------------------------------------------

namespace strategies {
namespace implementations {

//! Uses provider from binding, but overrides scope.
template <typename Scope>
struct OverrideScope {
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
struct OverrideBinding {
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

namespace non_owning_shared_ptr {

//! \brief Creates a shared_ptr to a container-managed object.
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

}  // namespace non_owning_shared_ptr
}  // namespace implementations

//! Resolves using binding directly, with no overrides.
struct UseBinding {
  template <typename Requested, typename Container, typename Binding>
  auto execute(Container& container, Binding& binding) const
      -> meta::RemoveRvalueRef<Requested> {
    return binding.scope.template resolve<Requested>(container,
                                                     binding.provider);
  }
};

//! Overrides scope with singleton.
using PromoteToSingleton = implementations::OverrideScope<scope::Singleton>;

//! Wraps cached reference in a non-owning shared_ptr.
//
// When resolving a shared_ptr to a reference, the bound scope and provider are
// used indirectly by recursing into the container to get the reference first.
// Then the shared_ptr is set up to point at the reference with a no-op
// deleter. Recursing is performed by overriding both the scope and provider.
using CacheSharedPtr = implementations::OverrideBinding<
    scope::Singleton,
    implementations::non_owning_shared_ptr::ProviderFactory<>>;

}  // namespace strategies

// ----------------------------------------------------------------------------
// StrategyFactory
// ----------------------------------------------------------------------------

//! Compile-time dispatcher for dependency resolution strategies.
struct StrategyFactory {
  //! Instantiates strategy chosen by dispatch logic.
  //
  // This function is a decision tree based on the requested type, whether a
  // binding was found or not, and whether the scope provides references or
  // transient values.
  //
  // \returns strategies of varying type
  // The return type varies with the strategy chosen. The strategies types
  // themselves are unrelated.
  template <typename Requested, bool found_binding,
            bool scope_provides_references>
  constexpr auto create() const noexcept -> auto {
    if constexpr (meta::IsSharedPtr<Requested> || meta::IsWeakPtr<Requested>) {
      // shared_ptr or weak_ptr.
      if constexpr (meta::IsSharedPtr<Requested> && found_binding &&
                    !scope_provides_references) {
        // shared_ptr bound transient; use the binding.
        return strategies::UseBinding{};
      } else {
        // weak_ptr, reference scope, or binding not found; cache shared_ptr.
        return strategies::CacheSharedPtr{};
      }
    } else if constexpr (std::is_lvalue_reference_v<Requested> ||
                         std::is_pointer_v<Requested>) {
      // Lvalue ref or pointer.
      if constexpr (found_binding && scope_provides_references) {
        // Already bound to reference scope; use the binding.
        return strategies::UseBinding{};
      } else {
        // Binding not found or a value-providing scope; must promote.
        return strategies::PromoteToSingleton{};
      }
    } else {
      // Value, rvalue ref, or unique_ptr.
      return strategies::UseBinding{};
    }
  }
};

}  // namespace dink
