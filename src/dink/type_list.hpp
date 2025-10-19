// \file
// Copyright (c) 2025 Frank Secilia
// SPDX-License-Identifier: MIT

#pragma once

#include <dink/lib.hpp>

namespace dink {

//! Lightweight, compile-time tuple.
template <typename... Elements>
struct TypeList {
  //! Metafunction returning the original type list with Element appended.
  template <typename Element>
  using Append = TypeList<Elements..., Element>;
};

}  // namespace dink
