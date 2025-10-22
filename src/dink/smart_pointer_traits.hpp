// \file
// Copyright (c) 2025 Frank Secilia
// SPDX-License-Identifier: MIT

#pragma once

#include <dink/lib.hpp>
#include <memory>
#include <type_traits>

namespace dink {

// ----------------------------------------------------------------------------
// shared_ptr
// ----------------------------------------------------------------------------

template <typename>
struct IsSharedPtr : std::false_type {};

template <typename Element>
struct IsSharedPtr<std::shared_ptr<Element>> : std::true_type {};

template <typename Type>
constexpr bool is_shared_ptr = IsSharedPtr<Type>::value;

template <typename Type>
concept SharedPtr = is_shared_ptr<std::remove_cvref_t<Type>>;

// ----------------------------------------------------------------------------
// unique_ptr
// ----------------------------------------------------------------------------

template <typename>
struct IsUniquePtr : std::false_type {};

template <typename Element, typename Deleter>
struct IsUniquePtr<std::unique_ptr<Element, Deleter>> : std::true_type {};

template <typename Type>
constexpr bool is_unique_ptr = IsUniquePtr<Type>::value;

template <typename Type>
concept UniquePtr = is_unique_ptr<std::remove_cvref_t<Type>>;

}  // namespace dink
