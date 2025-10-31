// \file
// Copyright (c) 2025 Frank Secilia
// SPDX-License-Identifier: MIT

#include "meta.hpp"
#include <dink/test.hpp>

namespace dink::meta {

// ----------------------------------------------------------------------------
// RemoveRvalueRef
// ----------------------------------------------------------------------------

// Basic types pass through.
static_assert(std::is_same_v<RemoveRvalueRef<int>, int>);
static_assert(std::is_same_v<RemoveRvalueRef<void>, void>);

// Lvalue references should not be removed.
static_assert(std::is_same_v<RemoveRvalueRef<int&>, int&>);
static_assert(std::is_same_v<RemoveRvalueRef<const int&>, const int&>);

// Rvalue references should be removed.
static_assert(std::is_same_v<RemoveRvalueRef<int&&>, int>);
static_assert(std::is_same_v<RemoveRvalueRef<const int&&>, const int>);
static_assert(std::is_same_v<RemoveRvalueRef<volatile int&&>, volatile int>);

// Pointers pass through.
static_assert(std::is_same_v<RemoveRvalueRef<int*>, int*>);
static_assert(std::is_same_v<RemoveRvalueRef<int*&>, int*&>);
static_assert(std::is_same_v<RemoveRvalueRef<int*&&>, int*>);
static_assert(std::is_same_v<RemoveRvalueRef<const int*&&>, const int*>);

// ----------------------------------------------------------------------------
// UniqueType
// ----------------------------------------------------------------------------

// Type other than UniqueType<> for testing.
struct ArbitraryType {};

// Verify uniqueness.
// ----------------------------------------------------------------------------

static_assert(
    !std::is_same_v<UniqueType<>, UniqueType<>>,
    "Two default UniqueType instantiations should not be the same type.");

// Verify the trait and concept work.
// ----------------------------------------------------------------------------

static_assert(IsUniqueType<UniqueType<>>,
              "Trait failed to identify a UniqueType.");

static_assert(!IsUniqueType<ArbitraryType>,
              "Trait incorrectly identified ArbitraryType as a UniqueType.");

static_assert(traits::is_unique_type<UniqueType<>>,
              "Trait value failed to identify a UniqueType.");

static_assert(
    !traits::is_unique_type<ArbitraryType>,
    "Trait value incorrectly identified ArbitraryType as a UniqueType.");

}  // namespace dink::meta
