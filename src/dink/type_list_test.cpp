// \file
// Copyright (c) 2025 Frank Secilia
// SPDX-License-Identifier: MIT

#include "type_list.hpp"
#include <dink/test.hpp>

namespace dink {
namespace {

// arbitrary, unique types
struct T0;
struct T1;
struct T2;
struct T3;

// ----------------------------------------------------------------------------
// TypeList::Append
// ----------------------------------------------------------------------------

// append to empty list
// ----------------------------------------------------------------------------
static_assert(std::is_same_v<TypeList<>::Append<T0>, TypeList<T0>>);

// append to list with one element
// ----------------------------------------------------------------------------
static_assert(std::is_same_v<TypeList<T0>::Append<T1>, TypeList<T0, T1>>);

// append to list with multiple elements
// ----------------------------------------------------------------------------
static_assert(
    std::is_same_v<TypeList<T0, T1, T2>::Append<T3>, TypeList<T0, T1, T2, T3>>);

// ----------------------------------------------------------------------------
// TypeList::kContains
// ----------------------------------------------------------------------------

// empty list
// ----------------------------------------------------------------------------
static_assert(!TypeList<>::kContains<T0>);  // always contains nothing

// single element
// ----------------------------------------------------------------------------
static_assert(TypeList<T0>::kContains<T0>);   // contained
static_assert(!TypeList<T0>::kContains<T1>);  // not contained

// multiple elements
// ----------------------------------------------------------------------------
static_assert(TypeList<T0, T1, T2>::kContains<T0>);   // begin contained
static_assert(TypeList<T0, T1, T2>::kContains<T2>);   // end contained
static_assert(!TypeList<T0, T1, T2>::kContains<T3>);  // not contained

}  // namespace
}  // namespace dink
