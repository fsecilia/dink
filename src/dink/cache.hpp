/*!
  \file
  \brief Provides per-type and per-instance caches.

  \copyright Copyright (c) 2025 Frank Secilia \n
  SPDX-License-Identifier: MIT
*/

#pragma once

#include <dink/lib.hpp>
#include <any>
#include <typeindex>
#include <unordered_map>

namespace dink::cache {

class Type {
 public:
  template <typename Container, typename Provider>
  auto get_or_create(Container& container, Provider& provider) ->
      typename Provider::Provided& {
    using Provided = typename Provider::Provided;

    static auto instance = provider.template create<Provided>(container);

    return instance;
  }
};

class Instance {
 public:
  template <typename Container, typename Provider>
  auto get_or_create(Container& container, Provider& provider) ->
      typename Provider::Provided& {
    using Provided = typename Provider::Provided;

    /*
      This keys on *Provider*, not Provided, so it matches semantics with the
      Meyers singleton in cache::Type.
    */
    auto& instance = map_[typeid(Provider)];
    if (!instance.has_value()) {
      instance = provider.template create<Provided>(container);
    }

    return std::any_cast<Provided&>(instance);
  }

 private:
  std::unordered_map<std::type_index, std::any> map_;
};

template <typename Cache, typename Container, typename Provider>
concept IsCache =
    requires(Cache& cache, Container& container, Provider& provider) {
      {
        cache.get_or_create(container, provider)
      } -> std::same_as<typename Provider::Provider>;
    };

}  // namespace dink::cache
