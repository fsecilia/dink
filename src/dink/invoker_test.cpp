// \file
// Copyright (c) 2025 Frank Secilia
// SPDX-License-Identifier: MIT

#include "invoker.hpp"
#include <dink/test.hpp>
#include <concepts>
#include <utility>

namespace dink {
namespace {

// ----------------------------------------------------------------------------
// IndexedResolverFactory
// ----------------------------------------------------------------------------

struct IndexedResolverFactoryTest {
  struct Container {};

  struct Resolver {
    Container& container;
  };

  struct SingleArgResolver {
    Resolver resolver;
  };

  using Sut = IndexedResolverFactory<Resolver, SingleArgResolver>;

  template <typename Expected, std::size_t arity, std::size_t index>
  constexpr auto test_single() {
    using Actual = decltype(std::declval<Sut>().template create<arity, index>(
        std::declval<Container&>()));

    static_assert(std::same_as<Expected, Actual>);
  }

  constexpr auto test_multiple() {
    test_single<Resolver, 0, 0>();
    test_single<Resolver, 0, 1>();
    test_single<Resolver, 0, 2>();

    test_single<SingleArgResolver, 1, 0>();
    test_single<SingleArgResolver, 1, 1>();
    test_single<SingleArgResolver, 1, 2>();

    test_single<Resolver, 2, 0>();
    test_single<Resolver, 2, 1>();
    test_single<Resolver, 2, 2>();
  }

  constexpr IndexedResolverFactoryTest() { test_multiple(); }
};
[[maybe_unused]] constexpr auto indexed_resolver_factory_test =
    IndexedResolverFactoryTest{};

}  // namespace
}  // namespace dink
