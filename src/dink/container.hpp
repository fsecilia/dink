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
      constexpr bool requested_is_lvalue_reference =
          std::is_lvalue_reference_v<Requested>;
      constexpr bool requested_is_pointer = std::is_pointer_v<Requested>;
      constexpr bool requested_is_value =
          !requested_is_lvalue_reference && !requested_is_pointer &&
          !SharedPtr<Requested> && !WeakPtr<Requested> &&
          !std::is_rvalue_reference_v<Requested>;

      // Special case: shared_ptr/weak_ptr from reference-providing scope
      if constexpr ((SharedPtr<Requested> || WeakPtr<Requested>) &&
                    scope_provides_references) {
        return resolve_via_transitive_shared_ptr_binding<Requested,
                                                         Canonical>();
      }
      // Promotion: Transient asked for reference/pointer → cache it in
      // Singleton
      else if constexpr (!scope_provides_references &&
                         (requested_is_lvalue_reference ||
                          requested_is_pointer)) {
        return resolve_via_transitive_singleton_binding<Requested, Canonical>();
      }
      // Relegation: Singleton asked for value → create new in Transient
      else if constexpr (scope_provides_references && requested_is_value) {
        return resolve_via_transitive_transient_binding<Requested, Canonical>();
      }
      // Normal case: delegate to bound scope
      else {
        return binding->scope.template resolve<Requested>(*this);
      }
    } else {
      // Delegate to default scope.
      return default_scope<Canonical>.template resolve<Requested>(*this);
    }
  }

 private:
  Config config_{};

  template <typename Constructed>
  inline static auto default_scope =
      scope::Deduced<provider::Ctor<Constructed>>{
          provider::Ctor<Constructed>{}};

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

  // Transitive Singleton for promotion: caches value from Transient
  template <typename Constructed>
  struct TransitiveSingletonProvider {
    using Provided = Constructed;

    template <typename Requested, typename Container>
    auto create(Container& container) -> Requested {
      // Resolve as value from the transient binding
      return container.template resolve<Constructed>();
    }
  };

  // Transitive Transient for relegation: creates value from Singleton
  template <typename Constructed>
  struct TransitiveTransientProvider {
    using Provided = Constructed;

    template <typename Requested, typename Container>
    auto create(Container& container) -> Requested {
      // Get reference from singleton, copy it to create value
      return container.template resolve<Constructed&>();
    }
  };

  template <typename Canonical>
  constexpr auto make_shared_ptr_transitive_binding() {
    using Provider = TransitiveSingletonSharedPtrProvider<Canonical>;
    return Binding<Canonical, scope::Singleton<Provider>>{
        scope::Singleton{Provider{}}};
  }

  template <typename Canonical>
  constexpr auto make_singleton_transitive_binding() {
    using Provider = TransitiveSingletonProvider<Canonical>;
    return Binding<Canonical, scope::Singleton<Provider>>{
        scope::Singleton{Provider{}}};
  }

  template <typename Canonical>
  constexpr auto make_transient_transitive_binding() {
    using Provider = TransitiveTransientProvider<Canonical>;
    return Binding<Canonical, scope::Transient<Provider>>{
        scope::Transient{Provider{}}};
  }

  template <typename Requested, typename Canonical>
  auto resolve_via_transitive_shared_ptr_binding()
      -> remove_rvalue_ref_t<Requested> {
    return make_shared_ptr_transitive_binding<Canonical>()
        .scope.template resolve<Requested>(*this);
  }

  template <typename Requested, typename Canonical>
  auto resolve_via_transitive_singleton_binding()
      -> remove_rvalue_ref_t<Requested> {
    return make_singleton_transitive_binding<Canonical>()
        .scope.template resolve<Requested>(*this);
  }

  template <typename Requested, typename Canonical>
  auto resolve_via_transitive_transient_binding()
      -> remove_rvalue_ref_t<Requested> {
    return make_transient_transitive_binding<Canonical>()
        .scope.template resolve<Requested>(*this);
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
