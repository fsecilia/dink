// \file
// Copyright (c) 2025 Frank Secilia
// SPDX-License-Identifier: MIT

#include "canonical.hpp"
#include <dink/test.hpp>

namespace dink {
namespace {

// Arbitrary unique type.
struct Type {};

// Deleter for unique_ptrs.
struct Deleter {};

// Arbitrary size for arrays.
inline static constexpr auto kArraySize = 10;

// Return type for functions.
struct ReturnType {};

// Arg type for functions.
struct Arg1 {};

// Second arg type for functions.
struct Arg2 {};

// Basic types.
// ----------------------------------------------------------------------------
static_assert(std::is_same_v<Canonical<Type>, Type>);
static_assert(std::is_same_v<Canonical<Type&>, Type>);
static_assert(std::is_same_v<Canonical<Type&&>, Type>);
static_assert(std::is_same_v<Canonical<Type const>, Type>);
static_assert(std::is_same_v<Canonical<Type volatile>, Type>);
static_assert(std::is_same_v<Canonical<Type*>, Type>);

// Function types.
// ----------------------------------------------------------------------------
static_assert(std::is_same_v<Canonical<void()>, void (*)()>);
static_assert(std::is_same_v<Canonical<void (*)()>, void (*)()>);
static_assert(std::is_same_v<Canonical<ReturnType()>, ReturnType (*)()>);
static_assert(std::is_same_v<Canonical<ReturnType (*)()>, ReturnType (*)()>);
static_assert(std::is_same_v<Canonical<void(Arg1)>, void (*)(Arg1)>);
static_assert(std::is_same_v<Canonical<void (*)(Arg1)>, void (*)(Arg1)>);
static_assert(
    std::is_same_v<Canonical<ReturnType(Arg1)>, ReturnType (*)(Arg1)>);
static_assert(
    std::is_same_v<Canonical<ReturnType (*)(Arg1)>, ReturnType (*)(Arg1)>);
static_assert(
    std::is_same_v<Canonical<void(Arg1, Arg2)>, void (*)(Arg1, Arg2)>);
static_assert(
    std::is_same_v<Canonical<void (*)(Arg1, Arg2)>, void (*)(Arg1, Arg2)>);
static_assert(std::is_same_v<Canonical<ReturnType(Arg1, Arg2)>,
                             ReturnType (*)(Arg1, Arg2)>);
static_assert(std::is_same_v<Canonical<ReturnType (*)(Arg1, Arg2)>,
                             ReturnType (*)(Arg1, Arg2)>);

// Array types.
// ----------------------------------------------------------------------------
static_assert(std::is_same_v<Canonical<Type[]>, Type>);
static_assert(std::is_same_v<Canonical<Type const[]>, Type>);
static_assert(std::is_same_v<Canonical<Type[kArraySize]>, Type>);
static_assert(std::is_same_v<Canonical<Type const[kArraySize]>, Type>);

// Composite types.
// ----------------------------------------------------------------------------
static_assert(std::is_same_v<Canonical<std::reference_wrapper<Type>>, Type>);
static_assert(std::is_same_v<Canonical<std::unique_ptr<Type>>, Type>);
static_assert(std::is_same_v<Canonical<std::unique_ptr<Type, Deleter>>, Type>);
static_assert(std::is_same_v<Canonical<std::shared_ptr<Type>>, Type>);
static_assert(std::is_same_v<Canonical<std::weak_ptr<Type>>, Type>);

// Type combinations.
// ----------------------------------------------------------------------------
static_assert(std::is_same_v<Canonical<Type const&>, Type>);
static_assert(std::is_same_v<Canonical<Type volatile*>, Type>);
static_assert(std::is_same_v<Canonical<Type const*&>, Type>);
static_assert(std::is_same_v<Canonical<Type**>, Type>);
static_assert(std::is_same_v<Canonical<Type const* volatile*&&>, Type>);
static_assert(
    std::is_same_v<Canonical<ReturnType (*const)()>, ReturnType (*)()>);
static_assert(std::is_same_v<Canonical<ReturnType (*volatile&)(Arg1)>,
                             ReturnType (*)(Arg1)>);
static_assert(std::is_same_v<Canonical<ReturnType (*&&)(Arg1, Arg2)>,
                             ReturnType (*)(Arg1, Arg2)>);

static_assert(std::is_same_v<Canonical<std::reference_wrapper<Type&>>, Type>);
static_assert(
    std::is_same_v<Canonical<std::reference_wrapper<Type const&>>, Type>);
static_assert(
    std::is_same_v<Canonical<std::reference_wrapper<Type> const>, Type>);
static_assert(
    std::is_same_v<Canonical<std::reference_wrapper<Type const>>, Type>);
static_assert(
    std::is_same_v<Canonical<std::reference_wrapper<Type const> const>, Type>);

static_assert(std::is_same_v<Canonical<std::unique_ptr<Type> const&>, Type>);
static_assert(std::is_same_v<Canonical<std::shared_ptr<Type*>>, Type>);
static_assert(std::is_same_v<Canonical<std::shared_ptr<Type const[]>>, Type>);
static_assert(std::is_same_v<
              Canonical<std::shared_ptr<Type const[kArraySize]> const&>, Type>);

static_assert(std::is_same_v<Canonical<std::weak_ptr<Type const>>, Type>);
static_assert(std::is_same_v<Canonical<std::weak_ptr<Type> const>, Type>);
static_assert(std::is_same_v<Canonical<std::weak_ptr<Type const> const>, Type>);

static_assert(
    std::is_same_v<
        Canonical<std::unique_ptr<std::reference_wrapper<Type const&>> const&>,
        Type>);

}  // namespace
}  // namespace dink
