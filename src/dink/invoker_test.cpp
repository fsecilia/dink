// \file
// Copyright (c) 2025 Frank Secilia
// SPDX-License-Identifier: MIT

#include "invoker.hpp"
#include <dink/test.hpp>
#include <concepts>
#include <tuple>

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

// ----------------------------------------------------------------------------
// SequencedInvoker
// ----------------------------------------------------------------------------

struct SequencedInvokerFixture {
  struct Container {};

  // IndexedFactory and Constructed work together so the factory can just
  // return indices directly instead of indexed resolvers that convert to ctor
  // paramters that happen to be indices. It's all transparent to
  // SequencedInvoker, which just replaces a sequence with the results of the
  // IndexedFactory.

  struct IndexedFactory {
    template <std::size_t arity, std::size_t index>
    constexpr auto create(auto& /*container*/) const noexcept -> std::size_t {
      return index;
    }
  };

  template <typename... Args>
  struct Constructed {
    std::tuple<Args...> args_tuple;
    explicit constexpr Constructed(Args... args) noexcept
        : args_tuple{args...} {}
  };

  static constexpr auto instance_factory = [](auto&&... args) noexcept {
    return Constructed{std::forward<decltype(args)>(args)...};
  };

  template <std::size_t... indices>
  using Sut =
      dink::SequencedInvoker<Constructed<decltype(indices)...>, IndexedFactory,
                             std::index_sequence<indices...>>;
};

struct SequencedInvokerTestCompileTime : SequencedInvokerFixture {
  template <std::size_t... indices>
  static constexpr auto test(auto&& invoke) -> bool {
    Container container;
    Sut<indices...> invoker{IndexedFactory{}};
    auto res = std::forward<decltype(invoke)>(invoke)(container, invoker);
    return res.args_tuple == std::make_tuple(indices...);
  }

  template <std::size_t... indices>
  static constexpr auto test_ctor() -> bool {
    return test<indices...>([&](auto& container, auto& invoker) constexpr {
      return invoker.create_value(container);
    });
  }

  template <std::size_t... indices>
  static constexpr auto test_factory() -> bool {
    return test<indices...>([&](auto& container, auto& invoker) constexpr {
      return invoker.create_value(container, instance_factory);
    });
  }

  static constexpr auto test_arity_0_ctor() -> bool { return test_ctor<>(); }
  static constexpr auto test_arity_1_ctor() -> bool { return test_ctor<0>(); }
  static constexpr auto test_arity_3_ctor() -> bool {
    return test_ctor<0, 1, 2>();
  }

  static constexpr auto test_arity_0_factory() -> bool {
    return test_factory<>();
  }

  static constexpr auto test_arity_1_factory() -> bool {
    return test_factory<0>();
  }

  static constexpr auto test_arity_3_factory() -> bool {
    return test_factory<0, 1, 2>();
  }

  static constexpr auto instantiate_constexpr_tests() -> void {
    static_assert(test_arity_0_ctor(), "Arity 0 (Ctor) Failed");
    static_assert(test_arity_1_ctor(), "Arity 1 (Ctor) Failed");
    static_assert(test_arity_3_ctor(), "Arity 3 (Ctor) Failed");

    static_assert(test_arity_0_factory(), "Arity 0 (Factory) Failed");
    static_assert(test_arity_1_factory(), "Arity 1 (Factory) Failed");
    static_assert(test_arity_3_factory(), "Arity 3 (Factory) Failed");
  }

  constexpr SequencedInvokerTestCompileTime() noexcept {
    instantiate_constexpr_tests();
  }
};
[[maybe_unused]] constexpr auto sequenced_invoker_test_constexpr =
    SequencedInvokerTestCompileTime{};

// ----------------------------------------------------------------------------

struct SequencedInvokerTestRunTime : SequencedInvokerFixture, Test {
  Container container;
  Sut<0, 1, 2> sut{IndexedFactory{}};
};

TEST_F(SequencedInvokerTestRunTime, Arity3SharedPtrCtor) {
  auto res = sut.create_shared(container);
  EXPECT_EQ(res->args_tuple, std::make_tuple(0, 1, 2));
}

TEST_F(SequencedInvokerTestRunTime, Arity3UniquePtrFactory) {
  auto res = sut.create_unique(container, instance_factory);
  EXPECT_EQ(res->args_tuple, std::make_tuple(0, 1, 2));
}

}  // namespace
}  // namespace dink
