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

      if constexpr ((SharedPtr<Requested> || WeakPtr<Requested>) &&
                    Binding::ScopeType::provides_references) {
        // Handle canonical shared_ptr.
        return resolve_via_transitive_binding<Requested, Canonical>();
      } else {
        // Delegate to bound scope.
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

  template <typename Constructed>
  struct TransitiveSingletonSharedPtrProvider {
    using Provided = std::shared_ptr<Constructed>;

    template <typename Requested, typename Container>
    auto create(Container& container) -> std::shared_ptr<Constructed> {
      auto& ref = container.template resolve<Constructed&>();
      return std::shared_ptr<Constructed>{&ref, [](Constructed*) {}};
    }
  };

  template <typename Constructed>
  inline static auto transitive_binding = Binding<
      Constructed,
      scope::Singleton<TransitiveSingletonSharedPtrProvider<Constructed>>>{
      scope::Singleton{TransitiveSingletonSharedPtrProvider<Constructed>{}}};

  template <typename Requested, typename Canonical>
  auto resolve_via_transitive_binding() -> remove_rvalue_ref_t<Requested> {
    return transitive_binding<Canonical>.scope.template resolve<Requested>(
        *this);
  }
};

}  // namespace dink
