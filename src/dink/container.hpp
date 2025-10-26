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
      else if constexpr ((SharedPtr<Requested> || WeakPtr<Requested>) &&
                         scope_provides_references) {
        return resolve_via_transitive_shared_ptr_binding<Requested,
                                                         Canonical>();
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
      else if constexpr (!scope_supports_values && requested_is_value) {
        return resolve_via_transitive_transient_binding<Requested, Canonical>();
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

  // Transitive Singleton for promotion: wraps shared_ptr around reference
  template <typename Constructed>
  struct TransitiveSingletonSharedPtrProvider {
    using Provided = std::shared_ptr<Constructed>;

    template <typename Requested, typename Container>
    auto create(Container& container) -> std::shared_ptr<Constructed> {
      auto& ref = container.template resolve<Constructed&>();
      return std::shared_ptr<Constructed>{&ref, [](Constructed*) {}};
    }
  };

  // Transitive Transient for relegation: creates value from Singleton
  template <typename Canonical>
  struct TransitiveTransientProvider {
    using Provided = Canonical;

    template <typename Requested, typename Container>
    auto create(Container& container) -> Requested {
      // Get reference from singleton, copy it to create value
      auto& ref = container.template resolve<Canonical&>();
      return ref;
    }
  };

  template <typename Requested, typename Canonical>
  auto resolve_via_transitive_shared_ptr_binding()
      -> remove_rvalue_ref_t<Requested> {
    using Provider = TransitiveSingletonSharedPtrProvider<Canonical>;
    auto provider = Provider{};
    auto singleton = scope::Singleton{};
    return singleton.template resolve<Requested>(*this, provider);
  }

  template <typename Requested, typename Canonical>
  auto resolve_via_transitive_transient_binding()
      -> remove_rvalue_ref_t<Requested> {
    using Provider = TransitiveTransientProvider<Canonical>;
    auto provider = Provider{};
    auto transient = scope::Transient{};
    return transient.template resolve<Requested>(*this, provider);
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
