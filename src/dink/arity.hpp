/*!
  \file
  \brief Finds greediest arity of a factory or constructor producing a specific
  type.

  Arity is the number of arguments a function or construtor takes.

  \copyright Copyright (c) 2025 Frank Secilia \n
  SPDX-License-Identifier: MIT
*/

#pragma once

#include <dink/lib.hpp>
#include <dink/meta.hpp>
#include <concepts>
#include <utility>

//! Controls the maximum arity to check.
#if !defined dink_max_deduced_arity
#define dink_max_deduced_arity 16
#endif

namespace dink {

namespace detail::arity {

// -----------------------------------------------------------------------------
/*!
  \defgroup Probes
  @{

  Probes are lightweight, match-any types passed as arguments to constructors
  and call operators to determine how many are needed to form a valid
  invocation.
*/
// -----------------------------------------------------------------------------

//! Probes individual constructor/function arguments.
struct Probe {
  /*!
    Matches all types except lvalue reference to const.

    \note This overload must be non-const or matches against mutable lvalue
    references become ambiguous.

    \tparam Deduced Argument type to convert to.
  */
  template <typename Deduced>
  operator Deduced();

  /*!
    Matches reference to const.

    \note This overload must be const to break ties for lvalue references to
    const.

    \tparam Deduced Argument type to convert to.
  */
  template <typename Deduced>
  operator Deduced&() const;
  //!@}
};

/*!
  Probe used for single-argument construction; doesn't match move or copy
  constructors for parameterized type.

  Trying to match one Probe against a constructor will match copy and move
  constructors for any type. Trying to match one SingleProbe<Resolved> will not
  match copy and move constructors for Resolved.

  \tparam Resolved Type being constructed that deduction should not match.
*/
template <typename Resolved>
struct SingleProbe {
  /*!
    Matches all types except lvalue reference to const and any qualified
    version of Resolved.

    \note This overload must be non-const or matches against mutable lvalue
    references become ambiguous.

    \tparam Deduced Argument type to convert to.
  */
  template <meta::DifferentUnqualifiedType<Resolved> Deduced>
  operator Deduced();

