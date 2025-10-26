// \file
// Copyright (c) 2025 Frank Secilia
// SPDX-License-Identifier: MIT

#pragma once

#include <dink/lib.hpp>

namespace dink {

template <typename Type>
struct remove_rvalue_ref;

template <typename Type>
struct remove_rvalue_ref {
  using type = Type;
};

template <typename Type>
struct remove_rvalue_ref<Type&&> {
  using type = Type;
};

template <typename Type>
using remove_rvalue_ref_t = typename remove_rvalue_ref<Type>::type;

}  // namespace dink
