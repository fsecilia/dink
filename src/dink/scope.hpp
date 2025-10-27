// \file
// Copyright (c) 2025 Frank Secilia
// SPDX-License-Identifier: MIT

#pragma once

#include <dink/lib.hpp>
#include <dink/meta.hpp>
#include <dink/remove_rvalue_ref.hpp>
#include <dink/smart_pointer_traits.hpp>
#include <concepts>
#include <memory>
#include <type_traits>

namespace dink::scope {

namespace detail {

//! Gets or creates cached instance.
template <typename Container, typename Provider>
auto cached_instance(Container& container, Provider& provider) ->
    typename Provider::Provided& {
  using Provided = typename Provider::Provided;
  static auto instance = provider.template create<Provided>(container);
  return instance;
}

}  // namespace detail

//! Resolves one instance per request.
class Transient {
 public:
  static constexpr auto provides_references = false;

  //! Resolves instance in requested form.
  template <typename Requested, typename Container, typename Provider>
  auto resolve(Container& container, Provider& provider)
      -> remove_rvalue_ref_t<Requested> {
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
  static constexpr auto provides_references = true;

  //! Resolves instance in requested form.
  template <typename Requested, typename Container, typename Provider>
  auto resolve(Container& container, Provider& provider) -> Requested {
    if constexpr (std::is_lvalue_reference_v<Requested>) {
      // lvalue references
      return detail::cached_instance(container, provider);
    } else if constexpr (std::is_pointer_v<Requested>) {
      // Pointers
      return &detail::cached_instance(container, provider);
    } else if constexpr (SharedPtr<Requested> || WeakPtr<Requested>) {
      // shared_ptr/weak_ptr - Cache shared_ptr<Provided> via static
      // This handles the canonical shared_ptr case
      using Provided = typename Provider::Provided;
      static_assert(
          std::same_as<
              Provided,
              std::shared_ptr<std::remove_cvref_t<
                  typename std::pointer_traits<Requested>::element_type>>>,
          "Singleton with smart pointer: Provider must provide shared_ptr");
      return detail::cached_instance(container, provider);
    } else {
      static_assert(meta::kDependentFalse<Requested>,
                    "Singleton scope: unsupported type conversion.");
    }
  }
};

//! Resolves one externally-owned instance.
template <typename Resolved>
class Instance {
 public:
  static constexpr auto provides_references = true;

  //! Resolves instance in requested form.
  template <typename Requested, typename Container, typename Provider>
  constexpr auto resolve(Container& /*container*/, Provider& /*provider*/)
      -> Requested {
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
    } else {
      static_assert(meta::kDependentFalse<Requested>,
                    "Instance scope: use references/pointers only. "
                    "Relegation will handle value requests.");
    }
  }

  // Constructs an Instance scope referencing an external object.
  explicit constexpr Instance(Resolved& instance) noexcept
      : instance_{&instance} {}

 private:
  Resolved* instance_;
};

}  // namespace dink::scope
