/*!
    \file
    Copyright (C) 2025 Frank Secilia
*/

#pragma once

#include <dink/lib.hpp>

namespace dink::scope {

//! resolves new instances per request
struct Transient {
  template <typename Container, typename Provider>
  static auto create(Container& container, Provider& provider) -> auto {
    return provider.provide(container);
  }
};

//! resolves shared instances across requests
struct Singleton {
};

}  // namespace dink::scope
