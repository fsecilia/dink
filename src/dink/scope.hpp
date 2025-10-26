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

namespace detail {

//! Gets or creates cached instance.
template <typename Container, typename Provider>
auto cached_instance(Container& container, Provider& provider)
    -> Provider::Provided& {
  using Provided = Provider::Provided;
  static auto instance = provider.template create<Provided>(container);
  return instance;
}

}  // namespace detail

//! Resolves one instance per request.
template <typename Provider>
class Transient {
 public:
  using Provided = typename Provider::Provided;

  //! Resolves instance in requested form.
  template <typename Requested, typename Container>
  auto resolve(Container& container) -> std::remove_reference_t<Requested> {
    using Provided = typename Provider::Provided;

    if constexpr (std::same_as<std::remove_cvref_t<Requested>, Provided>) {
      // Value type or rvalue reference.
      return provider_.template create<Requested>(container);
    } else if constexpr (SharedPtr<Requested> || UniquePtr<Requested>) {
      // Smart pointers with ownership semantics.
      return provider_.template create<Requested>(container);
    } else {
      static_assert(meta::kDependentFalse<Requested>,
                    "Transient scope: unsupported type conversion.");
    }
  }

  explicit constexpr Transient(Provider provider) noexcept
      : provider_{std::move(provider)} {}

 private:
  Provider provider_;
};

///! Resolves one instance per provider.
template <typename Provider>
class Singleton {
 public:
  using Provided = typename Provider::Provided;

  //! Resolves instance in requested form.
  template <typename Requested, typename Container>
  auto resolve(Container& container) const -> Requested {
    // Order matters here; check for smart pointers first so references to them
    // can be taken without taking the reference branch.
    if constexpr (SharedPtr<Requested> || WeakPtr<Requested>) {
      // shared_ptr/weak_ptr
      return detail::cached_instance(container, provider_);
    } else if constexpr (std::is_lvalue_reference_v<Requested>) {
      // lvalue references
      return detail::cached_instance(container, provider_);
    } else if constexpr (std::is_pointer_v<Requested>) {
      // Pointers
      return &detail::cached_instance(container, provider_);
    } else {
      static_assert(meta::kDependentFalse<Requested>,
                    "Singleton scope: unsupported type conversion.");
    }
  }

  explicit constexpr Singleton(Provider provider) noexcept
      : provider_{std::move(provider)} {}

 private:
  Provider provider_;
};

template <typename Provider>
class Deduced {
 public:
  using Provided = typename Provider::Provided;

  //! Resolves instance in requested form.
  template <typename Requested, typename Container>
  auto resolve(Container& container) -> std::remove_reference_t<Requested> {
    if constexpr (SharedPtr<Requested> || WeakPtr<Requested>) {
      // shared_ptr/weak_ptr
      return detail::cached_instance(container, provider_);
    } else if constexpr (std::is_lvalue_reference_v<Requested>) {
      // lvalue references
      return detail::cached_instance(container, provider_);
    } else if constexpr (std::is_pointer_v<Requested>) {
      // Pointers
      return &detail::cached_instance(container, provider_);
    } else {
      // Value type or rvalue reference.
      return provider_.template create<Requested>(container);
    }
  }

  explicit constexpr Deduced(Provider provider) noexcept
      : provider_{std::move(provider)} {}

 private:
  Provider provider_;
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
