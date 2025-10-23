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

struct InvokerFactoryTest {
  struct Dep {};

  struct Arity0Constructed {
    Arity0Constructed() = default;
  };

  struct Arity1Constructed {
    explicit Arity1Constructed(Dep) {}
  };

  struct Arity3Constructed {
    Arity3Constructed(Dep, Dep, Dep) {}
  };

  struct Factory3 {
    auto operator()(Dep, Dep, Dep) const -> Arity3Constructed {
      return Arity3Constructed{Dep{}, Dep{}, Dep{}};
    }
  };

  struct Container {};
  using DependencyChain = TypeList<>;
  static constexpr auto min_lifetime = scope::Lifetime::kDefault;

  using Sut = InvokerFactory;

  // Test that factory creates correct invoker type for ctor specialization.
  template <typename Constructed, std::size_t expected_arity>
  constexpr auto test_ctor_creates_correct_type() -> void {
    Sut factory;
    auto invoker = factory.template create<Container, DependencyChain,
                                           min_lifetime, Constructed>();

    // Verify it's a Invoker with correct template params
    using ActualInvoker = decltype(invoker);
    using ExpectedInvoker =
        CtorInvoker<Container, DependencyChain, min_lifetime, Constructed>;

    static_assert(std::same_as<ActualInvoker, ExpectedInvoker>);
  }

  // Test that factory creates correct invoker type for factory specialization.
  template <typename Constructed, typename ConstructedFactory,
            std::size_t expected_arity>
  constexpr auto test_factory_creates_correct_type() -> void {
    Sut factory;
    auto invoker =
        factory.template create<Container, DependencyChain, min_lifetime,
                                Constructed>(ConstructedFactory{});

    // Verify it's a Invoker with correct template params
    using ActualInvoker = decltype(invoker);
    using ExpectedInvoker =
        FactoryInvoker<Container, DependencyChain, min_lifetime, Constructed,
                       ConstructedFactory>;

    static_assert(std::same_as<ActualInvoker, ExpectedInvoker>);
  }

  constexpr InvokerFactoryTest() {
    // Ctor specialization
    test_ctor_creates_correct_type<Arity0Constructed, 0>();
    test_ctor_creates_correct_type<Arity1Constructed, 1>();
    test_ctor_creates_correct_type<Arity3Constructed, 3>();

    // Factory specialization
    test_factory_creates_correct_type<Arity3Constructed, Factory3, 3>();
  }
};
[[maybe_unused]] constexpr auto sequenced_resolver_invoker_factory_test =
    InvokerFactoryTest{};

}  // namespace
}  // namespace dink
