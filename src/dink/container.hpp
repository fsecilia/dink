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

template <IsConfig Config>
class Container {
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
    using BindingPtr = decltype(binding);
    constexpr auto found_binding = !std::is_same_v<BindingPtr, std::nullptr_t>;

    if constexpr (found_binding) {
      using Binding = std::remove_cvref_t<decltype(*binding)>;
      constexpr bool scope_provides_references =
          Binding::ScopeType::provides_references;
      constexpr bool scope_supports_values = !scope_provides_references;

      constexpr bool requested_is_lvalue_reference =
          std::is_lvalue_reference_v<Requested>;
      constexpr bool requested_is_pointer = std::is_pointer_v<Requested>;
      constexpr bool requested_is_smart_ptr =
          SharedPtr<Requested> || WeakPtr<Requested> || UniquePtr<Requested>;
      constexpr bool requested_is_value =
          !requested_is_lvalue_reference && !requested_is_pointer &&
          !requested_is_smart_ptr && !std::is_rvalue_reference_v<Requested>;

      // FIRST: Force unique_ptr to always use transient behavior
      if constexpr (UniquePtr<Requested>) {
        // Extract provider from binding, wrap in Transient scope
        auto transient = scope::Transient{};
        return transient.template resolve<Requested>(*this, binding->provider);
      }
      // Special case: shared_ptr/weak_ptr from reference-providing scope
      // Create a canonical shared_ptr that wraps the singleton instance
      else if constexpr ((SharedPtr<Requested> || WeakPtr<Requested>) &&
                         scope_provides_references) {
        return resolve_via_canonical_shared_ptr<Requested, Canonical>();
      }
      // Promotion: Scope can't provide references, but user wants them
      else if constexpr (!scope_provides_references &&
                         (requested_is_lvalue_reference ||
                          requested_is_pointer)) {
        // Use the bound provider with Singleton scope for caching
        auto singleton = scope::Singleton{};
        return singleton.template resolve<Requested>(*this, binding->provider);
      }
      // Relegation: Scope can't provide values, but user wants them
      // Create fresh instance using bound provider (symmetric with promotion)
      else if constexpr (!scope_supports_values && requested_is_value) {
        auto transient = scope::Transient{};
        return transient.template resolve<Requested>(*this, binding->provider);
      }
      // Normal case: delegate to bound scope with bound provider
      else {
        return binding->scope.template resolve<Requested>(*this,
                                                          binding->provider);
      }
    } else {
      // Delegate to default scope.
      return default_scope<Canonical>.template resolve<Requested>(
          *this, default_provider<Canonical>);
    }
  }

 private:
  Config config_{};

  // Default scope is Transient (creates fresh instances)
  template <typename Constructed>
  inline static auto default_scope = scope::Transient{};

  // Default provider uses Ctor
  template <typename Constructed>
  inline static auto default_provider = provider::Ctor<Constructed>{};

  // Transitive provider for canonical shared_ptr caching
  // This provider resolves the reference (using bound provider), then wraps it
  template <typename Constructed>
  struct TransitiveSingletonSharedPtrProvider {
    using Provided = std::shared_ptr<Constructed>;

    template <typename Requested, typename Container>
    auto create(Container& container) -> std::shared_ptr<Constructed> {
      // Resolve reference using the bound provider (through Container)
      auto& ref = container.template resolve<Constructed&>();
      // Wrap in shared_ptr with no-op deleter
      return std::shared_ptr<Constructed>{&ref, [](Constructed*) {}};
    }
  };

  // Helper to resolve shared_ptr/weak_ptr via canonical shared_ptr
  template <typename Requested, typename Canonical>
  auto resolve_via_canonical_shared_ptr() -> remove_rvalue_ref_t<Requested> {
    using Provider = TransitiveSingletonSharedPtrProvider<Canonical>;
    auto provider = Provider{};
    auto singleton = scope::Singleton{};
    return singleton.template resolve<Requested>(*this, provider);
  }
};

// Deduction guide - converts builders to Bindings, then deduces Config
template <typename... Builders>
Container(Builders&&...) -> Container<detail::ConfigFromTuple<
    decltype(make_bindings(std::declval<Builders>()...))>>;

// Deduction guide for Config directly
template <IsConfig ConfigType>
Container(ConfigType) -> Container<ConfigType>;

}  // namespace dink
