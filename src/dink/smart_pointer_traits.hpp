// \file
// Copyright (c) 2025 Frank Secilia
// SPDX-License-Identifier: MIT

#pragma once

#include <dink/lib.hpp>
#include <memory>
#include <type_traits>

namespace dink {

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

}  // namespace dink
