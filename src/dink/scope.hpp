// \file
// Copyright (c) 2025 Frank Secilia
// SPDX-License-Identifier: MIT

#pragma once

#include <dink/lib.hpp>
#include <concepts>

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

///! resolves one instance per provider
class Singleton {
 public:
  //! resolves instance in requested form
  template <typename Requested, typename Container, typename Provider>
  auto resolve(Container& container, Provider& provider) -> Requested& {
    static_assert(std::same_as<Requested, typename Provider::Provided>);
    return cached_instance(container, provider);
  }

 private:
  template <typename Container, typename Provider>
  auto cached_instance(Container& container, Provider& provider)
      -> Provider::Provided& {
    static auto instance =
        provider.template create<typename Provider::Provided>(container);
    return instance;
  }
};

}  // namespace dink::scope
