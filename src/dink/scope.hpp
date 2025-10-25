// \file
// Copyright (c) 2025 Frank Secilia
// SPDX-License-Identifier: MIT

#pragma once

#include <dink/lib.hpp>

namespace dink::scope {

//! resolves one instance per request
class Transient {
 public:
  //! resolves instance in requested form
  template <typename Requested, typename Container, typename Provider>
  auto resolve(Container& container, Provider& provider) -> Requested {
    return provider.template create<Requested>(container);
  }
};

//! resolves one instance per provider
class Singleton {
 public:
  //! resolves instance in requested form
  template <typename Requested, typename Container, typename Provider>
  auto resolve(Container& container, Provider& provider) -> Requested& {
    static auto instance = provider.template create<Requested>(container);
    return instance;
  }
};

}  // namespace dink::scope
