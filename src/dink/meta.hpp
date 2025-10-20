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

}  // namespace dink
