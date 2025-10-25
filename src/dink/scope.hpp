// \file
// Copyright (c) 2025 Frank Secilia
// SPDX-License-Identifier: MIT

#pragma once

#include <dink/lib.hpp>

namespace dink::scope {

//! resolves one instance per request
struct Transient {
  //! resolves instance in requested form
  template <typename Requested, typename Container, typename Provider>
  auto resolve(Container& container, Provider& provider) -> Requested {
    return provider.template create<Requested>(container);
  }
};

//! resolves one instance per provider
struct Singleton {
  //! resolves instance in requested form
  template <typename Requested, typename Container, typename Provider>
  auto resolve(Container& container, Provider& provider) -> Requested& {
    static auto instance = provider.template create<Requested>(container);
    return instance;
  }
};

}  // namespace dink::scope
