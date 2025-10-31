// \file
// Copyright (c) 2025 Frank Secilia
// SPDX-License-Identifier: MIT
//
// \brief Finds greediest arity of a factory or ctor producing a specific type.

#pragma once

#include <dink/lib.hpp>
#include <dink/meta.hpp>
#include <concepts>
#include <utility>

#if !defined dink_max_deduced_arity
#define dink_max_deduced_arity 16
#endif

namespace dink {
namespace detail::arity {

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
// Trying to match one Probe against a ctor will match copy and move ctors.
// Trying to match one SingleProbe<Constructed> will not match copy and move
// ctors for Constructed.
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

//! Aliases to SingleProbe when invoking ctor and arity is 1, Probe otherwise.
template <typename Constructed, bool invoking_ctor, std::size_t arity>
using InitialProbe = std::conditional_t<invoking_ctor && arity == 1,
                                        SingleProbe<Constructed>, Probe>;

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

// ----------------------------------------------------------------------------
// Search
// ----------------------------------------------------------------------------

//! Searches by decreasing arity for matching invocation.
//
// Search uses Match to find the greediest invocation of either Factory's call
// operator or Constructed's ctor. It tests by arity, decrements arity and
// slices the index sequence, then tries again. The search stops when a
// matching invocation is found, or after arity 0 is tested.
template <typename Constructed, typename Factory, bool invoking_ctor,
          std::size_t arity, typename IndexSequence>
struct Search;

//! Base Case: match found
//
// The recursive Search is short circuted when a match is found by
// instantiating a Found at the next level instead of another Search. This
// effectively chops the recursive tail instead of always recursing to arity 0.
template <std::size_t arity>
struct Found {
  static constexpr auto value = arity;
};

//! Base Case: match not found
//
// If the recursive chain of Search instantiations runs past 0 without finding
// a match, the next level instantiates a NotFound instead of another Search.
struct NotFound {
  static constexpr auto value = std::size_t(-1);
};
inline constexpr auto not_found = NotFound::value;

//! Recursive Case: check for match with given arity, then try next smaller
template <typename Constructed, typename Factory, bool invoking_ctor,
          std::size_t arity, std::size_t index,
          std::size_t... remaining_indices>
struct Search<Constructed, Factory, invoking_ctor, arity,
              std::index_sequence<index, remaining_indices...>>
    : std::conditional_t<match<Constructed, Factory,
                               InitialProbe<Constructed, invoking_ctor, arity>,
                               IndexedProbe<remaining_indices>...>,
                         Found<arity>,
                         Search<Constructed, Factory, invoking_ctor, arity - 1,
                                std::index_sequence<remaining_indices...>>> {};

//! Base case: check zero-argument construction
template <typename Constructed, typename Factory, bool invoking_ctor>
struct Search<Constructed, Factory, invoking_ctor, 0, std::index_sequence<>>
    : std::conditional_t<match<Constructed, Factory>, Found<0>, NotFound> {};

//! Arity of greediest factory or ctor call to produce Constructed.
//
// If factory is void, Constructed's ctor is investigated. If no matching
// ctor or call operator is found, the result is not_found.
template <typename Constructed, typename Factory>
inline constexpr std::size_t search =
    Search<Constructed, Factory, std::same_as<Factory, void>,
           dink_max_deduced_arity,
           std::make_index_sequence<dink_max_deduced_arity>>::value;

// ----------------------------------------------------------------------------
// Arity
// ----------------------------------------------------------------------------

//! Successfully calculates ctor or factory arity or asserts.
template <typename Constructed, typename Factory>
struct AssertedArity {
  static constexpr auto value = search<Constructed, Factory>;
  static_assert(value != not_found, "could not deduce arity");
};

}  // namespace detail::arity

/*!
    Finds largest arity that constructs or produces Constructed.

    If Factory is callable, searches for largest arity call operator returning
    Constructed. If Factory is void, searches Constructed's constructors
    directly.

    Triggers compile error if no matching arity found.
*/
template <typename Constructed, typename Factory = void>
inline constexpr std::size_t arity =
    detail::arity::AssertedArity<Constructed, Factory>::value;

}  // namespace dink
