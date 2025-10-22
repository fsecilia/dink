/*!
    \file
    Copyright (C) 2025 Frank Secilia
*/

#pragma once

#include <dink/lib.hpp>

namespace dink::scope {

//! orders scopes by lifetime
//
// Instances resolved with a particular scope cannot depend on instances
// resolved with a shorter scope. Lifetime give scopes an order.
enum class Lifetime { kTransient, kSingleton };

//! nominally resolves new instances per request
//
// Instances resolved from transient scope are normally created per request,
// meaning a unique instance is constructed from a provider for every request,
// and the instance is returned with value semantics. A request for a transient
// instance can be promoted to singleton scope if the type of the request
// requires it, such as requesting a reference or pointer.
struct transient_t {
  static inline constexpr auto lifetime = Lifetime::kTransient;
};

//! nominally resolves shared instances per request
//
// Instances resolved from singleton scope are normally cached by the
// requesting container, meaning they are constructed from a provider once,
// saved in the local cache, and the cached instance is returned with reference
// semantics. A request for a singleton instance can be relegated to transient
// scope if the type of the request requires it, such as requesting a
// unique_ptr or rvalue reference. Relegated requests are initialized with a
// copy of the cached singleton.
struct singleton_t {
  static inline constexpr auto lifetime = Lifetime::kSingleton;
};

}  // namespace dink::scope
