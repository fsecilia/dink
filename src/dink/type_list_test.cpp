// \file
// Copyright (c) 2025 Frank Secilia
// SPDX-License-Identifier: MIT

#include "type_list.hpp"
#include <dink/test.hpp>

namespace dink {
namespace {

// Arbitrary, unique types.
struct T0;
struct T1;
struct T2;
struct T3;

// ----------------------------------------------------------------------------
// TypeList::Append
// ----------------------------------------------------------------------------

// Append to empty list.
// ----------------------------------------------------------------------------
static_assert(std::is_same_v<TypeList<>::Append<T0>, TypeList<T0>>);

// Append to list with one element.
// ----------------------------------------------------------------------------
static_assert(std::is_same_v<TypeList<T0>::Append<T1>, TypeList<T0, T1>>);

// Append to list with multiple elements.
// ----------------------------------------------------------------------------
static_assert(
    std::is_same_v<TypeList<T0, T1, T2>::Append<T3>, TypeList<T0, T1, T2, T3>>);

// ----------------------------------------------------------------------------
// TypeList::kContains
// ----------------------------------------------------------------------------

// Empty list.
// ----------------------------------------------------------------------------
static_assert(!TypeList<>::kContains<T0>);  // Always contains nothing.

// Single element.
// ----------------------------------------------------------------------------
static_assert(TypeList<T0>::kContains<T0>);   // Contained.
static_assert(!TypeList<T0>::kContains<T1>);  // Not contained.

// Multiple elements.
// ----------------------------------------------------------------------------
static_assert(TypeList<T0, T1, T2>::kContains<T0>);   // Begin contained.
static_assert(TypeList<T0, T1, T2>::kContains<T2>);   // End contained.
static_assert(!TypeList<T0, T1, T2>::kContains<T3>);  // Not contained.

}  // namespace
}  // namespace dink
