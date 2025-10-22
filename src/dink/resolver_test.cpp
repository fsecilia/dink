// \file
// Copyright (c) 2025 Frank Secilia
// SPDX-License-Identifier: MIT

#include "resolver.hpp"
#include <dink/test.hpp>
#include <concepts>

namespace dink {
namespace {

// ----------------------------------------------------------------------------
// Resolver
// ----------------------------------------------------------------------------

// Tests that Resolver returns real instances from the container.
struct ResolverReturnsRealInstancesFromContainerTest : Test {
  struct Deduced {
    using id_t = int_t;
    static constexpr auto default_id = id_t{3};
    static constexpr auto expected_id = id_t{5};
    id_t id = default_id;
  };

  struct Container {
    const Deduced real_instance = Deduced{Deduced::expected_id};

    template <typename, typename, scope::Lifetime>
    auto resolve() -> Deduced {
      return real_instance;
    }
  };

  struct Handler {
    auto handle(Deduced deduced) const -> void {
      ASSERT_EQ(Deduced::expected_id, deduced.id);
    }
  };

  template <typename ActualDeduced>
  auto test() -> void {
    auto container = Container{};

    Handler{}.handle(
        Resolver<Container, TypeList<>, scope::Lifetime::kDefault>{container});
  }
};

TEST_F(ResolverReturnsRealInstancesFromContainerTest, result) {
  test<Deduced>();
}

// Tests that resolvers convert to the expected type for types we expect.
struct ResolverDeducesTypeTest {
  struct Container;

  struct Deduced {};

  // Tag type to capture passed argument type verbatim, without any decay.
  template <typename ExpectedArg>
  struct Result;

  // Handler whose argument can be specified directly and is captured in a tag.
  struct Handler {
    template <typename ExpectedArg>
    constexpr auto handle(ExpectedArg expected_arg) const
        -> Result<ExpectedArg>;
  };
  static constexpr Handler handler{};

  // This test creates a handler taking literally an ExpectedArg parameter,
  // then calls it, passing the Sut. It tests that the tagged result type
  // matches. This executes the conversion at compile time. If it compiles,
  // the conversion works.
  template <typename Sut, typename ExpectedArg>
  constexpr auto test_deduction() -> void {
    static_assert(std::same_as<decltype(handler.template handle<ExpectedArg>(
                                   std::declval<Sut>())),
                               Result<ExpectedArg>>);
  }

  // Tests combinations of value type with a few modifiers.
  template <typename Sut>
  constexpr auto test_all_deductions() -> void {
    test_deduction<Sut, Deduced>();
    test_deduction<Sut, Deduced&>();
    test_deduction<Sut, Deduced&&>();
    test_deduction<Sut, const Deduced&>();
    test_deduction<Sut, const Deduced&&>();
    test_deduction<Sut, Deduced*>();
    test_deduction<Sut, const Deduced*>();
    test_deduction<Sut, Deduced*&>();
    test_deduction<Sut, const Deduced*&>();
    test_deduction<Sut, Deduced*&&>();
    test_deduction<Sut, const Deduced*&&>();
    test_deduction<Sut, Deduced* const&>();
    test_deduction<Sut, const Deduced* const&>();
    test_deduction<Sut, Deduced* const&&>();
    test_deduction<Sut, const Deduced* const&&>();

    constexpr auto deduced_array_size = int{4};
    test_deduction<Sut, Deduced(&)[deduced_array_size]>();
  }

  constexpr ResolverDeducesTypeTest() {
    using Resolver = Resolver<Container, TypeList<>, scope::Lifetime::kDefault>;
    test_all_deductions<Resolver>();
  }
};
[[maybe_unused]] constexpr auto resolver_deduces_type_test =
    ResolverDeducesTypeTest{};

// Tests that Resolver updates the dependency chain correctly.
struct ResolverAppendsCanonicalDeducedToDependencyChainTest {
  struct Deduced {};

  using InitialDependencyChain = TypeList<int, void*>;
  using ExpectedDependencyChain = TypeList<int, void*, Deduced>;

  struct Container {
    template <typename ActualDeduced, typename NextDependencyChain,
              scope::Lifetime min_lifetime>
    constexpr auto resolve() -> ActualDeduced {
      static_assert(std::same_as<ExpectedDependencyChain, NextDependencyChain>);
      return ActualDeduced{};
    }
  };

  template <typename ActualDeduced>
  struct Handler {
    constexpr auto handle(ActualDeduced) const -> void {}
  };

  template <typename ActualDeduced>
  constexpr auto test() -> void {
    auto container = Container{};

    Handler<ActualDeduced>{}.handle(
        Resolver<Container, InitialDependencyChain, scope::Lifetime::kDefault>{
            container});

    Handler<ActualDeduced*>{}.handle(
        Resolver<Container, InitialDependencyChain, scope::Lifetime::kDefault>{
            container});

    Handler<ActualDeduced&&>{}.handle(
        Resolver<Container, InitialDependencyChain, scope::Lifetime::kDefault>{
            container});

    Handler<std::unique_ptr<ActualDeduced>>{}.handle(
        Resolver<Container, InitialDependencyChain, scope::Lifetime::kDefault>{
            container});
  }

  constexpr ResolverAppendsCanonicalDeducedToDependencyChainTest() {
    test<Deduced>();
  }
};
[[maybe_unused]] constexpr auto
    resolver_appends_canonical_deduced_to_dependency_chain =
        ResolverAppendsCanonicalDeducedToDependencyChainTest{};

// Tests that Resolver forwards min_lifetime.
struct ResolverForwardsMinLifetimeTest {
  static constexpr auto expected_min_lifetime = scope::Lifetime::kSingleton;

  struct Deduced {};

  struct Container {
    template <typename ActualDeduced, typename NextDependencyChain,
              scope::Lifetime min_lifetime>
    constexpr auto resolve() -> ActualDeduced {
      static_assert(min_lifetime == expected_min_lifetime);
      return ActualDeduced{};
    }
  };

  template <typename ActualDeduced>
  struct Handler {
    constexpr auto handle(ActualDeduced) const -> void {}
  };

  template <typename ActualDeduced>
  constexpr auto test() -> void {
    auto container = Container{};

    Handler<ActualDeduced>{}.handle(
        Resolver<Container, TypeList<>, expected_min_lifetime>{container});
  }

  constexpr ResolverForwardsMinLifetimeTest() { test<Deduced>(); }
};
[[maybe_unused]] constexpr auto resolver_forwards_min_lifetime =
    ResolverForwardsMinLifetimeTest{};

}  // namespace
}  // namespace dink
