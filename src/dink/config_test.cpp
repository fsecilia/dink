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

namespace {

// ----------------------------------------------------------------------------
// IsConfig
// ----------------------------------------------------------------------------

static_assert(IsConfig<Config<>>, "should match a real, empty Config");
static_assert(
    IsConfig<Config<Binding<int_t, scope::Transient, provider::Ctor<int_t>>>>,
    "should match a Config");
static_assert(!IsConfig<int_t>, "should not match an int");
static_assert(!IsConfig<std::tuple<>>, "should not match a tuple");

// ----------------------------------------------------------------------------
// Config
// ----------------------------------------------------------------------------

struct ConfigTest {
  using Binding0 = Binding<int_t, scope::Transient, provider::Ctor<int_t>>;
  using Binding1 = Binding<uint_t, scope::Transient, provider::Ctor<uint_t>>;
  using Binding2 = Binding<char, scope::Transient, provider::Ctor<char>>;

  static auto constexpr binding0 = Binding0{};
  static auto constexpr binding1 = Binding1{};
  static auto constexpr binding2 = Binding2{};

  // Empty ctors.
  static_assert(std::same_as<Config<>, decltype(Config{})>,
                "empty args should produce empty Config");
  static_assert(std::same_as<Config<>, decltype(Config{std::tuple{}})>,
                "empty tuple should produce empty Config");

  // Single-element ctors.
  static_assert(std::same_as<Config<Binding0>, decltype(Config{binding0})>,
                "single-element tuple should produce single-element Config");
  static_assert(
      std::same_as<Config<Binding0>, decltype(Config{std::tuple{binding0}})>,
      "single-element args should produce single-element Config");

  // Multiple-element ctors.
  static_assert(
      std::same_as<Config<Binding0, Binding1, Binding2>,
                   decltype(Config{binding0, binding1, binding2})>,
      "multiple-element tuple should produce multiple-element Config");
  static_assert(
      std::same_as<Config<Binding0, Binding1, Binding2>,
                   decltype(Config{std::tuple{binding0, binding1, binding2}})>,
      "multiple-element args should produce multiple-element Config");

  [[maybe_unused]] static inline auto sut =
      Config{binding0, binding1, binding2};

  // find_binding.
  static_assert(
      std::same_as<Binding0*,
                   decltype(sut.template find_binding<Binding0::FromType>())>,
      "should find binding0");
  static_assert(
      std::same_as<Binding1*,
                   decltype(sut.template find_binding<Binding1::FromType>())>,
      "should find binding1");
  static_assert(
      std::same_as<Binding2*,
                   decltype(sut.template find_binding<Binding2::FromType>())>,
      "should find binding2");
  static_assert(
      std::same_as<std::nullptr_t, decltype(sut.template find_binding<void>())>,
      "should not find binding");
  static_assert(std::same_as<std::nullptr_t,
                             decltype(sut.template find_binding<void*>())>,
                "should not find binding");
};

}  // namespace
}  // namespace dink
