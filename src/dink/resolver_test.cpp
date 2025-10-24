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

    template <typename>
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

    Handler{}.handle(Resolver<Container>{container});
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
    using Resolver = Resolver<Container>;
    test_all_deductions<Resolver>();
    test_all_deductions<SingleArgResolver<Handler, Resolver>>();
  }
};
[[maybe_unused]] constexpr auto resolver_deduces_type_test =
    ResolverDeducesTypeTest{};

// ----------------------------------------------------------------------------
// SingleArgResolver
// ----------------------------------------------------------------------------

// Tests that SingleArgResolver filters copy or move ctors.
struct SingleArgResolverDoesNotMatchCopyOrMoveCtorsTest {
  struct CopyMoveOnly {
    CopyMoveOnly(const CopyMoveOnly&) = default;
    CopyMoveOnly(CopyMoveOnly&&) = default;
  };

  struct Arg {};
  struct SingleArgConstructible {
    SingleArgConstructible(Arg);
    SingleArgConstructible(const SingleArgConstructible&) = default;
    SingleArgConstructible(SingleArgConstructible&&) = default;
  };

  struct Resolver {
    operator CopyMoveOnly() const;
    operator SingleArgConstructible() const;
    operator Arg() const;
  };

  // Test that CopyMoveOnly is not constructible at all.
  static_assert(!std::constructible_from<
                CopyMoveOnly, SingleArgResolver<CopyMoveOnly, Resolver>>);

  // Test that SingleArgConstructible is constructible through its Arg ctor.
  static_assert(std::constructible_from<
                SingleArgConstructible,
                SingleArgResolver<SingleArgConstructible, Resolver>>);
};

// ----------------------------------------------------------------------------
// ResolverSequence
// ----------------------------------------------------------------------------

// Resolver spy that tracks the container it was given
template <typename Container>
struct ResolverSpy {
  Container* container_ptr = nullptr;

  template <typename Deduced>
  constexpr operator Deduced() {
    return Deduced{};
  }

  template <typename Deduced>
  constexpr operator Deduced&() const {
    return *static_cast<Deduced*>(nullptr);
  }

  explicit constexpr ResolverSpy(Container& container) noexcept
      : container_ptr{&container} {}
};

// SingleArgResolver spy that wraps a ResolverSpy
template <typename Constructed, typename Resolver>
struct SingleArgResolverSpy {
  Resolver resolver;

  template <meta::DifferentUnqualifiedType<Constructed> Deduced>
  constexpr operator Deduced() {
    return resolver.operator Deduced();
  }

  template <meta::DifferentUnqualifiedType<Constructed> Deduced>
  constexpr operator Deduced&() const {
    return resolver.operator Deduced&();
  }

  explicit constexpr SingleArgResolverSpy(Resolver resolver) noexcept
      : resolver{std::move(resolver)} {}
};

// Shared fixture
struct ResolverSequenceFixture {
  struct Container {
    int_t id = 0;
  };

  struct Constructed {};

  template <typename Container>
  using Resolver = ResolverSpy<Container>;

  template <typename Constructed, typename Resolver>
  using SingleArgResolver = SingleArgResolverSpy<Constructed, Resolver>;

  using Sut = ResolverSequence<Resolver, SingleArgResolver>;
};

// ----------------------------------------------------------------------------
// Compile-Time Tests: Type Correctness and Arity Branching
// ----------------------------------------------------------------------------

struct ResolverSequenceCompileTimeTest : ResolverSequenceFixture {
  // Test that arity != 1 produces Resolver
  template <std::size_t arity, std::size_t index>
  static constexpr auto test_resolver_type() {
    using Actual =
        decltype(std::declval<Sut>()
                     .template create_element<Constructed, arity, index>(
                         std::declval<Container&>()));
    using Expected = Resolver<Container>;

    static_assert(std::same_as<Actual, Expected>);
  }

  // Test that arity == 1 produces SingleArgResolver
  template <std::size_t index>
  static constexpr auto test_single_arg_resolver_type() {
    using Actual = decltype(std::declval<Sut>()
                                .template create_element<Constructed, 1, index>(
                                    std::declval<Container&>()));
    using ResolverType = Resolver<Container>;
    using Expected = SingleArgResolver<Constructed, ResolverType>;

    static_assert(std::same_as<Actual, Expected>);
  }

  constexpr ResolverSequenceCompileTimeTest() {
    // Test arity 0 produces Resolver
    test_resolver_type<0, 0>();

    // Test arity 1 produces SingleArgResolver (the special case)
    test_single_arg_resolver_type<0>();

    // Test arity 2 produces Resolver
    test_resolver_type<2, 0>();
    test_resolver_type<2, 1>();

    // Test arity 3 produces Resolver
    test_resolver_type<3, 0>();
    test_resolver_type<3, 1>();
    test_resolver_type<3, 2>();
  }
};
[[maybe_unused]] constexpr auto resolver_sequence_compile_time_test =
    ResolverSequenceCompileTimeTest{};

// ----------------------------------------------------------------------------
// Run-Time Tests: Container Reference Preservation
// ----------------------------------------------------------------------------

struct ResolverSequenceRunTimeTest : ResolverSequenceFixture, Test {
  static constexpr auto unique_id = int_t{42};
  Container container{unique_id};
  Sut sut;
};

TEST_F(ResolverSequenceRunTimeTest, Arity0PreservesContainer) {
  auto resolver = sut.template create_element<Constructed, 0, 0>(container);

  EXPECT_EQ(resolver.container_ptr, &container);
  EXPECT_EQ(resolver.container_ptr->id, unique_id);
}

TEST_F(ResolverSequenceRunTimeTest, Arity1PreservesContainer) {
  auto single_arg_resolver =
      sut.template create_element<Constructed, 1, 0>(container);

  // SingleArgResolver wraps Resolver, so check the wrapped resolver
  EXPECT_EQ(single_arg_resolver.resolver.container_ptr, &container);
  EXPECT_EQ(single_arg_resolver.resolver.container_ptr->id, unique_id);
}

TEST_F(ResolverSequenceRunTimeTest, Arity2PreservesContainer) {
  auto resolver0 = sut.template create_element<Constructed, 2, 0>(container);
  auto resolver1 = sut.template create_element<Constructed, 2, 1>(container);

  EXPECT_EQ(resolver0.container_ptr, &container);
  EXPECT_EQ(resolver1.container_ptr, &container);
  EXPECT_EQ(resolver0.container_ptr->id, unique_id);
}

TEST_F(ResolverSequenceRunTimeTest, Arity3PreservesContainer) {
  auto resolver0 = sut.template create_element<Constructed, 3, 0>(container);
  auto resolver1 = sut.template create_element<Constructed, 3, 1>(container);
  auto resolver2 = sut.template create_element<Constructed, 3, 2>(container);

  EXPECT_EQ(resolver0.container_ptr, &container);
  EXPECT_EQ(resolver1.container_ptr, &container);
  EXPECT_EQ(resolver2.container_ptr, &container);
  EXPECT_EQ(resolver0.container_ptr->id, unique_id);
}

}  // namespace
}  // namespace dink
