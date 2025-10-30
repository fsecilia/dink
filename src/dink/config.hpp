// \file
// Copyright (c) 2025 Frank Secilia
// SPDX-License-Identifier: MIT

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

// ----------------------------------------------------------------------------
// ConfigFromTuple
// ----------------------------------------------------------------------------

//! Creates a config_t type from a tuple of bindings.
//
// Used primarily with deduction guides to convert tuple<Bindings...> to
// Config<Bindings...>.
template <typename Tuple>
struct FindConfigFromTuple;

//! Specialization to extract bindings from given tuple.
template <template <typename...> class Tuple, typename... Bindings>
struct FindConfigFromTuple<Tuple<Bindings...>> {
  using type = Config<Bindings...>;
};

//! Alias for creating config from tuple.
template <typename Tuple>
using ConfigFromTuple = typename FindConfigFromTuple<Tuple>::type;

}  // namespace detail

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
  explicit Config(std::tuple<Bindings...> bindings)
      : bindings_{std::move(bindings)} {}

  //! Construct from individual binding arguments.
  template <typename... Args>
  explicit Config(Args&&... args) : bindings_{std::forward<Args>(args)...} {}

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
  // between found and not found if constexpr branches.
  template <typename From>
  auto find_binding() -> auto {
    static constexpr auto index = detail::binding_index<From, BindingsTuple>;

    if constexpr (index != detail::npos) {
      return &std::get<index>(bindings_);
    } else {
      return nullptr;
    }
  }

 private:
  [[dink_no_unique_address]] BindingsTuple bindings_;
};

//! Constructs config from individual bindings.
template <IsBinding... Bindings>
Config(Bindings&&...) -> Config<std::remove_cvref_t<Bindings>...>;

}  // namespace dink
