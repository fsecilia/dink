/*
  Copyright (c) 2025 Frank Secilia
  SPDX-License-Identifier: MIT
*/

#include "arity.hpp"
#include <dink/test.hpp>

namespace dink::detail::arity {
namespace {

/*
  Terse Types
  -----------------------------------------------------------------------------
  The static assert blocks below that use these types get a bit jumbled if
  they are named more traditionally. Here, we use terse names just to keep the
  blocks aligned.
*/

// Argument types
struct A0 {};
struct A1 {};
struct A2 {};
struct A3 {};

// Return type
struct R {
  int_t value;
  auto operator==(const R&) const noexcept -> bool = default;
};

// Derived return type
struct D : R {
  auto operator==(const D&) const noexcept -> bool = default;
};

// Probe type
using P = Probe;

/*
  Target Types
  -----------------------------------------------------------------------------
*/

struct DefaultConstructed {
  DefaultConstructed() {}
};

struct SingleValueConstructed {
  SingleValueConstructed(A0) {}
};

struct MultipleValueConstructed {
  MultipleValueConstructed(const A0&, A1&, A2&&) {}
};

struct CopyMoveConstructed {
  CopyMoveConstructed() = delete;
  CopyMoveConstructed(const CopyMoveConstructed&) = default;
  CopyMoveConstructed(CopyMoveConstructed&&) = default;
};

struct SingleValueAndCopyConstructed {
  SingleValueAndCopyConstructed(A0) {}
  SingleValueAndCopyConstructed(const SingleValueAndCopyConstructed&) = default;
};

/*
  -----------------------------------------------------------------------------
  Match
  -----------------------------------------------------------------------------
  Match is generously about 20 lines, but it interacts with the type system,
  which has a massive surface area, so it requires quite a bit of testing.
  Particularly, the result of std::invoke_result_t is effectively arbitrary, so
  we test its interaction with std::is_same by testing not only exact matches,
  but also types in the qualification neighborhood that have implicit
  conversions such as references, const, and derived types.
*/

/*
  Factory Arity Match
  -----------------------------------------------------------------------------
  arity-0
*/
static_assert(match<R, R (*)()>);

// arity-1, one test per equivalence class
static_assert(match<R, R (*)(A0), P>);
static_assert(match<R, R (*)(A0&), P>);
static_assert(match<R, R (*)(const A0&), P>);
static_assert(match<R, R (*)(A0&&), P>);
static_assert(match<R, R (*)(A0*), P>);
static_assert(match<R, R (*)(const A0*), P>);

// arity-2
static_assert(match<R, R (*)(A0, A1), P, P>);
static_assert(match<R, R (*)(const A0&, A1&&), P, P>);

// arity-3
static_assert(match<R, R (*)(A0, A1, A2), P, P, P>);
static_assert(match<R, R (*)(A0*, const A1&, A2&&), P, P, P>);

/*
  Factory Arity Mismatch
  -----------------------------------------------------------------------------
*/
static_assert(!match<R, R (*)(), P>);
static_assert(!match<R, R (*)(A0)>);
static_assert(!match<R, R (*)(A0, A1), P>);
static_assert(!match<R, R (*)(A0, A1), P, P, P>);

/*
  Factory Return Match
  -----------------------------------------------------------------------------
*/
static_assert(match<R&, R& (*)()>);
static_assert(match<R*, R* (*)()>);
static_assert(match<D&, D& (*)()>);
static_assert(match<void, void (*)()>);

/*
  Factory Return Mismatch
  -----------------------------------------------------------------------------
*/
static_assert(!match<R, R& (*)()>);
static_assert(!match<R&, R (*)()>);
static_assert(!match<R&, D& (*)()>);
static_assert(!match<D&, R& (*)()>);

/*
  Ctor Arity Match
  -----------------------------------------------------------------------------
*/
static_assert(match<DefaultConstructed, void>);
static_assert(
    match<SingleValueConstructed, void, SingleProbe<SingleValueConstructed>>);
static_assert(match<MultipleValueConstructed, void, Probe, Probe, Probe>);

/*
  Ctor Arity Mismatch
  -----------------------------------------------------------------------------
*/
static_assert(
    !match<DefaultConstructed, void, SingleProbe<DefaultConstructed>>);
static_assert(!match<SingleValueConstructed, void>);
static_assert(!match<MultipleValueConstructed, void, Probe, Probe>);
static_assert(
    !match<MultipleValueConstructed, void, Probe, Probe, Probe, Probe>);

/*
  Special Member Functions Match
  -----------------------------------------------------------------------------
*/
static_assert(match<CopyMoveConstructed, void, Probe>);
static_assert(match<SingleValueAndCopyConstructed, void,
                    SingleProbe<SingleValueAndCopyConstructed>>);

/*
  Special Member Functions Mismatch
  -----------------------------------------------------------------------------
*/
static_assert(
    !match<CopyMoveConstructed, void, SingleProbe<CopyMoveConstructed>>);

/*
  -----------------------------------------------------------------------------
  Search
  -----------------------------------------------------------------------------
*/

template <typename... Args>
struct Constructed {
  Constructed(Args...) noexcept {}
};

template <typename... Args>
struct Factory {
  auto operator()(Args&&... args) const noexcept -> Constructed<Args...> {
    return Constructed{std::forward<Args>(args)...};
  }
};

struct MultipleArityCtorConstructed {
  MultipleArityCtorConstructed();
  MultipleArityCtorConstructed(A0);
  MultipleArityCtorConstructed(A0, A1, A2);
};

/*
  Matching Factories
  -----------------------------------------------------------------------------
*/
static_assert(search<Constructed<>, Factory<>> == 0);
static_assert(search<Constructed<A0>, Factory<A0>> == 1);
static_assert(search<Constructed<A0, A1>, Factory<A0, A1>> == 2);
static_assert(search<Constructed<A0, A1, A2>, Factory<A0, A1, A2>> == 3);

/*
  Matching Ctors
  -----------------------------------------------------------------------------
*/
static_assert(search<Constructed<>, void> == 0);
static_assert(search<Constructed<A0>, void> == 1);
static_assert(search<Constructed<A0, A1>, void> == 2);
static_assert(search<Constructed<A0, A1, A2>, void> == 3);

/*
  Invocable factory but Mismatched Return Value
  -----------------------------------------------------------------------------
*/
static_assert(search<Constructed<>, Factory<A0>> == not_found);
static_assert(search<Constructed<A0>, Factory<>> == not_found);

/*
  Search Interactions with SingleProbe
  -----------------------------------------------------------------------------
*/
static_assert(search<SingleValueConstructed, void> == 1);
static_assert(search<SingleValueAndCopyConstructed, void> == 1);
static_assert(search<CopyMoveConstructed, void> == not_found);

/*
  Type with Multiple Arity Ctors Chooses Greediest
  -----------------------------------------------------------------------------
*/
static_assert(search<MultipleArityCtorConstructed, void> == 3);

/*
  Max Arity
  -----------------------------------------------------------------------------
*/

// Contains a Constructed and Factory taking A0 repeated once per index.
template <typename IndexSequence>
struct TypesByIndexSequence;

// Specialization cracks sequence to get actual indices.
template <std::size_t... indices>
struct TypesByIndexSequence<std::index_sequence<indices...>> {
  using Constructed = Constructed<meta::IndexedType<A0, indices>...>;
  using Factory = Factory<meta::IndexedType<A0, indices>...>;
};

// Contains a Constructed and Factory with given arity by repeating A0.
template <std::size_t arity>
using TypesByArity = TypesByIndexSequence<std::make_index_sequence<arity>>;

// Test that max arity is found.
using TypesByMaxArity = TypesByArity<dink_max_deduced_arity>;
static_assert(search<TypesByMaxArity::Constructed, void> ==
              dink_max_deduced_arity);
static_assert(search<TypesByMaxArity::Constructed, TypesByMaxArity::Factory> ==
              dink_max_deduced_arity);

// Test that exceeding max arity is not found.
using TypesExceedingMaxArity = TypesByArity<dink_max_deduced_arity + 1>;
static_assert(search<TypesExceedingMaxArity::Constructed, void> == not_found);
static_assert(search<TypesExceedingMaxArity::Constructed,
                     TypesExceedingMaxArity::Factory> == not_found);

/*
  -----------------------------------------------------------------------------
  arity
  -----------------------------------------------------------------------------
  arity is just an alias for detail::arity::search, wrapped with an assert to
  guard against not_found. We smoke test it here to make sure the assert
  doesn't fire.
*/

static_assert(dink::arity<Constructed<>, Factory<>> == 0);
static_assert(dink::arity<Constructed<A0>, Factory<A0>> == 1);
static_assert(dink::arity<Constructed<A0, A1>, Factory<A0, A1>> == 2);
static_assert(dink::arity<Constructed<A0, A1, A2>, Factory<A0, A1, A2>> == 3);

static_assert(dink::arity<Constructed<>, void> == 0);
static_assert(dink::arity<Constructed<A0>, void> == 1);
static_assert(dink::arity<Constructed<A0, A1>, void> == 2);
static_assert(dink::arity<Constructed<A0, A1, A2>, void> == 3);

}  // namespace
}  // namespace dink::detail::arity
