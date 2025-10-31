// \file
// Copyright (c) 2025 Frank Secilia
// SPDX-License-Identifier: MIT
//
// \brief Simple list of types.

#pragma once

#include <dink/lib.hpp>
#include <type_traits>

namespace dink {

//! Lightweight, compile-time tuple.
template <typename... Elements>
struct TypeList {
  //! Metafunction returning the original type list with Element appended.
  template <typename Element>
  using Append = TypeList<Elements..., Element>;

  //! true if Elements contains Element.
  template <typename Element>
  static constexpr auto kContains = (std::is_same_v<Element, Elements> || ...);
};

}  // namespace dink
