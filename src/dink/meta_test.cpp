// \file
// Copyright (c) 2025 Frank Secilia
// SPDX-License-Identifier: MIT

#include "meta.hpp"
#include <dink/test.hpp>

namespace dink::meta {

struct Element {};
struct Deleter {};

using SharedPtr = std::shared_ptr<Element>;
using WeakPtr = std::weak_ptr<Element>;
using UniquePtr = std::unique_ptr<Element, Deleter>;

// ----------------------------------------------------------------------------
// shared_ptr
// ----------------------------------------------------------------------------

// Trait Variable Template
// ----------------------------------------------------------------------------

static_assert(!traits::is_shared_ptr<void>);
static_assert(!traits::is_shared_ptr<Element>);
static_assert(!traits::is_shared_ptr<Element*>);
static_assert(traits::is_shared_ptr<SharedPtr>);
static_assert(!traits::is_shared_ptr<WeakPtr>);
static_assert(!traits::is_shared_ptr<UniquePtr>);

static_assert(!traits::is_shared_ptr<const SharedPtr>);
static_assert(!traits::is_shared_ptr<SharedPtr&>);
static_assert(!traits::is_shared_ptr<const SharedPtr&>);
static_assert(!traits::is_shared_ptr<SharedPtr&&>);

// Concept
// ----------------------------------------------------------------------------
static_assert(!IsSharedPtr<void>);
static_assert(!IsSharedPtr<Element>);
static_assert(!IsSharedPtr<Element*>);
static_assert(IsSharedPtr<SharedPtr>);
static_assert(!IsSharedPtr<WeakPtr>);
static_assert(!IsSharedPtr<UniquePtr>);

static_assert(IsSharedPtr<const SharedPtr>);
static_assert(IsSharedPtr<SharedPtr&>);
static_assert(IsSharedPtr<const SharedPtr&>);
static_assert(IsSharedPtr<SharedPtr&&>);

// ----------------------------------------------------------------------------
// unique_ptr
// ----------------------------------------------------------------------------

// Trait Variable Template
// ----------------------------------------------------------------------------
static_assert(!traits::is_unique_ptr<void>);
static_assert(!traits::is_unique_ptr<Element>);
static_assert(!traits::is_unique_ptr<Element*>);
static_assert(!traits::is_unique_ptr<SharedPtr>);
static_assert(traits::is_unique_ptr<UniquePtr>);
static_assert(!traits::is_unique_ptr<WeakPtr>);

static_assert(!traits::is_unique_ptr<const UniquePtr>);
static_assert(!traits::is_unique_ptr<UniquePtr&>);
static_assert(!traits::is_unique_ptr<const UniquePtr&>);
static_assert(!traits::is_unique_ptr<UniquePtr&&>);

// Concept
// ----------------------------------------------------------------------------
static_assert(!IsUniquePtr<void>);
static_assert(!IsUniquePtr<Element>);
static_assert(!IsUniquePtr<Element*>);
static_assert(!IsUniquePtr<SharedPtr>);
static_assert(IsUniquePtr<UniquePtr>);
static_assert(!IsUniquePtr<WeakPtr>);

static_assert(IsUniquePtr<const UniquePtr>);
static_assert(IsUniquePtr<UniquePtr&>);
static_assert(IsUniquePtr<const UniquePtr&>);
static_assert(IsUniquePtr<UniquePtr&&>);

// ----------------------------------------------------------------------------
// weak_ptr
// ----------------------------------------------------------------------------

// Trait Variable Template
// ----------------------------------------------------------------------------
static_assert(!traits::is_weak_ptr<void>);
static_assert(!traits::is_weak_ptr<Element>);
static_assert(!traits::is_weak_ptr<Element*>);
static_assert(!traits::is_weak_ptr<SharedPtr>);
static_assert(traits::is_weak_ptr<WeakPtr>);
static_assert(!traits::is_weak_ptr<UniquePtr>);

static_assert(!traits::is_weak_ptr<const WeakPtr>);
static_assert(!traits::is_weak_ptr<WeakPtr&>);
static_assert(!traits::is_weak_ptr<const WeakPtr&>);
static_assert(!traits::is_weak_ptr<WeakPtr&&>);

// Concept
// ----------------------------------------------------------------------------
static_assert(!IsWeakPtr<void>);
static_assert(!IsWeakPtr<Element>);
static_assert(!IsWeakPtr<Element*>);
static_assert(!IsWeakPtr<SharedPtr>);
static_assert(IsWeakPtr<WeakPtr>);
static_assert(!IsWeakPtr<UniquePtr>);

static_assert(IsWeakPtr<const WeakPtr>);
static_assert(IsWeakPtr<WeakPtr&>);
static_assert(IsWeakPtr<const WeakPtr&>);
static_assert(IsWeakPtr<WeakPtr&&>);

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
