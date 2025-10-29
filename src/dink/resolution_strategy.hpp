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

// ----------------------------------------------------------------------------
// Resolution Strategies - Implementation Details
// ----------------------------------------------------------------------------

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

//! Factory for creating SharedPtrFromRefProvider instances.
struct SharedPtrFromRefProviderFactory {
  template <typename Canonical>
  auto create() const -> SharedPtrFromRefProvider<Canonical> {
    return {};
  }
};

namespace strategies {

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
struct CacheSharedPtrImpl {
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
}  // namespace detail

// ----------------------------------------------------------------------------
// Resolution Strategies - Public Interface
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

//! Executes by forcing transient scope
using RelegateToTransient = detail::strategies::OverrideScope<scope::Transient>;

//! Executes by forcing singleton scope
using PromoteToSingleton = detail::strategies::OverrideScope<scope::Singleton>;

//! Executes by wrapping reference in canonical shared_ptr
using CacheSharedPtr = detail::strategies::CacheSharedPtrImpl<
    scope::Singleton, detail::SharedPtrFromRefProviderFactory>;

}  // namespace strategies
}  // namespace dink
