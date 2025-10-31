// \file
// Copyright (c) 2025 Frank Secilia
// SPDX-License-Identifier: MIT
//
// \brief Metaprogramming support module.

#pragma once

#include <dink/lib.hpp>
#include <concepts>
#include <memory>
#include <type_traits>

namespace dink::meta {

//! constexpr bool, but dependent on a template parameter to delay evaluation.
//
// This is useful to trigger a static assert conditionally, using Context to
// make the expression dependent. The static assert message will also contain
// information related to Context.
template <bool condition, typename Context>
inline constexpr auto kDependentBool = condition;

//! constexpr false, but dependent on a template parameter to delay evaluation.
//
// This is useful to trigger a static assert unconditionally, using Context to
// make the expression dependent. The static assert message will also contain
// information related to Context.
//
// \sa kDependentBool
template <typename Context>
inline constexpr auto kDependentFalse = kDependentBool<false, Context>;

//! Consumes an index to produce a type.
//
// This type is used to repeat a type n times by consuming the indices of an
// index sequence of length n.
template <typename Type, std::size_t index>
using IndexedType = Type;

//! Filters types that match Unqualified after removing cv and ref qualifiers.
//
// This is used to filter signatures that match copy or move ctors.
template <typename Qualified, typename Unqualified>
concept DifferentUnqualifiedType =
    !std::same_as<std::remove_cvref_t<Qualified>, Unqualified>;

//! arbitrary probing type for abstract concepts
//
// Some concepts check things that are so abstract, all you know is it should
// accept, return, or contain a type. ConceptProbe is meant as a type
// standardized for this purpose.
struct ConceptProbe {};

// ----------------------------------------------------------------------------
// IsSharedPtr
// ----------------------------------------------------------------------------

namespace traits {

template <typename>
struct IsSharedPtr : std::false_type {};

template <typename Element>
struct IsSharedPtr<std::shared_ptr<Element>> : std::true_type {};

template <typename Type>
constexpr bool is_shared_ptr = IsSharedPtr<Type>::value;

}  // namespace traits

template <typename Type>
concept IsSharedPtr = traits::is_shared_ptr<std::remove_cvref_t<Type>>;

// ----------------------------------------------------------------------------
// IsUniquePtr
// ----------------------------------------------------------------------------

namespace traits {

template <typename>
struct IsUniquePtr : std::false_type {};

template <typename Element, typename Deleter>
struct IsUniquePtr<std::unique_ptr<Element, Deleter>> : std::true_type {};

template <typename Type>
constexpr bool is_unique_ptr = IsUniquePtr<Type>::value;

}  // namespace traits

template <typename Type>
concept IsUniquePtr = traits::is_unique_ptr<std::remove_cvref_t<Type>>;

// ----------------------------------------------------------------------------
// IsWeakPtr
// ----------------------------------------------------------------------------

namespace traits {

template <typename>
struct IsWeakPtr : std::false_type {};

template <typename Element>
struct IsWeakPtr<std::weak_ptr<Element>> : std::true_type {};

template <typename Type>
constexpr bool is_weak_ptr = IsWeakPtr<Type>::value;

}  // namespace traits

template <typename Type>
concept IsWeakPtr = traits::is_weak_ptr<std::remove_cvref_t<Type>>;

// ----------------------------------------------------------------------------
// RemoveRvalueRef
// ----------------------------------------------------------------------------

namespace traits {

template <typename Type>
struct RemoveRvalueRef;

template <typename Type>
struct RemoveRvalueRef {
  using type = Type;
};

template <typename Type>
struct RemoveRvalueRef<Type&&> {
  using type = Type;
};

}  // namespace traits

template <typename Type>
using RemoveRvalueRef = typename traits::RemoveRvalueRef<Type>::type;

// ----------------------------------------------------------------------------
// UniqueType
// ----------------------------------------------------------------------------

//! Generates a unique type using a lambda's closure type.
//
// Each instantiation returns a different lambda with a unique closure type.
// This provides compile-time uniqueness without macros or non-standard
// features.
template <typename Impl = decltype([] {})>
struct UniqueType {};

}  // namespace dink::meta
