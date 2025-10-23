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
// Invoker
// ----------------------------------------------------------------------------

struct InvokerFixture {
  struct Container {};
  using DependencyChain = TypeList<>;
  static constexpr scope::Lifetime min_lifetime = scope::Lifetime::kDefault;

  // Constructed and ResolverSequence work together so the factory can just
  // return indices directly instead of indexed resolvers that convert to
  // parameters that happen to be indices. It's all transparent to Invoker,
  // which just replaces a sequence with the results of the ResolverSequence.

  template <typename... Args>
  struct Constructed {
    std::tuple<Args...> args_tuple;
    explicit constexpr Constructed(Args... args) noexcept
        : args_tuple{args...} {}
  };

  struct ResolverSequence {
    template <typename DependencyChain, scope::Lifetime min_lifetime,
              typename Constructed, std::size_t arity, std::size_t index,
              typename Container>
    constexpr auto create_element(Container& /*container*/) const noexcept
        -> std::size_t {
      return index;
    }
  };
};

// Factory Specialization
// ----------------------------------------------------------------------------

struct InvokerFixtureFactoryCompileTime : InvokerFixture {
  static constexpr auto constructed_factory = [](auto&&... args) noexcept {
    return Constructed{std::forward<decltype(args)>(args)...};
  };

  template <std::size_t... indices>
  using Sut = dink::Invoker<Constructed<decltype(indices)...>,
                            decltype(constructed_factory), ResolverSequence,
                            std::index_sequence<indices...>>;

  template <std::size_t... indices>
  static constexpr auto test() -> bool {
    Container container;
    const auto sut = Sut<indices...>{ResolverSequence{}};
    const auto result =
        sut.template create<Constructed<decltype(indices)...>, DependencyChain,
                            min_lifetime>(container, constructed_factory);
    return result.args_tuple == std::make_tuple(indices...);
  }

  static constexpr auto test_arity_0() -> bool { return test<>(); }
  static constexpr auto test_arity_1() -> bool { return test<0>(); }
  static constexpr auto test_arity_3() -> bool { return test<0, 1, 2>(); }

  constexpr InvokerFixtureFactoryCompileTime() noexcept {
    static_assert(test_arity_0(), "Arity 0 (Factory) Failed");
    static_assert(test_arity_1(), "Arity 1 (Factory) Failed");
    static_assert(test_arity_3(), "Arity 3 (Factory) Failed");
  }
};
[[maybe_unused]] constexpr auto invoker_fixture_factory_compile_time =
    InvokerFixtureFactoryCompileTime{};

struct InvokerTestFactoryRunTime : InvokerFixture, Test {
  using Constructed = Constructed<std::size_t, std::size_t, std::size_t>;

  struct Factory {
    mutable std::vector<std::size_t> call_args;

    auto operator()(auto... args) const -> Constructed {
      (call_args.push_back(args), ...);  // Record each arg
      return Constructed{args...};
    }
  };
  Factory constructed_factory;

  template <std::size_t... indices>
  using Sut = dink::Invoker<Constructed, Factory, ResolverSequence,
                            std::index_sequence<indices...>>;

  Container container;
  Sut<0, 1, 2> sut{ResolverSequence{}};

  auto test_result(const Constructed& result) -> void {
    EXPECT_EQ(result.args_tuple, std::make_tuple(0, 1, 2));
    const auto expected_call_args = std::vector<std::size_t>{0, 1, 2};
    EXPECT_EQ(expected_call_args, constructed_factory.call_args);
  }
};

TEST_F(InvokerTestFactoryRunTime, Arity3Value) {
  test_result(sut.template create<Constructed, DependencyChain, min_lifetime>(
      container, constructed_factory));
}

TEST_F(InvokerTestFactoryRunTime, Arity3SharedPtr) {
  test_result(
      *sut.template create<std::shared_ptr<Constructed>, DependencyChain,
                           min_lifetime>(container, constructed_factory));
}

TEST_F(InvokerTestFactoryRunTime, Arity3UniquePtr) {
  test_result(
      *sut.template create<std::unique_ptr<Constructed>, DependencyChain,
                           min_lifetime>(container, constructed_factory));
}

// Ctor Specialization
// ----------------------------------------------------------------------------

struct InvokerFixtureCtor : InvokerFixture {
  template <std::size_t... indices>
  using Sut = dink::Invoker<Constructed<decltype(indices)...>, void,
                            ResolverSequence, std::index_sequence<indices...>>;
};

struct InvokerFixtureCtorCompileTime : InvokerFixtureCtor {
  template <std::size_t... indices>
  static constexpr auto test() -> bool {
    Container container;
    const auto sut = Sut<indices...>{ResolverSequence{}};
    const auto result =
        sut.template create<Constructed<decltype(indices)...>, DependencyChain,
                            min_lifetime>(container);
    return result.args_tuple == std::make_tuple(indices...);
  }

  static constexpr auto test_arity_0() -> bool { return test<>(); }
  static constexpr auto test_arity_1() -> bool { return test<0>(); }
  static constexpr auto test_arity_3() -> bool { return test<0, 1, 2>(); }

  constexpr InvokerFixtureCtorCompileTime() noexcept {
    static_assert(test_arity_0(), "Arity 0 (Ctor) Failed");
    static_assert(test_arity_1(), "Arity 1 (Ctor) Failed");
    static_assert(test_arity_3(), "Arity 3 (Ctor) Failed");
  }
};
[[maybe_unused]] constexpr auto sequenced_invoker_fixture_ctor_compile_time =
    InvokerFixtureCtorCompileTime{};

