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

//! Probe for single-argument construction; doesn't match move or copy ctors.
template <typename Constructed>
struct SingleProbe {
  template <meta::DifferentUnqualifiedType<Constructed> Deduced>
  operator Deduced();

  template <meta::DifferentUnqualifiedType<Constructed> Deduced>
  operator Deduced&() const;
};

//! Repeats Probe for each index in a sequence.
template <std::size_t Index>
using IndexedProbe = meta::IndexedType<Probe, Index>;

}  // namespace arity::detail
}  // namespace dink
