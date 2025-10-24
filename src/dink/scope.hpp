// \file
// Copyright (c) 2025 Frank Secilia
// SPDX-License-Identifier: MIT

#pragma once

#include <dink/lib.hpp>
#include <dink/meta.hpp>
#include <dink/smart_pointer_traits.hpp>
#include <memory>
#include <type_traits>

namespace dink::scope {

//! Transient scope - creates new instances per request.
struct Transient {
  //! Resolves a transient instance.
  template <typename Requested, typename Container, typename Provider>
  constexpr auto resolve(Container& container, Provider& provider) const
      -> Requested {
    return provider.template create<Requested>(container);
  }
};

//! Singleton scope - creates and caches one instance per type.
struct Singleton {
  //! Resolves a singleton instance.
  template <typename Requested, typename Container, typename Provider>
  auto resolve(Container& container, Provider& provider) const -> Requested {
    using Provided = typename Provider::Provided;

    // Fast path: references and pointers don't need shared_ptr machinery
    if constexpr (std::is_lvalue_reference_v<Requested>) {
      return get_instance<Provided>(container, provider);
    } else if constexpr (std::is_pointer_v<Requested>) {
      return &get_instance<Provided>(container, provider);
    }
    // Slow path: shared_ptr/weak_ptr need the canonical control block
    else if constexpr (SharedPtr<Requested> || WeakPtr<Requested>) {
      auto& shared = get_canonical_shared<Provided>(container, provider);
      return shared;
    }
    // Value request - copy from singleton
    else if constexpr (std::is_same_v<std::remove_cv_t<Requested>, Provided>) {
      auto& instance = get_instance<Provided>(container, provider);
      return instance;
    } else {
      static_assert(meta::kDependentFalse<Requested>,
                    "Singleton scope: unsupported type conversion.");
    }
  }

 private:
  //! Gets or creates the cached instance.
  template <typename Provided, typename Container, typename Provider>
  static auto get_instance(Container& container, Provider& provider)
      -> Provided& {
    static auto instance = provider.template create<Provided>(container);
    return instance;
  }

  //! Gets or creates canonical shared_ptr. Points at instance with no-op
  // deleter.
  template <typename Provided, typename Container, typename Provider>
  static auto get_canonical_shared(Container& container, Provider& provider)
      -> std::shared_ptr<Provided>& {
    static auto shared = std::shared_ptr<Provided>(
        &get_instance<Provided>(container, provider), [](Provided*) {});
    return shared;
  }
};

//! Instance scope - resolves from an externally-owned instance.
template <typename Provided>
class Instance {
 public:
  //! Resolves directly from external instance.
  template <typename Requested, typename Container>
  constexpr auto resolve(Container& /*container*/) const -> Requested {
    // Lvalue reference (mutable or const)
    if constexpr (std::is_lvalue_reference_v<Requested>) {
      return *instance_;
    }
    // Pointer (mutable or const)
    else if constexpr (std::is_pointer_v<Requested>) {
      return instance_;
    }
    // shared_ptr and weak_ptr - create with no-op deleter.
    else if constexpr (SharedPtr<Requested> || WeakPtr<Requested>) {
      using Element = typename std::remove_cvref_t<Requested>::element_type;
      return std::shared_ptr<Element>(instance_, [](Element*) {});
    }
    // Value - copy from instance
    else if constexpr (std::is_same_v<std::remove_cv_t<Requested>,
                                      std::remove_cv_t<Provided>>) {
      return *instance_;
    } else {
      static_assert(meta::kDependentFalse<Requested>,
                    "Unsupported type conversion from instance reference.");
    }
  }

  // Constructs an Instance scope referencing an external object.
  explicit constexpr Instance(Provided& instance) noexcept
      : instance_{&instance} {}

 private:
  Provided* instance_;
};

//! Deduction guide for Instance
template <typename T>
Instance(T&) -> Instance<T>;

}  // namespace dink::scope
