// \file
// Copyright (c) 2025 Frank Secilia
// SPDX-License-Identifier: MIT

#pragma once

#include <dink/lib.hpp>
#include <memory>
#include <type_traits>

namespace dink {

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
