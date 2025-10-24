/*!
    \file
    Copyright (C) 2025 Frank Secilia
*/

#pragma once

#include <dink/lib.hpp>

namespace dink::scope {

//! resolves new instances per request
struct Transient {
};

//! resolves shared instances across requests
struct Singleton {
};

}  // namespace dink::scope
