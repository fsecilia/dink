// \file
// Copyright (c) 2025 Frank Secilia
// SPDX-License-Identifier: MIT
//
// \brief Defines how managed instances are stored.

#pragma once

#include <dink/lib.hpp>
#include <dink/meta.hpp>
#include <dink/smart_pointer_traits.hpp>
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

    if constexpr (IsSharedPtr<Requested> || IsUniquePtr<Requested> ||
                  std::same_as<std::remove_cvref_t<Requested>, Provided>) {
      // Value type or rvalue reference.
      return provider.template create<Requested>(container);
    } else {
      static_assert(meta::kDependentFalse<Requested>,
                    "Transient scope: unsupported type conversion.");
    }
  }
};

///! Resolves one instance per provider.
class Singleton {
 public:
  static constexpr auto provides_references = true;

  //! Resolves instance in requested form.
  template <typename Requested, typename Container, typename Provider>
  auto resolve(Container& container, Provider& provider) const -> Requested {
    if constexpr (IsSharedPtr<Requested> || IsWeakPtr<Requested> ||
                  std::is_lvalue_reference_v<Requested>) {
      // shared/weak pointers and lvalue references
      return cached_instance(container, provider);
    } else if constexpr (std::is_pointer_v<Requested>) {
      // Pointers
      return &cached_instance(container, provider);
    } else {
      static_assert(meta::kDependentFalse<Requested>,
                    "Singleton scope: unsupported type conversion.");
    }
  }

 private:
  //! Gets or creates cached instance.
  template <typename Container, typename Provider>
  static auto cached_instance(Container& container, Provider& provider) ->
      typename Provider::Provided& {
    using Provided = typename Provider::Provided;
    static auto instance = provider.template create<Provided>(container);
    return instance;
  }
};

//! Resolves one externally-owned instance.
template <typename Resolved>
class Instance {
 public:
  static constexpr auto provides_references = true;

  //! Resolves instance in requested form.
  template <typename Requested, typename Container, typename Provider>
  constexpr auto resolve(Container& container, Provider& provider) const
      -> Requested {
    // Get reference to the external instance from provider
    auto& instance =
        provider.template create<typename Provider::Provided&>(container);

    if constexpr (std::is_same_v<std::remove_cvref_t<Requested>, Resolved> ||
                  std::is_lvalue_reference_v<Requested>) {
      // Values and Lvalue reference (mutable or const)
      return instance;
    } else if constexpr (std::is_pointer_v<Requested>) {
      // Pointer (mutable or const)
      return &instance;
    } else {
      static_assert(meta::kDependentFalse<Requested>,
                    "Instance scope: unsupported type conversion.");
    }
  }
};

}  // namespace dink::scope
