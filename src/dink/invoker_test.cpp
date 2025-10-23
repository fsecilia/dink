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

  // Constructed and ResolverFactory work together so the factory can just
  // return indices directly instead of indexed resolvers that convert to
  // parameters that happen to be indices. It's all transparent to Invoker,
  // which just replaces a sequence with the results of the ResolverFactory.

  template <typename... Args>
  struct Constructed {
    std::tuple<Args...> args_tuple;
    explicit constexpr Constructed(Args... args) noexcept
        : args_tuple{args...} {}
  };

  struct ResolverFactory {
    template <typename DependencyChain, scope::Lifetime min_lifetime,
              typename Constructed, std::size_t arity, std::size_t index,
              typename Container>
    constexpr auto create(Container& /*container*/) const noexcept
        -> std::size_t {
      return index;
    }
  };
};

// Factory Specialization
// ----------------------------------------------------------------------------

struct InvokerFixtureFactory : InvokerFixture {
  static constexpr auto constructed_factory = [](auto&&... args) noexcept {
    return Constructed{std::forward<decltype(args)>(args)...};
  };

  template <std::size_t... indices>
  using Sut = dink::Invoker<Constructed<decltype(indices)...>,
                            decltype(constructed_factory), ResolverFactory,
                            std::index_sequence<indices...>>;
};

struct InvokerFixtureFactoryCompileTime : InvokerFixtureFactory {
  template <std::size_t... indices>
  static constexpr auto test() -> bool {
    Container container;
    const Sut<indices...> invoker{constructed_factory, ResolverFactory{}};
    const auto result =
        invoker.template create<Constructed<decltype(indices)...>,
                                DependencyChain, min_lifetime>(container);
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
[[maybe_unused]] constexpr auto sequenced_invoker_fixture_Factory_compile_time =
    InvokerFixtureFactoryCompileTime{};

struct InvokerTestFactoryRunTime : InvokerFixtureFactory, Test {
  Container container;
  Sut<0, 1, 2> sut{constructed_factory, ResolverFactory{}};
  using ConstructedType = Constructed<std::size_t, std::size_t, std::size_t>;
};

TEST_F(InvokerTestFactoryRunTime, Arity3SharedPtr) {
  const auto result =
      sut.template create<std::shared_ptr<ConstructedType>, DependencyChain,
                          min_lifetime>(container);
  EXPECT_EQ(result->args_tuple, std::make_tuple(0, 1, 2));
}

TEST_F(InvokerTestFactoryRunTime, Arity3UniquePtr) {
  const auto result =
      sut.template create<std::unique_ptr<ConstructedType>, DependencyChain,
                          min_lifetime>(container);
  EXPECT_EQ(result->args_tuple, std::make_tuple(0, 1, 2));
}

// Ctor Specialization
// ----------------------------------------------------------------------------

struct InvokerFixtureCtor : InvokerFixture {
  template <std::size_t... indices>
  using Sut = dink::Invoker<Constructed<decltype(indices)...>, void,
                            ResolverFactory, std::index_sequence<indices...>>;
};

struct InvokerFixtureCtorCompileTime : InvokerFixtureCtor {
  template <std::size_t... indices>
  static constexpr auto test() -> bool {
    Container container;
    const Sut<indices...> invoker{ResolverFactory{}};
    const auto result =
        invoker.template create<Constructed<decltype(indices)...>,
                                DependencyChain, min_lifetime>(container);
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
  Sut<0, 1, 2> sut{ResolverFactory{}};
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
          typename ResolverFactory, std::size_t... indices>
struct InvokerSpyBase {
  static constexpr std::size_t arity = sizeof...(indices);

  template <typename Container, typename Requested, typename DependencyChain,
            scope::Lifetime min_lifetime>
  auto constexpr create(Container&) const -> Requested {
    return Requested{};
  }
};

template <typename Constructed, typename ConstructedFactory,
          typename ResolverFactory, typename IndexSequence>
struct InvokerSpy;

// factory specialization
template <typename Constructed, typename ConstructedFactory,
          typename ResolverFactory, std::size_t... indices>
struct InvokerSpy<Constructed, ConstructedFactory, ResolverFactory,
                  std::index_sequence<indices...>>
    : InvokerSpyBase<Constructed, ConstructedFactory, ResolverFactory,
                     indices...> {
  ConstructedFactory constructed_factory{-1};

  explicit constexpr InvokerSpy(ConstructedFactory constructed_factory,
                                ResolverFactory) noexcept
      : constructed_factory{std::move(constructed_factory)} {}
};

// ctor specialization
template <typename Constructed, typename ResolverFactory,
          std::size_t... indices>
struct InvokerSpy<Constructed, void, ResolverFactory,
                  std::index_sequence<indices...>>
    : InvokerSpyBase<Constructed, void, ResolverFactory, indices...> {
  explicit constexpr InvokerSpy(ResolverFactory) noexcept {}
};

struct InvokerFactoryFixture {
  struct ResolverFactory {};

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
            typename ResolverFactory, typename IndexSequence>
  using Invoker = InvokerSpy<Constructed, ConstructedFactory, ResolverFactory,
                             IndexSequence>;

  using Sut = InvokerFactory<Invoker>;
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
                                      ResolverFactory>(
                         std::declval<ConstructedFactory>()));
    using Expected = Invoker<Constructed, ConstructedFactory, ResolverFactory,
                             std::make_index_sequence<expected_arity>>;

    static_assert(std::same_as<Actual, Expected>);
    static_assert(Expected::arity == expected_arity);
  }

  // Tests that ctor specialization produces correct type
  template <typename Constructed, std::size_t expected_arity>
  static constexpr auto test_ctor_type() {
    using Actual =
        decltype(std::declval<Sut>()
                     .template create<Constructed, ResolverFactory>());
    using Expected = Invoker<Constructed, void, ResolverFactory,
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

// Run-Time Tests: Factory Instance
// ----------------------------------------------------------------------------

struct InvokerFactoryRunTimeTest : InvokerFactoryFixture, Test {
  Sut sut;

  static constexpr auto unique_id = int_t{42};
};

TEST_F(InvokerFactoryRunTimeTest, FactoryArity0PreservesInstance) {
  ConstructedFactory0 factory;
  factory.id = unique_id;

  auto invoker =
      sut.template create<Constructed0, ConstructedFactory0, ResolverFactory>(
          factory);

  EXPECT_EQ(invoker.constructed_factory.id, unique_id);
}

TEST_F(InvokerFactoryRunTimeTest, FactoryArity1PreservesInstance) {
  ConstructedFactory1 factory;
  factory.id = unique_id;

  auto invoker =
      sut.template create<Constructed1, ConstructedFactory1, ResolverFactory>(
          factory);

  EXPECT_EQ(invoker.constructed_factory.id, unique_id);
}

TEST_F(InvokerFactoryRunTimeTest, FactoryArity3PreservesInstance) {
  ConstructedFactory3 factory;
  factory.id = unique_id;

  auto invoker =
      sut.template create<Constructed3, ConstructedFactory3, ResolverFactory>(
          factory);

  EXPECT_EQ(invoker.constructed_factory.id, unique_id);
}

}  // namespace
}  // namespace dink
