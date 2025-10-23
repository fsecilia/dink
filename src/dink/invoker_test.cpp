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
    template <typename Container, typename DependencyChain,
              scope::Lifetime min_lifetime, typename Constructed,
              std::size_t arity, std::size_t index>
    constexpr auto create(auto& /*container*/) const noexcept -> std::size_t {
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
    const auto res =
        invoker.template create<Constructed<decltype(indices)...>,
                                DependencyChain, min_lifetime>(container);
    return res.args_tuple == std::make_tuple(indices...);
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
  const auto res =
      sut.template create<std::shared_ptr<ConstructedType>, DependencyChain,
                          min_lifetime>(container);
  EXPECT_EQ(res->args_tuple, std::make_tuple(0, 1, 2));
}

TEST_F(InvokerTestFactoryRunTime, Arity3UniquePtr) {
  const auto res =
      sut.template create<std::unique_ptr<ConstructedType>, DependencyChain,
                          min_lifetime>(container);
  EXPECT_EQ(res->args_tuple, std::make_tuple(0, 1, 2));
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
    const auto res =
        invoker.template create<Constructed<decltype(indices)...>,
                                DependencyChain, min_lifetime>(container);
    return res.args_tuple == std::make_tuple(indices...);
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
  auto res = sut.template create<std::shared_ptr<ConstructedType>,
                                 DependencyChain, min_lifetime>(container);
  EXPECT_EQ(res->args_tuple, std::make_tuple(0, 1, 2));
}

TEST_F(InvokerTestCtorRunTime, Arity3UniquePtr) {
  auto res = sut.template create<std::unique_ptr<ConstructedType>,
                                 DependencyChain, min_lifetime>(container);
  EXPECT_EQ(res->args_tuple, std::make_tuple(0, 1, 2));
}

// ----------------------------------------------------------------------------
// InvokerFactory
// ----------------------------------------------------------------------------

struct InvokerFactoryTestCompileTime {
  struct ResolverFactory {};

  struct Constructed0 {
    Constructed0();
  };
  struct Constructed1 {
    Constructed1(int_t);
  };
  struct Constructed3 {
    Constructed3(int_t, int_t, int_t);
  };

  struct ConstructedFactory0 {
    constexpr auto operator()() const noexcept -> Constructed0;
  };

  struct ConstructedFactory1 {
    constexpr auto operator()(int_t) const noexcept -> Constructed1;
  };

  struct ConstructedFactory3 {
    constexpr auto operator()(int_t, int_t, int_t) const noexcept
        -> Constructed3;
  };

  template <typename Constructed, typename ConstructedFactory,
            typename ResolverFactory, typename>
  struct Invoker {};

  using Sut = InvokerFactory<Invoker>;

  template <typename Constructed, typename ConstructedFactory,
            std::size_t arity>
  static constexpr auto factory_specialization_result_type_matches() -> bool {
    using Actual =
        decltype(std::declval<Sut>()
                     .template create<Constructed, ConstructedFactory,
                                      ResolverFactory>(
                         std::declval<ConstructedFactory>()));
    using Expected = Invoker<Constructed, ConstructedFactory, ResolverFactory,
                             std::make_index_sequence<arity>>;
    return std::same_as<Actual, Expected>;
  }

  template <typename Constructed, std::size_t arity>
  static constexpr auto ctor_specialization_result_type_matches() -> bool {
    using Actual =
        decltype(std::declval<Sut>()
                     .template create<Constructed, ResolverFactory>());
    using Expected = Invoker<Constructed, void, ResolverFactory,
                             std::make_index_sequence<arity>>;
    return std::same_as<Actual, Expected>;
  }

  constexpr InvokerFactoryTestCompileTime() {
    static_assert(
        factory_specialization_result_type_matches<Constructed0,
                                                   ConstructedFactory0, 0>());
    static_assert(
        factory_specialization_result_type_matches<Constructed1,
                                                   ConstructedFactory1, 1>());
    static_assert(
        factory_specialization_result_type_matches<Constructed3,
                                                   ConstructedFactory3, 3>());

    static_assert(ctor_specialization_result_type_matches<Constructed0, 0>());
    static_assert(ctor_specialization_result_type_matches<Constructed1, 1>());
    static_assert(ctor_specialization_result_type_matches<Constructed3, 3>());
  }
};
[[maybe_unused]] constexpr auto invoker_factory_test_compile_time =
    InvokerFactoryTestCompileTime{};

}  // namespace
}  // namespace dink