  /*!
    Matches references to const other than reference to const Resolved.

    \note This overload must be const to break ties for lvalue references to
    const.

    \tparam Deduced Argument type to convert to.
  */
  template <meta::DifferentUnqualifiedType<Resolved> Deduced>
  operator Deduced&() const;
};

/*!
  Repeats Probe for each index in a sequence.

  \tparam Index The index this Probe replaces.
*/
template <std::size_t Index>
using IndexedProbe = meta::IndexedType<Probe, Index>;

/*!
  Aliases to SingleProbe when invoking constructor and arity is 1, Probe
  otherwise.

  This probe is used for the first index of a sequence to prevent matching copy
  and move constructors.

  \tparam Resolved The type being constructed that deduction should not match.
  \tparam invoking_ctor True if checking a ctor, false for a factory.
  \tparam arity The arity being tested.
*/
template <typename Resolved, bool invoking_constructor, std::size_t arity>
using InitialProbe = std::conditional_t<invoking_constructor && arity == 1,
                                        SingleProbe<Resolved>, Probe>;

//!@}

/*
  -----------------------------------------------------------------------------
  Match
  -----------------------------------------------------------------------------
*/

/*!
  Matches probes against invocables.

  Match checks if the given probes can produce Resolved, contextually by either
  passing them to a callable factory, or invocking a constructor.

  \tparam Resolved The target type to be produced.
  \tparam Factory Either the factory type, or void to match Resolved's
  constructor directly.
  \tparam Probes The pack of probe arguments passed to the factory or
  constructor.

  Generic case checks if Factory(Probes...) produces Resolved.
*/
template <typename Resolved, typename Factory, typename... Probes>
struct Match {
  //! true if an arity match is found.
  static constexpr auto value = []() constexpr {
    if constexpr (std::is_invocable_v<Factory, Probes...>) {
      return std::is_same_v<Resolved, std::invoke_result_t<Factory, Probes...>>;
    } else {
      return false;
    }
  }();
};

//! Specialization for void Factory checks if Resolved(Probes...) is invocable.
template <typename Resolved, typename... Probes>
struct Match<Resolved, void, Probes...> {
  //! true if an arity match is found.
  static constexpr auto value = std::is_constructible_v<Resolved, Probes...>;
};

/*!
  true if an arity match is found.

  \tparam Resolved The target type to be produced.
  \tparam Factory Either the factory type, or void to match Resolved's
  constructor directly.
  \tparam Probes The pack of probe arguments passed to the factory or
  constructor.
*/
template <typename Resolved, typename Factory, typename... Probes>
inline constexpr auto match = Match<Resolved, Factory, Probes...>::value;

//!@}

/*
  -----------------------------------------------------------------------------
  Search
  -----------------------------------------------------------------------------
*/

/*!
  Base Case: match found

  The recursive Search is short circuted when a match is found by
  instantiating a Found at the next level instead of another Search. This
  effectively chops the recursive tail instead of always recursing to arity 0.

  \tparam arity The arity that was successfully matched.
*/
template <std::size_t arity>
struct Found {
  static constexpr auto value = arity;
};

/*!
  Base Case: match not found

  If the recursive chain of Search instantiations runs past 0 without finding
  a match, the next level instantiates a NotFound instead of another Search.
*/
struct NotFound {
  static constexpr auto value = std::size_t(-1);
};
inline constexpr auto not_found = NotFound::value;

/*!
  Searches by decreasing arity for matching invocation.

  Search uses Match to find the greediest invocation of either Factory's call
  operator or Resolved's constructor. It tests by arity, decrements arity and
  slices the index sequence, then tries again. The search stops when a
  matching invocation is found, or after arity 0 is tested.

  \tparam Resolved The target type to be produced.
  \tparam Factory Either the factory type, or void to search Resolved's
  constructor directly.
  \tparam invoking_ctor True if checking a constructor, false for a factory.
  \tparam arity The arity being tested.
  \tparam IndexSequence The std::index_sequence to replace with probes.
*/
template <typename Resolved, typename Factory, bool invoking_constructor,
          std::size_t arity, typename IndexSequence>
struct Search;

/*!
  Recursive Case: check for match with given arity, then try next smaller

  \tparam Resolved The target type to be produced.
  \tparam Factory Either the factory type, or void to search Resolved's
  constructor directly.
  \tparam invoking_ctor True if checking a constructor, false for a factory.
  \tparam arity The arity being tested.
  \tparam index The first index; sliced off.
  \tparam remaining_indices Indices for remaining probes and the next recursion.
*/
template <typename Resolved, typename Factory, bool invoking_constructor,
          std::size_t arity, std::size_t index,
          std::size_t... remaining_indices>
struct Search<Resolved, Factory, invoking_constructor, arity,
              std::index_sequence<index, remaining_indices...>>
    : std::conditional_t<
          match<Resolved, Factory,
                InitialProbe<Resolved, invoking_constructor, arity>,
                IndexedProbe<remaining_indices>...>,
          Found<arity>,
          Search<Resolved, Factory, invoking_constructor, arity - 1,
                 std::index_sequence<remaining_indices...>>> {};

/*!
  Base case: check zero-argument construction

  \tparam Resolved The target type to be produced.
  \tparam Factory Either the factory type, or void to search Resolved's
  constructor directly.
  \tparam invoking_ctor True if checking a constructor, false for a factory.
*/
template <typename Resolved, typename Factory, bool invoking_constructor>
struct Search<Resolved, Factory, invoking_constructor, 0, std::index_sequence<>>
    : std::conditional_t<match<Resolved, Factory>, Found<0>, NotFound> {};

/*!
  Arity of greediest factory or constructor call to produce Resolved.

  If factory is void, Resolved's constructor is investigated. If no matching
  constructor or call operator is found, the result is not_found.

  \tparam Resolved The target type to be produced.
  \tparam Factory Either the factory type, or void to search Resolved's
  constructor directly.
*/
template <typename Resolved, typename Factory>
inline constexpr std::size_t search =
    Search<Resolved, Factory, std::same_as<Factory, void>,
           dink_max_deduced_arity,
           std::make_index_sequence<dink_max_deduced_arity>>::value;

/*
  -----------------------------------------------------------------------------
  Arity
  -----------------------------------------------------------------------------
*/

/*!
  Successfully calculates constructor or factory arity or asserts.

  \tparam Resolved The target type to be produced.
  \tparam Factory Either the factory type, or void to search Resolved's
  constructor directly.
*/
template <typename Resolved, typename Factory>
struct AssertedArity {
  //! The found arity.
  static constexpr auto value = search<Resolved, Factory>;

  static_assert(value != not_found, "could not deduce arity");
};

}  // namespace detail::arity

/*!
  Contains largest arity that constructs or produces Resolved.

  If Factory is callable, this value contains the arity of its largest-arity
  call operator returning Resolved. If Factory is void, this value contains
  the arity of Resolved's largest-arity constructor.

  A compile error is triggered if no matching arity found.

  \tparam Resolved The target type to be produced.
  \tparam Factory Either the factory type, or void to search Resolved's
  constructor directly.
*/
template <typename Resolved, typename Factory = void>
inline constexpr std::size_t arity =
    detail::arity::AssertedArity<Resolved, Factory>::value;

}  // namespace dink
