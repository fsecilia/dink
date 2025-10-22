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

}  // namespace
}  // namespace dink
