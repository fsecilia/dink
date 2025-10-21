// \file
// Copyright (c) 2025 Frank Secilia
// SPDX-License-Identifier: MIT

#include "arity.hpp"
#include <dink/test.hpp>

namespace dink {
namespace arity::detail {
namespace {

// Terse Types
// ----------------------------------------------------------------------------
// The static assert blocks below that use these types get a bit jumbled if
// they are named more traditionally. Here, we use terse names just to keep the
// blocks aligned.

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

// Target Types
// ----------------------------------------------------------------------------

struct DefaultConstructed {
  DefaultConstructed() {}
};

struct SingleValueConstructed {
  SingleValueConstructed(A0) {}
};

struct MultipleValueConstructed {
  MultipleValueConstructed(const A0&, A1&, A2&&) {}
};

struct CopyConstructed {
  CopyConstructed() = delete;
  CopyConstructed(const CopyConstructed&) = default;
};

struct MoveConstructed {
  MoveConstructed() = delete;
  MoveConstructed(MoveConstructed&&) = default;
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

// ----------------------------------------------------------------------------
// Match
// ----------------------------------------------------------------------------
// Match is generously about 20 lines, but it interacts with the type system,
// which has a massive surface area, so it requires quite a bit of testing.
// Particularly, the result of std::invoke_result_t is effectively arbitrary,
// so we test its interaction with std::is_same by testing not only exact
// matches, but also types in the qualification neighborhood that have implicit
// conversions such as references, const, and derived types.

// Factory Arity Match
// ----------------------------------------------------------------------------
// arity-0
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
static_assert(match<R, R (*)(A0*, A1&, A2&&), P, P, P>);

// Factory Arity Mismatch
// ----------------------------------------------------------------------------
static_assert(!match<R, R (*)(), P>);
static_assert(!match<R, R (*)(A0)>);
static_assert(!match<R, R (*)(A0, A1), P>);
static_assert(!match<R, R (*)(A0, A1), P, P, P>);

// Factory Return Match
// ----------------------------------------------------------------------------
static_assert(match<R&, R& (*)()>);
static_assert(match<R*, R* (*)()>);
static_assert(match<D&, D& (*)()>);
static_assert(match<void, void (*)()>);

// Factory Return Mismatch
// ----------------------------------------------------------------------------
static_assert(!match<R, R& (*)()>);
static_assert(!match<R&, R (*)()>);
static_assert(!match<R&, D& (*)()>);
static_assert(!match<D&, R& (*)()>);

// Ctor Arity Match
// ----------------------------------------------------------------------------
static_assert(match<DefaultConstructed, void>);
static_assert(
    match<SingleValueConstructed, void, SingleProbe<SingleValueConstructed>>);
static_assert(match<MultipleValueConstructed, void, Probe, Probe, Probe>);
static_assert(match<SingleValueAndCopyConstructed, void,
                    SingleProbe<SingleValueAndCopyConstructed>>);

// Ctor Arity Mismatch
// ----------------------------------------------------------------------------
static_assert(
    !match<DefaultConstructed, void, SingleProbe<DefaultConstructed>>);
static_assert(!match<SingleValueConstructed, void>);
static_assert(!match<MultipleValueConstructed, void, Probe, Probe>);
static_assert(
    !match<MultipleValueConstructed, void, Probe, Probe, Probe, Probe>);

// Special Member Functions Match
// ----------------------------------------------------------------------------
static_assert(match<CopyConstructed, void, Probe>);
static_assert(match<MoveConstructed, void, Probe>);
static_assert(match<CopyMoveConstructed, void, Probe>);

// Special Member Functions Mismatch
// ----------------------------------------------------------------------------
static_assert(!match<CopyConstructed, void, SingleProbe<CopyConstructed>>);
static_assert(!match<MoveConstructed, void, SingleProbe<MoveConstructed>>);
static_assert(
    !match<CopyMoveConstructed, void, SingleProbe<CopyMoveConstructed>>);

}  // namespace
}  // namespace arity::detail
}  // namespace dink
