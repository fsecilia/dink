// \file
// Copyright (c) 2025 Frank Secilia
// SPDX-License-Identifier: MIT

#pragma once

#include <dink/lib.hpp>
#include <functional>
#include <memory>

namespace dink {
namespace detail {

//! Recursively strips Source type of qualifier and wrapper.
//
// This is essentially a fixed-point iteration that strips one qualifier or
// wrapper at every step until a base case with nothing left to strip is
// reached.
template <typename Source>
struct Canonical;

//! Base case: type is unmodified
template <typename Source>
struct Canonical {
  using Type = Source;
};

//! Removes const.
template <typename Source>
struct Canonical<Source const> : Canonical<Source> {};

//! Removes volatile.
template <typename Source>
struct Canonical<Source volatile> : Canonical<Source> {};

//! Removes lvalue reference.
template <typename Source>
struct Canonical<Source&> : Canonical<Source> {};

//! Removes rvalue reference.
template <typename Source>
struct Canonical<Source&&> : Canonical<Source> {};

//! Removes pointer.
template <typename Source>
struct Canonical<Source*> : Canonical<Source> {};

//! Pointers to function are unmodified; pointer is not removed.
template <typename ReturnSource, typename... Args>
struct Canonical<ReturnSource (*)(Args...)> {
  using Type = ReturnSource (*)(Args...);
};

//! Decays function to function pointer.
template <typename ReturnSource, typename... Args>
struct Canonical<ReturnSource(Args...)> : Canonical<ReturnSource (*)(Args...)> {
};

//! Removes unsized array.
template <typename Source>
struct Canonical<Source[]> : Canonical<Source> {};

//! Removes const unsized array.
//
// This specialization is necessary to break a tie between const and unsized
// array.
template <typename Source>
struct Canonical<Source const[]> : Canonical<Source> {};

//! Removes sized array.
template <typename Source, std::size_t size>
struct Canonical<Source[size]> : Canonical<Source> {};

//! Removes const sized array.
//
// This specialization is necessary to break a tie between const and sized
// array.
template <typename Source, std::size_t size>
struct Canonical<Source const[size]> : Canonical<Source> {};

//! Removes reference_wrapper.
template <typename Source>
struct Canonical<std::reference_wrapper<Source>> : Canonical<Source> {};

//! Removes unique_ptr.
template <typename Source, typename Deleter>
struct Canonical<std::unique_ptr<Source, Deleter>> : Canonical<Source> {};

//! Removes shared_ptr.
template <typename Source>
struct Canonical<std::shared_ptr<Source>> : Canonical<Source> {};

//! Removes weak_ptr.
template <typename Source>
struct Canonical<std::weak_ptr<Source>> : Canonical<Source> {};

}  // namespace detail

//! Trait to remove all ref, cv, and pointer qualifiers and standard wrappers.
template <typename Source>
using Canonical = typename detail::Canonical<Source>::Type;

}  // namespace dink
