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

    if constexpr (auto binding = config_.template find_binding<Canonical>();
                  !std::is_same_v<decltype(binding), std::nullptr_t>) {
      // found binding

      using Binding = std::remove_cvref_t<decltype(*binding)>;
      constexpr auto scope_provides_references =
          Binding::ScopeType::provides_references;

      if constexpr (UniquePtr<Requested>) {
        // UniquePtr: Always force transient behavior
        const auto transient = scope::Transient{};
        return transient.template resolve<Requested>(*this, binding->provider);
      } else if constexpr (SharedPtr<Requested> || WeakPtr<Requested>) {
        // SharedPtr / WeakPtr
        if constexpr (scope_provides_references) {
          // Bound to a ref-scope (e.g., singleton). Wrap the ref.
          return resolve_via_canonical_shared_ptr<Requested, Canonical>();
        } else {
          // Bound to a value-scope (e.g., transient). Let it resolve.
          return binding->scope.template resolve<Requested>(*this,
                                                            binding->provider);
        }
      } else if constexpr (std::is_lvalue_reference_v<Requested> ||
                           std::is_pointer_v<Requested>) {
        // LValue Ref / Pointer
        if constexpr (scope_provides_references) {
          // Bound to a ref-scope. Normal case.
          return binding->scope.template resolve<Requested>(*this,
                                                            binding->provider);
        } else {
          // "Promotion": Bound to a value-scope, but ref wanted.
          // Promote to singleton to cache the value.
          const auto singleton = scope::Singleton{};
          return singleton.template resolve<Requested>(*this,
                                                       binding->provider);
        }
      } else {
        // Value Types (and rvalue refs)
        if constexpr (scope_provides_references) {
          // "Relegation": Bound to a ref-scope, but value wanted.
          // Create a fresh instance.
          const auto transient = scope::Transient{};
          return transient.template resolve<Requested>(*this,
                                                       binding->provider);
        } else {
          // Bound to a value-scope. Normal case.
          return binding->scope.template resolve<Requested>(*this,
                                                            binding->provider);
        }
      }
    } else {
      // no binding found

      if constexpr (SharedPtr<Requested> || WeakPtr<Requested>) {
        // SharedPtr / WeakPtr: Always cached
        return resolve_via_canonical_shared_ptr<Requested, Canonical>();
      } else if constexpr (std::is_lvalue_reference_v<Requested> ||
                           std::is_pointer_v<Requested>) {
        // LValue Ref / Pointer: Always cached
        const auto singleton = scope::Singleton{};
        return singleton.template resolve<Requested>(
            *this, default_provider<Canonical>);
      } else {
        // Value Types: Always transient
        const auto transient = scope::Transient{};
        return transient.template resolve<Requested>(
            *this, default_provider<Canonical>);
      }
    }
  }

 private:
  Config config_{};

  // Default provider uses Ctor
  template <typename Constructed>
  inline static auto default_provider = provider::Ctor<Constructed>{};

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
