// \file
// Copyright (c) 2025 Frank Secilia
// SPDX-License-Identifier: MIT
//
// \brief Defines how managed instances are stored.

#pragma once

#include <dink/lib.hpp>
#include <dink/meta.hpp>
#include <concepts>
#include <type_traits>

namespace dink::scope {

//! Resolves one instance per request.
class Transient {
 public:
  static constexpr auto provides_references = false;

  //! Resolves instance in requested form.
  template <typename Requested, typename Container, typename Provider>
  auto resolve(Container& container, Provider& provider) const
      -> meta::RemoveRvalueRef<Requested> {
    using Provided = typename Provider::Provided;

    if constexpr (meta::IsSharedPtr<Requested> ||
                  meta::IsUniquePtr<Requested> ||
                  std::same_as<std::remove_cvref_t<Requested>, Provided>) {
      // Value type or rvalue reference.
      return provider.template create<Requested>(container);
    } else {
      static_assert(meta::kDependentFalse<Requested>,
                    "Transient scope: unsupported type conversion.");
    }
  }
};

//! Resolves one instance per provider.
class Singleton {
 public:
  static constexpr auto provides_references = true;

  //! Resolves instance in requested form.
  template <typename Requested, typename Container, typename Provider>
  auto resolve(Container& container, Provider& provider) const
      -> meta::RemoveRvalueRef<Requested> {
    using Provided = typename Provider::Provided;

    if constexpr (std::is_same_v<std::remove_cvref_t<Requested>, Provided> ||
                  std::is_lvalue_reference_v<Requested> ||
                  meta::IsSharedPtr<Requested> || meta::IsWeakPtr<Requested>) {
      // Values, lvalue references, and shared/weak pointers.
      static_assert(
          !meta::IsWeakPtr<Requested> || meta::IsSharedPtr<Provided>,
          "Request for weak_ptr must be satisfied by cached shared_ptr.");
      return cached_instance(container, provider);
    } else if constexpr (std::is_pointer_v<Requested>) {
      // Pointers.
      return &cached_instance(container, provider);
    } else if constexpr (meta::IsUniquePtr<Requested>) {
      // unique_ptr.
      return std::make_unique<Provided>(cached_instance(container, provider));
    } else {
      static_assert(meta::kDependentFalse<Requested>,
                    "Singleton scope: unsupported type conversion.");
    }
  }

 private:
  //! Gets or creates cached instance.
  template <typename Container, typename Provider>
  static auto cached_instance(Container& container, Provider& provider)
      -> Provider::Provided& {
    return container.get_or_create(provider);
  }
};

//! Resolves one externally-owned instance.
class Instance {
 public:
  static constexpr auto provides_references = true;

  //! Resolves instance in requested form.
  template <typename Requested, typename Container, typename Provider>
  constexpr auto resolve(Container& container, Provider& provider) const
      -> meta::RemoveRvalueRef<Requested> {
    using Provided = typename Provider::Provided;

    // Get reference to the external instance from provider
    auto& instance = provider.template create<Provided&>(container);

    if constexpr (std::is_same_v<std::remove_cvref_t<Requested>, Provided> ||
                  std::is_lvalue_reference_v<Requested> ||
                  meta::IsSharedPtr<Requested> || meta::IsWeakPtr<Requested>) {
      // Values, lvalue references, and shared/weak pointers.
      static_assert(
          !meta::IsWeakPtr<Requested> || meta::IsSharedPtr<Provided>,
          "Request for weak_ptr must be satisfied by cached shared_ptr.");
      return instance;
    } else if constexpr (std::is_pointer_v<Requested>) {
      // Pointers.
      return &instance;
    } else if constexpr (meta::IsUniquePtr<Requested>) {
      // unique_ptr.
      return std::make_unique<Provided>(instance);
    } else {
      static_assert(meta::kDependentFalse<Requested>,
                    "Instance scope: unsupported type conversion.");
    }
  }
};

}  // namespace dink::scope
