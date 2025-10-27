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

    auto binding_ptr = config_.template find_binding<Canonical>();
    constexpr bool has_binding =
        !std::is_same_v<decltype(binding_ptr), std::nullptr_t>;

    if constexpr (has_binding) {
      using Binding = std::remove_cvref_t<decltype(*binding_ptr)>;
      constexpr bool provides_references =
          Binding::ScopeType::provides_references;
      constexpr auto strategy =
          determine_strategy<Requested, has_binding, provides_references>();

      if constexpr (strategy == ResolutionStrategy::CanonicalSharedPtr) {
        return resolve_via_canonical_shared_ptr<Requested, Canonical>();
      } else {
        return execute_with_binding<Requested, strategy>(*binding_ptr);
      }
    } else {
      constexpr auto strategy =
          determine_strategy<Requested, has_binding, false>();

      if constexpr (strategy == ResolutionStrategy::CanonicalSharedPtr) {
        return resolve_via_canonical_shared_ptr<Requested, Canonical>();
      } else {
        auto default_binding = make_default_binding<Canonical, strategy>();
        return execute_with_binding<Requested, strategy>(default_binding);
      }
    }
  }

 private:
  Config config_{};

  enum class ResolutionStrategy {
    Normal,             // Use binding's scope and provider
    ForceTransient,     // Override to transient
    ForceSingleton,     // Override to singleton (promotion)
    CanonicalSharedPtr  // Wrap via canonical shared_ptr
  };

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

  // Create a default binding when none exists
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

  // Execute strategy with a binding (found or default)
  template <typename Requested, ResolutionStrategy strategy, typename Binding>
  auto execute_with_binding(Binding& binding)
      -> remove_rvalue_ref_t<Requested> {
    if constexpr (strategy == ResolutionStrategy::Normal) {
      return binding.scope.template resolve<Requested>(*this, binding.provider);
    } else if constexpr (strategy == ResolutionStrategy::ForceTransient) {
      const auto transient = scope::Transient{};
      return transient.template resolve<Requested>(*this, binding.provider);
    } else if constexpr (strategy == ResolutionStrategy::ForceSingleton) {
      const auto singleton = scope::Singleton{};
      return singleton.template resolve<Requested>(*this, binding.provider);
    }
  }

  // Transitive provider for canonical shared_ptr caching
  // This provider resolves the reference indirectly using bound provider by
  // calling back into the container to resolve the original type, then wraps it
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
    const auto singleton = scope::Singleton{};
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
