// \file
// Copyright (c) 2025 Frank Secilia
// SPDX-License-Identifier: MIT

#pragma once

#include <dink/lib.hpp>

namespace dink {

//! constexpr bool, but dependent on a template parameter to delay evaluation.
//
// This is useful to trigger a static assert conditionally, using Context to
// make the expression dependent. The static assert message will also contain
// information related to Context.
template <bool condition, typename Context>
constexpr auto kDependentBool = condition;

//! constexpr false, but dependent on a template parameter to delay evaluation.
//
// This is useful to trigger a static assert unconditionally, using Context to
// make the expression dependent. The static assert message will also contain
// information related to Context.
//
// \sa kDependentBool
template <typename Context>
constexpr auto kDependentFalse = kDependentBool<false, Context>;

//! Consumes an index to produce a type.
//
// This type is used to repeat a type n times by consuming the indices of an
// index sequence of length n.
template <typename Type, std::size_t index>
using IndexedType = Type;

}  // namespace dink
