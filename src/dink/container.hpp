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
// Concepts
// ----------------------------------------------------------------------------

template <typename Container>
concept IsContainer = requires(Container& container) {
  {
    container.template resolve<meta::ConceptProbe>()
  } -> std::same_as<meta::ConceptProbe>;
};

template <IsConfig Config, typename Parent>
class Container;

template <IsConfig Config>
class Container<Config, void> {
 public:
  //! constructs root container with given bindings
  template <IsBinding... Bindings>
  explicit Container(Bindings&&... bindings) noexcept
      : config_{make_bindings(std::forward<Bindings>(bindings)...)} {}

  //! direct construction from components
  Container(Config config) noexcept : config_{std::move(config)} {}

  template <typename Requested>
  auto resolve() -> remove_rvalue_ref_t<Requested> {
    using Canonical = Canonical<Requested>;
    auto binding = config_.template find_binding<Canonical>();
    using Binding = decltype(binding);
    constexpr auto found_binding = !std::is_same_v<Binding, std::nullptr_t>;
    if constexpr (found_binding) {
      // Handle canonical shared_ptr.
      if constexpr (SharedPtr<Requested> || WeakPtr<Requested>) {
        if constexpr (Binding::ScopeType::provides_references) {
          return resolve_via_transitive_binding<Requested, Canonical>();
        }
      }

      // Delegate to bound scope.
      return binding.scope.template resolve<Requested>(*this);
    }

    // Delegate to default scope.
    return default_scope<Canonical>.template resolve<Requested>(*this);
  }

 private:
  Config config_{};

  template <typename Constructed>
  static constexpr auto default_scope =
      scope::Deduced<provider::Ctor<Constructed>>{{}};

  template <typename Constructed>
  struct TransitiveSingletonSharedPtrProvider {
    auto create(Container& container) -> std::shared_ptr<Constructed> {
      auto& ref = container.template resolve<Constructed&>(container);
      return std::shared_ptr<Constructed>{&ref, [](Constructed*) {}};
    }
  };

  template <typename Constructed>
  static constexpr auto transitive_binding =
      Binding<Constructed, scope::Singleton<provider::Ctor<Constructed>>>{
          scope::Singleton{TransitiveSingletonSharedPtrProvider{}}};

  template <typename Requested, typename Canonical>
  auto resolve_via_transitive_binding() -> remove_rvalue_ref_t<Requested> {
    return transitive_binding<Canonical>.scope.template resolve<Requested>(
        *this);
  }
};

}  // namespace dink
