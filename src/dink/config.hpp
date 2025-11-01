// \file
// Copyright (c) 2025 Frank Secilia
// SPDX-License-Identifier: MIT
//
// \brief Defines a compile-time collection of binding triples.
//
// \sa binding.hpp for the binding triples themselves.

#pragma once

#include <dink/lib.hpp>
#include <dink/binding.hpp>
#include <dink/meta.hpp>
#include <dink/scope.hpp>
#include <tuple>
#include <utility>

namespace dink {

template <typename... Bindings>
class Config;

namespace detail {

// ----------------------------------------------------------------------------
// BindingIndex
// ----------------------------------------------------------------------------

inline static constexpr auto npos = std::size_t(-1);

//! Finds the index of a binding for Resolved in the bindings tuple.
//
// \tparam Resolved type to search for
// \tparam index current search index
// \tparam BindingsTuple tuple of binding types
// \return index of binding, or npos if not found
template <typename From, std::size_t index, typename BindingsTuple>
struct BindingIndex;

//! Base case: value was not found.
template <typename From, std::size_t index, typename BindingsTuple>
struct BindingIndex {
  static constexpr auto value = npos;
};

//! Recursive case: check if current binding matches, otherwise continue.
template <typename From, std::size_t index, typename BindingsTuple>
  requires(index < std::tuple_size_v<BindingsTuple>)
struct BindingIndex<From, index, BindingsTuple> {
  using CurrentBinding = std::tuple_element_t<index, BindingsTuple>;

  static constexpr auto value =
      std::same_as<From, typename CurrentBinding::FromType>
          ? index
          : BindingIndex<From, index + 1, BindingsTuple>::value;
};

//! Variable template containing binding index.
template <typename From, typename BindingsTuple>
inline static constexpr auto binding_index =
    BindingIndex<From, 0, BindingsTuple>::value;

}  // namespace detail

// ----------------------------------------------------------------------------
// Config
// ----------------------------------------------------------------------------

//! Searchable, type-safe, heterogeneous storage for DI bindings.
//
// This type stores bindings in a tuple, enabling O(1) lookup by resolved type
// at compile time.
//
// Each binding is a unique type mapping from a requested type (FromType) to:
// - The type to construct (ToType)
// - The scope (when to cache)
// - The provider (how to construct)
//
// \tparam Bindings pack of arbitrary Binding types.
template <typename... Bindings>
class Config {
 public:
  using BindingsTuple = std::tuple<Bindings...>;

  //! Construct from a tuple of bindings.
  explicit constexpr Config(BindingsTuple bindings) noexcept
      : bindings_{std::move(bindings)} {}

  //! Construct from individual binding arguments.
  //
  // This constructor accepts types convertible to a binding and converts each.
  template <IsConvertibleToBinding... Args>
  explicit constexpr Config(Args&&... args) noexcept
      : Config{std::tuple{Binding{std::forward<Args>(args)}...}} {}

  //! Finds first binding with matching From type.
  //
  // \tparam From FromType to search for in bindings
  // \return pointer to binding if found, otherwise nullptr
  //
  // This method searches for the first binding that has a FromType matching
  // From. The search is done at compile time.
  //
  // The type of the result is a pointer to a binding if found, nullptr_t
  // otherwise. This distinction can be tested at compile-time to switch
  // between `if constexpr` branches for found and not found.
  template <typename From>
  constexpr auto find_binding() noexcept -> auto {
    constexpr auto index = detail::binding_index<From, BindingsTuple>;

    if constexpr (index != detail::npos) {
      return &std::get<index>(bindings_);
    } else {
      return nullptr;
    }
  }

 private:
  [[dink_no_unique_address]] BindingsTuple bindings_;
};

// ----------------------------------------------------------------------------
// Deduction Guides
// ----------------------------------------------------------------------------

//! Converts args to actual bindings.
template <IsConvertibleToBinding... Args>
Config(Args&&...)
    -> Config<std::remove_cvref_t<decltype(Binding{std::declval<Args>()})>...>;

// ----------------------------------------------------------------------------
// Concepts
// ----------------------------------------------------------------------------

//! Identifies valid configuration types.
//
// A config must support finding bindings by resolved type.
template <typename config_t>
concept IsConfig = requires(config_t& config) {
  config.template find_binding<meta::ConceptProbe>();
};

}  // namespace dink
