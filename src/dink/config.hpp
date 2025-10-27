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

//! forward declaration
template <typename... Bindings>
class Config;

// ----------------------------------------------------------------------------
// concepts
// ----------------------------------------------------------------------------

//! concept for valid configuration types
//
// A config must support finding bindings by resolved type.
template <typename config_t>
concept IsConfig = requires(config_t& config) {
  config.template find_binding<meta::ConceptProbe>();
};

// ----------------------------------------------------------------------------
// implementation details
// ----------------------------------------------------------------------------

namespace detail {

inline static constexpr auto npos = std::size_t(-1);

//! finds the index of a binding for Resolved in the bindings tuple
//
// \tparam Resolved type to search for
// \tparam index current search index
// \tparam BindingsTuple tuple of binding types
// \return index of binding, or npos if not found
template <typename Resolved, std::size_t index, typename BindingsTuple>
struct BindingIndex;

//! base case: value is not found
template <typename Resolved, std::size_t index, typename BindingsTuple>
struct BindingIndex {
  static constexpr auto value = npos;
};

//! recursive case: check if current binding matches, otherwise continue
template <typename Resolved, std::size_t index, typename BindingsTuple>
  requires(index < std::tuple_size_v<BindingsTuple>)
struct BindingIndex<Resolved, index, BindingsTuple> {
  using CurrentBinding = std::tuple_element_t<index, BindingsTuple>;

  static constexpr auto value =
      std::same_as<Resolved, typename CurrentBinding::FromType>
          ? index
          : BindingIndex<Resolved, index + 1, BindingsTuple>::value;
};

//! alias for finding binding index
template <typename Resolved, typename BindingsTuple>
inline static constexpr auto binding_index =
    BindingIndex<Resolved, 0, BindingsTuple>::value;

// ----------------------------------------------------------------------------

//! creates a config_t type from a tuple of bindings
//
// Used primarily with deduction guides to convert tuple<bindings...> to
// config_t<bindings...>
template <typename Tuple>
struct FindConfigFromTuple;

//! specialization to extract bindings from given tuple
template <template <typename...> class Tuple, typename... Bindings>
struct FindConfigFromTuple<Tuple<Bindings...>> {
  using type = Config<Bindings...>;
};

//! alias for creating config from tuple
template <typename Tuple>
using ConfigFromTuple = typename FindConfigFromTuple<Tuple>::type;

}  // namespace detail

// ----------------------------------------------------------------------------
// configuration class
// ----------------------------------------------------------------------------

//! type-safe configuration storage for dependency injection bindings
//
// Stores bindings in a compile-time indexed tuple, enabling O(1) lookup by
// type.
//
// Each binding maps from a requested type (from_type) to:
// - The type to construct (to_type)
// - The scope (when to cache)
// - The provider (how to construct)
//
// \tparam Bindings pack of binding_t types
template <typename... Bindings>
class Config {
 public:
  using BindingsTuple = std::tuple<Bindings...>;

  // -----------------------------------------------------------------------------------------------------------------
  // constructors
  // -----------------------------------------------------------------------------------------------------------------

  //! construct from a tuple of bindings
  explicit Config(std::tuple<Bindings...> bindings)
      : bindings_{std::move(bindings)} {}

  //! construct from individual binding arguments
  template <typename... Args>
  explicit Config(Args&&... args) : bindings_{std::forward<Args>(args)...} {}

  // -----------------------------------------------------------------------------------------------------------------
  // binding lookup
  // -----------------------------------------------------------------------------------------------------------------

  //! finds binding for a resolved type
  //
  // \tparam Resolved canonical type to look up
  // \return pointer to binding if found, otherwise nullptr
  //
  // The type of the result is a pointer to a binding if found, but nullptr_t
  // otherwise. This distinction can be tested at compile-time to switch
  // between found and not found branches.
  template <typename Resolved>
  auto find_binding() -> auto {
    static constexpr auto index =
        detail::binding_index<Resolved, BindingsTuple>;

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
// deduction guides
// ----------------------------------------------------------------------------

//! deduction guide to construct config from individual bindings
template <typename... Bindings>
Config(Bindings&&...) -> Config<std::remove_cvref_t<Bindings>...>;

}  // namespace dink
