// \file
// Copyright (c) 2025 Frank Secilia
// SPDX-License-Identifier: MIT

#pragma once

#include <dink/lib.hpp>
#include <dink/binding.hpp>
#include <dink/config.hpp>
#include <dink/meta.hpp>

namespace dink {

// ----------------------------------------------------------------------------
// Concepts
// ----------------------------------------------------------------------------

template <typename container_t>
concept IsContainer = requires(container_t& container) {
  {
    container.template resolve<meta::ConceptProbe>()
  } -> std::same_as<meta::ConceptProbe>;
};

template <IsConfig Config, typename Parent>
class container_t;

template <IsConfig Config>
class container_t<Config, void> {
 public:
  //! constructs root container with given bindings
  template <IsBinding... Bindings>
  explicit container_t(Bindings&&... bindings) noexcept
      : config_{make_bindings(std::forward<Bindings>(bindings)...)} {}

  //! direct construction from components
  container_t(Config config) noexcept : config_{std::move(config)} {}

 private:
  Config config_{};
};

}  // namespace dink
