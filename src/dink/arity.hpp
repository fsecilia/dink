// \file
// Copyright (c) 2025 Frank Secilia
// SPDX-License-Identifier: MIT

#pragma once

#include <dink/lib.hpp>
#include <dink/meta.hpp>

namespace dink {
namespace arity::detail {

// ----------------------------------------------------------------------------
// Probes
// ----------------------------------------------------------------------------
// Probes are lightweight, match-any types passed as arguments to ctors and
// call operators to determine how many are needed to form a valid invocation.

//! Probes individual constructor/function arguments.
struct Probe {
  template <typename Deduced>
  operator Deduced();

  template <typename Deduced>
  operator Deduced&() const;
};

//! Probe for single-argument construction; doesn't match move or copy ctors.
//
// Trying to match one Probe against a ctor will match copy ctors and move
// ctors. Trying to match one SingleProbe will not.
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

// ----------------------------------------------------------------------------
// Match
// ----------------------------------------------------------------------------

//! Checks if Factory(Probes...) produces Constructed.
template <typename Constructed, typename Factory, typename... Probes>
struct Match {
  static constexpr auto value = []() constexpr {
    if constexpr (std::is_invocable_v<Factory, Probes...>) {
      return std::is_same_v<Constructed,
                            std::invoke_result_t<Factory, Probes...>>;
    } else {
      return false;
    }
  }();
};

//! Specialization for void Factory checks Constructed's constructor.
template <typename Constructed, typename... Probes>
struct Match<Constructed, void, Probes...> {
  static constexpr auto value = std::is_constructible_v<Constructed, Probes...>;
};

//! true if an arity match is found.
template <typename Constructed, typename Factory, typename... Probes>
inline constexpr auto match = Match<Constructed, Factory, Probes...>::value;

}  // namespace arity::detail
}  // namespace dink
