// \file
// Copyright (c) 2025 Frank Secilia
// SPDX-License-Identifier: MIT

#include "config.hpp"
#include <dink/test.hpp>

namespace dink {
namespace detail {
namespace {

// ----------------------------------------------------------------------------
// BindingIndex
// ----------------------------------------------------------------------------

struct BindingIndexTest {
  struct Found {};
  struct NotFound {};

  template <typename From, typename... Types>
  static constexpr auto test_case =
      binding_index<From, decltype(make_bindings(bind<Types>()...))>;

  // 0
  static_assert(npos == test_case<Found>, "should find npos for empty binding");

  // 1
  static_assert(0 == test_case<Found, Found>, "should find single binding");
  static_assert(npos == test_case<Found, NotFound>,
                "should not find single binding");

  // Many, unique found entry.
  static_assert(0 == test_case<Found, Found, NotFound, NotFound>,
                "should find matching binding at front");
  static_assert(1 == test_case<Found, NotFound, Found, NotFound>,
                "should find matching binding in middle");
  static_assert(2 == test_case<Found, NotFound, NotFound, Found>,
                "should find matching binding at back");
  static_assert(npos == test_case<Found, NotFound, NotFound, NotFound>,
                "should not find matching binding");

  // Many, multiple found entires.
  static_assert(0 == test_case<Found, Found, Found, Found>,
                "should find first matching binding at front");
  static_assert(1 == test_case<Found, NotFound, Found, Found>,
                "should find first matching binding in middle");
};

}  // namespace
}  // namespace detail
}  // namespace dink
