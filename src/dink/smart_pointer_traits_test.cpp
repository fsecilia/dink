// \file
// Copyright (c) 2025 Frank Secilia
// SPDX-License-Identifier: MIT

#include "smart_pointer_traits.hpp"
#include <dink/test.hpp>

namespace dink {
namespace {

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
static_assert(!is_shared_ptr<void>);
static_assert(!is_shared_ptr<Element>);
static_assert(!is_shared_ptr<Element*>);
static_assert(is_shared_ptr<SharedPtr>);
static_assert(!is_shared_ptr<WeakPtr>);
static_assert(!is_shared_ptr<UniquePtr>);

static_assert(!is_shared_ptr<const SharedPtr>);
static_assert(!is_shared_ptr<SharedPtr&>);
static_assert(!is_shared_ptr<const SharedPtr&>);
static_assert(!is_shared_ptr<SharedPtr&&>);

// Concept
// ----------------------------------------------------------------------------
static_assert(!dink::SharedPtr<void>);
static_assert(!dink::SharedPtr<Element>);
static_assert(!dink::SharedPtr<Element*>);
static_assert(dink::SharedPtr<SharedPtr>);
static_assert(!dink::SharedPtr<WeakPtr>);
static_assert(!dink::SharedPtr<UniquePtr>);

static_assert(dink::SharedPtr<const SharedPtr>);
static_assert(dink::SharedPtr<SharedPtr&>);
static_assert(dink::SharedPtr<const SharedPtr&>);
static_assert(dink::SharedPtr<SharedPtr&&>);

// ----------------------------------------------------------------------------
// unique_ptr
// ----------------------------------------------------------------------------

// Trait Variable Template
// ----------------------------------------------------------------------------
static_assert(!is_unique_ptr<void>);
static_assert(!is_unique_ptr<Element>);
static_assert(!is_unique_ptr<Element*>);
static_assert(!is_unique_ptr<SharedPtr>);
static_assert(is_unique_ptr<UniquePtr>);
static_assert(!is_unique_ptr<WeakPtr>);

static_assert(!is_unique_ptr<const UniquePtr>);
static_assert(!is_unique_ptr<UniquePtr&>);
static_assert(!is_unique_ptr<const UniquePtr&>);
static_assert(!is_unique_ptr<UniquePtr&&>);

// Concept
// ----------------------------------------------------------------------------
static_assert(!dink::UniquePtr<void>);
static_assert(!dink::UniquePtr<Element>);
static_assert(!dink::UniquePtr<Element*>);
static_assert(!dink::UniquePtr<SharedPtr>);
static_assert(dink::UniquePtr<UniquePtr>);
static_assert(!dink::UniquePtr<WeakPtr>);

static_assert(dink::UniquePtr<const UniquePtr>);
static_assert(dink::UniquePtr<UniquePtr&>);
static_assert(dink::UniquePtr<const UniquePtr&>);
static_assert(dink::UniquePtr<UniquePtr&&>);

// ----------------------------------------------------------------------------
// weak_ptr
// ----------------------------------------------------------------------------

// Trait Variable Template
// ----------------------------------------------------------------------------
static_assert(!is_weak_ptr<void>);
static_assert(!is_weak_ptr<Element>);
static_assert(!is_weak_ptr<Element*>);
static_assert(!is_weak_ptr<SharedPtr>);
static_assert(is_weak_ptr<WeakPtr>);
static_assert(!is_weak_ptr<UniquePtr>);

static_assert(!is_weak_ptr<const WeakPtr>);
static_assert(!is_weak_ptr<WeakPtr&>);
static_assert(!is_weak_ptr<const WeakPtr&>);
static_assert(!is_weak_ptr<WeakPtr&&>);

// Concept
// ----------------------------------------------------------------------------
static_assert(!dink::WeakPtr<void>);
static_assert(!dink::WeakPtr<Element>);
static_assert(!dink::WeakPtr<Element*>);
static_assert(!dink::WeakPtr<SharedPtr>);
static_assert(dink::WeakPtr<WeakPtr>);
static_assert(!dink::WeakPtr<UniquePtr>);

static_assert(dink::WeakPtr<const WeakPtr>);
static_assert(dink::WeakPtr<WeakPtr&>);
static_assert(dink::WeakPtr<const WeakPtr&>);
static_assert(dink::WeakPtr<WeakPtr&&>);

}  // namespace
}  // namespace dink
