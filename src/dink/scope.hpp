/*!
    \file
    Copyright (C) 2025 Frank Secilia
*/

#pragma once

#include <dink/lib.hpp>

namespace dink::scope {

//! nominally resolves new instances per request
//
// Instances resolved from transient scope are normally created per request,
// meaning a unique instance is constructed from a provider for every request,
// and the instance is returned with value semantics. A request for a transient
// instance can be promoted to singleton scope if the type of the request
// requires it, such as requesting a reference or pointer.
struct transient_t {};

}  // namespace dink::scope
