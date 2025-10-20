// \file
// Copyright (c) 2025 Frank Secilia
// SPDX-License-Identifier: MIT

#pragma once

#include <dink/lib.hpp>
#include <dink/meta.hpp>

namespace dink {
namespace arity::detail {

//! Probes individual constructor/function arguments.
struct Probe {
  template <typename Deduced>
  operator Deduced();

  template <typename Deduced>
  operator Deduced&() const;
};

//! Specialized probe for single-argument construction.
template <typename Resolved>
struct SingleProbe {
  template <meta::DifferentUnqualifiedType<Resolved> Deduced>
  operator Deduced();

  template <meta::DifferentUnqualifiedType<Resolved> Deduced>
  operator Deduced&() const;
};

}  // namespace arity::detail
}  // namespace dink