struct InvokerTestCtorRunTime : InvokerFixtureCtor, Test {
  Container container;
  Sut<0, 1, 2> sut{ResolverSequence{}};
  using ConstructedType = Constructed<std::size_t, std::size_t, std::size_t>;
};

TEST_F(InvokerTestCtorRunTime, Arity3SharedPtr) {
  auto result = sut.template create<std::shared_ptr<ConstructedType>,
                                    DependencyChain, min_lifetime>(container);
  EXPECT_EQ(result->args_tuple, std::make_tuple(0, 1, 2));
}

TEST_F(InvokerTestCtorRunTime, Arity3UniquePtr) {
  auto result = sut.template create<std::unique_ptr<ConstructedType>,
                                    DependencyChain, min_lifetime>(container);
  EXPECT_EQ(result->args_tuple, std::make_tuple(0, 1, 2));
}

// ----------------------------------------------------------------------------
// InvokerFactory
// ----------------------------------------------------------------------------

template <typename Constructed, typename ConstructedFactory,
          typename ResolverSequence, std::size_t... indices>
struct SpyInvokerBase {
  static constexpr std::size_t arity = sizeof...(indices);
  explicit constexpr SpyInvokerBase(ResolverSequence) noexcept {}
};

template <typename Constructed, typename ConstructedFactory,
          typename ResolverSequence, typename IndexSequence>
struct SpyInvoker;

// factory specialization
template <typename Constructed, typename ConstructedFactory,
          typename ResolverSequence, std::size_t... indices>
struct SpyInvoker<Constructed, ConstructedFactory, ResolverSequence,
                  std::index_sequence<indices...>>
    : SpyInvokerBase<Constructed, ConstructedFactory, ResolverSequence,
                     indices...> {
  using Base = SpyInvokerBase<Constructed, ConstructedFactory, ResolverSequence,
                              indices...>;

  template <typename Container, typename Requested, typename DependencyChain,
            scope::Lifetime min_lifetime>
  auto constexpr create(Container&, ConstructedFactory&) const -> Requested {
    return Requested{};
  }

  template <typename Container, typename Requested, typename DependencyChain,
            scope::Lifetime min_lifetime>
  auto constexpr create(Container&) const -> Requested {
    return Requested{};
  }

  using Base::Base;
};

// ctor specialization
template <typename Constructed, typename ResolverSequence,
          std::size_t... indices>
struct SpyInvoker<Constructed, void, ResolverSequence,
                  std::index_sequence<indices...>>
    : SpyInvokerBase<Constructed, void, ResolverSequence, indices...> {
  using Base = SpyInvokerBase<Constructed, void, ResolverSequence, indices...>;

  using Base::Base;
};

struct InvokerFactoryFixture {
  struct ResolverSequence {};

  struct Constructed0 {
    Constructed0() = default;
  };
  struct Constructed1 {
    Constructed1(int_t) {}
  };
  struct Constructed3 {
    Constructed3(int_t, int_t, int_t) {}
  };

  struct ConstructedFactory0 {
    int_t id = 0;
    constexpr auto operator()() const noexcept -> Constructed0 { return {}; }
  };

  struct ConstructedFactory1 {
    int_t id = 1;
    constexpr auto operator()(int_t) const noexcept -> Constructed1 {
      return {0};
    }
  };

  struct ConstructedFactory3 {
    int_t id = 3;
    constexpr auto operator()(int_t, int_t, int_t) const noexcept
        -> Constructed3 {
      return {0, 0, 0};
    }
  };

  template <typename Constructed, typename ConstructedFactory,
            typename ResolverSequence, typename IndexSequence>
  using Invoker = SpyInvoker<Constructed, ConstructedFactory, ResolverSequence,
                             IndexSequence>;

  using Sut =
      InvokerFactory<Invoker, ResolverSequenceFactory<ResolverSequence>>;
};

// Compile-Time Tests: Type Correctness and Arity
// ----------------------------------------------------------------------------

struct InvokerFactoryCompileTimeTest : InvokerFactoryFixture {
  // Tests that factory specialization produces correct type
  template <typename Constructed, typename ConstructedFactory,
            std::size_t expected_arity>
  static constexpr auto test_factory_type() {
    using Actual =
        decltype(std::declval<Sut>()
                     .template create<Constructed, ConstructedFactory,
                                      ResolverSequence>());
    using Expected = Invoker<Constructed, ConstructedFactory, ResolverSequence,
                             std::make_index_sequence<expected_arity>>;

    static_assert(std::same_as<Actual, Expected>);
    static_assert(Expected::arity == expected_arity);
  }

  // Tests that ctor specialization produces correct type
  template <typename Constructed, std::size_t expected_arity>
  static constexpr auto test_ctor_type() {
    using Actual =
        decltype(std::declval<Sut>()
                     .template create<Constructed, void, ResolverSequence>());
    using Expected = Invoker<Constructed, void, ResolverSequence,
                             std::make_index_sequence<expected_arity>>;

    static_assert(std::same_as<Actual, Expected>);
    static_assert(Expected::arity == expected_arity);
  }

  constexpr InvokerFactoryCompileTimeTest() {
    // Factory specializations
    test_factory_type<Constructed0, ConstructedFactory0, 0>();
    test_factory_type<Constructed1, ConstructedFactory1, 1>();
    test_factory_type<Constructed3, ConstructedFactory3, 3>();

    // Ctor specializations
    test_ctor_type<Constructed0, 0>();
    test_ctor_type<Constructed1, 1>();
    test_ctor_type<Constructed3, 3>();
  }
};
[[maybe_unused]] constexpr auto invoker_factory_compile_time_test =
    InvokerFactoryCompileTimeTest{};

}  // namespace
}  // namespace dink
