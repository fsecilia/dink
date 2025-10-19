// \file
// Copyright (c) 2025 Frank Secilia
// SPDX-License-Identifier: MIT

#pragma once

#include <dink/config.gen.hpp>
#include <cstdint>

namespace dink {

// ----------------------------------------------------------------------------
// processor word size ints
//
// Default integer types are the same size as the processor word.
// ----------------------------------------------------------------------------

using int_t = std::intptr_t;
using uint_t = std::uintptr_t;

}  // namespace dink
