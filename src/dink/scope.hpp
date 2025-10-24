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
  auto create(Container& container, Provider& provider) -> auto {
    return provider.provide(container);
  }
};

//! resolves shared instances across requests
struct Singleton {
  template <typename Container, typename Provider>
  auto create(Container& container, Provider& provider) -> auto& {
    static auto instance = provider.provide(container);
    return instance;
  }
};

}  // namespace dink::scope
