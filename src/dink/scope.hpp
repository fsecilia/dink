// \file
// Copyright (c) 2025 Frank Secilia
// SPDX-License-Identifier: MIT

#pragma once

#include <dink/lib.hpp>
#include <dink/meta.hpp>
#include <dink/smart_pointer_traits.hpp>
#include <concepts>
#include <memory>
#include <type_traits>

namespace dink::scope {

//! Resolves one instance per request.
class Transient {
 public:
  //! Resolves instance in requested form.
  template <typename Requested, typename Container, typename Provider>
  auto resolve(Container& container, Provider& provider)
      -> std::remove_reference_t<Requested> {
    using Provided = typename Provider::Provided;

    if constexpr (std::same_as<std::remove_cvref_t<Requested>, Provided>) {
      // Value type or rvalue reference.
      return provider.template create<Requested>(container);
    } else if constexpr (SharedPtr<Requested> || UniquePtr<Requested>) {
      // Smart pointers with ownership semantics.
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
  //! Resolves instance in requested form.
  template <typename Requested, typename Container, typename Provider>
  auto resolve(Container& container, Provider& provider) const -> Requested {
    // Order matters here; check for smart pointers first so references to them
    // can be taken without taking the reference branch.
    if constexpr (SharedPtr<Requested> || WeakPtr<Requested>) {
      // shared_ptr/weak_ptr
      return canonical_shared(container, provider);
    } else if constexpr (std::is_lvalue_reference_v<Requested>) {
      // lvalue references
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
  auto cached_instance(Container& container, Provider& provider) const
      -> Provider::Provided& {
    using Provided = Provider::Provided;
    static auto instance = provider.template create<Provided>(container);
    return instance;
  }

  //! Gets or creates canonical shared_ptr.
  //
  // The canonical shared_ptr points at instance with a no-op deleter. It
  // is itself cached. This means the control block is only allocated once
  // and weak_ptrs don't immediately expire.
  template <typename Container, typename Provider>
  auto canonical_shared(Container& container, Provider& provider) const
      -> std::shared_ptr<typename Provider::Provided>& {
    using Provided = Provider::Provided;
    static auto canonical_shared = std::shared_ptr<Provided>{
        &cached_instance(container, provider), [](Provided*) {}};
    return canonical_shared;
  }
};

//! Resolves one externally-owned instance.
template <typename Provided>
class Instance {
 public:
  //! Resolves instance in requested form.
  template <typename Requested, typename Container>
  constexpr auto resolve(Container& /*container*/) const -> Requested {
    if constexpr (std::is_lvalue_reference_v<Requested>) {
      // Lvalue reference (mutable or const)
      return *instance_;
    } else if constexpr (std::is_pointer_v<Requested>) {
      // Pointer (mutable or const)
      return instance_;
    } else if constexpr (SharedPtr<Requested> || WeakPtr<Requested>) {
      // shared_ptr and weak_ptr - create with no-op deleter.
      using Element = typename std::remove_cvref_t<Requested>::element_type;
      return std::shared_ptr<Element>(instance_, [](Element*) {});
    } else if constexpr (std::same_as<std::remove_cv_t<Requested>,
                                      std::remove_cv_t<Provided>>) {
      // Value - copy from instance
      return *instance_;
    } else {
      static_assert(meta::kDependentFalse<Requested>,
                    "Instance scope: Unsupported type conversion.");
    }
  }

  // Constructs an Instance scope referencing an external object.
  explicit constexpr Instance(Provided& instance) noexcept
      : instance_{&instance} {}

 private:
  Provided* instance_;
};

}  // namespace dink::scope
