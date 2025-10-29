// \file
// Copyright (c) 2025 Frank Secilia
// SPDX-License-Identifier: MIT

#include "provider.hpp"
#include <dink/test.hpp>
#include <dink/canonical.hpp>
#include <concepts>

namespace dink::provider {
namespace {

struct Fixture {
  struct Constructed {
    static inline const int_t default_value = 3;
    static inline const int_t expected_value = 5;
    int_t value;
    explicit constexpr Constructed(int_t value = default_value)
        : value{value} {}
  };

  struct Container {};

  // Stub invoker that returns canned values.
  template <typename ExpectedConstructed>
  struct Invoker {
    int_t ctor_specialization_return_value;

    template <typename Requested, typename Container>
    constexpr auto create(Container&) const -> Requested {
      // Verify we're constructing the expected type.
      static_assert(std::same_as<Canonical<Requested>, ExpectedConstructed>);

      if constexpr (IsSharedPtr<Requested>) {
        return std::make_shared<ExpectedConstructed>(
            ctor_specialization_return_value);
      } else if constexpr (IsUniquePtr<Requested>) {
        return std::make_unique<ExpectedConstructed>(
            ctor_specialization_return_value);
      } else {
        return ExpectedConstructed{ctor_specialization_return_value};
      }
    }

    template <typename Requested, typename Container, typename Factory>
    constexpr auto create(Container&, Factory& factory) const -> Requested {
      // Verify we're constructing the expected type.
      static_assert(std::same_as<Canonical<Requested>, ExpectedConstructed>);

      if constexpr (IsSharedPtr<Requested>) {
        return std::make_shared<ExpectedConstructed>(factory());
      } else if constexpr (IsUniquePtr<Requested>) {
        return std::make_unique<ExpectedConstructed>(factory());
      } else {
        return ExpectedConstructed{factory()};
      }
    }
  };

  // InvokerFactory spy that verifies template args and returns StubInvoker.
  template <typename ExpectedFactory>
  struct InvokerFactory {
    template <typename Constructed, typename Factory>
    constexpr auto create() -> Invoker<Constructed> {
      // Verify the factory was called with expected template parameters
      static_assert(std::same_as<Constructed, Constructed>);
      static_assert(std::same_as<Factory, ExpectedFactory>);

      return Invoker<Constructed>{Constructed::expected_value};
    }
  };

  auto test_result(const Constructed& result) -> void {
    EXPECT_EQ(result.value, Constructed::expected_value);
  }
};

// ----------------------------------------------------------------------------
// Ctor
// ----------------------------------------------------------------------------

struct CtorFixture : Fixture {
  using InvokerFactory = InvokerFactory<void>;
  using Sut = Ctor<Constructed, InvokerFactory>;
};

// Compile-Time Tests
// ----------------------------------------------------------------------------

struct CtorCompileTimeTest : CtorFixture {
  static constexpr auto creates_value() -> bool {
    Container container{};
    Sut sut{InvokerFactory{}};

    auto constructed = sut.create<Constructed>(container);

    return constructed.value == Constructed::expected_value;
  }

  constexpr CtorCompileTimeTest() { static_assert(creates_value()); }
};
[[maybe_unused]] constexpr auto ctor_provider_compile_time_test =
    CtorCompileTimeTest{};

// Run-time Tests
// ----------------------------------------------------------------------------

struct ProviderCtorRunTimeTest : CtorFixture, Test {
  Container container{};
  Sut sut{InvokerFactory{}};
};

TEST_F(ProviderCtorRunTimeTest, CreatesSharedPtr) {
  test_result(*sut.create<std::shared_ptr<Constructed>>(container));
}

TEST_F(ProviderCtorRunTimeTest, CreatesUniquePtr) {
  test_result(*sut.create<std::unique_ptr<Constructed>>(container));
}

// ----------------------------------------------------------------------------
// Factory
// ----------------------------------------------------------------------------

struct FactoryFixture : Fixture {
  struct ConstructedFactory {
    constexpr auto operator()() noexcept -> Constructed {
      return Constructed{Constructed::expected_value};
    }
  };
  using InvokerFactory = InvokerFactory<ConstructedFactory>;

  using Sut = Factory<Constructed, ConstructedFactory, InvokerFactory>;
};

// Compile-Time Tests
// ----------------------------------------------------------------------------

struct FactoryCompileTimeTest : FactoryFixture {
  static constexpr auto creates_value() -> bool {
    Container container{};
    InvokerFactory invoker_factory{};
    Sut sut{ConstructedFactory{}, invoker_factory};

    auto constructed = sut.create<Constructed>(container);

    return constructed.value == Constructed::expected_value;
  }

  constexpr FactoryCompileTimeTest() { static_assert(creates_value()); }
};
[[maybe_unused]] constexpr auto factory_provider_compile_time_test =
    FactoryCompileTimeTest{};

// Run-time Tests
// ----------------------------------------------------------------------------

struct ProviderFactoryRunTimeTest : FactoryFixture, Test {
  Container container{};
  Sut sut{ConstructedFactory{}, InvokerFactory{}};
};

TEST_F(ProviderFactoryRunTimeTest, CreatesSharedPtr) {
  test_result(*sut.create<std::shared_ptr<Constructed>>(container));
}

TEST_F(ProviderFactoryRunTimeTest, CreatesUniquePtr) {
  test_result(*sut.create<std::unique_ptr<Constructed>>(container));
}

// ----------------------------------------------------------------------------
// Instance
// ----------------------------------------------------------------------------

struct InstanceFixture : Fixture {
  struct External {
    static inline const int_t default_value = 42;
    static inline const int_t expected_value = 99;
    int_t value;
    explicit constexpr External(int_t value = default_value) : value{value} {}
  };

  External external_instance{External::expected_value};

  using Sut = provider::Instance<External>;
};

// Compile-Time Tests
// ----------------------------------------------------------------------------

struct InstanceCompileTimeTest : InstanceFixture {
  static constexpr auto returns_reference() -> bool {
    External ext{External::expected_value};
    Container container{};
    Sut sut{ext};

    auto& ref = sut.create<External&>(container);

    return &ref == &ext && ref.value == External::expected_value;
  }

  static constexpr auto returns_value_copy() -> bool {
    External ext{External::expected_value};
    Container container{};
    Sut sut{ext};

    auto value = sut.create<External>(container);

    // Value should match but be different address
    return value.value == External::expected_value;
  }

  constexpr InstanceCompileTimeTest() {
    static_assert(returns_reference(),
                  "Instance provider reference test failed");
    static_assert(returns_value_copy(), "Instance provider value test failed");
  }
};
[[maybe_unused]] constexpr auto instance_provider_compile_time_test =
    InstanceCompileTimeTest{};

// Run-time Tests
// ----------------------------------------------------------------------------

struct ProviderInstanceRunTimeTest : InstanceFixture, Test {
  Container container{};
  Sut sut{external_instance};
};

TEST_F(ProviderInstanceRunTimeTest, ReturnsReferenceToExternal) {
  auto& ref = sut.create<External&>(container);

  EXPECT_EQ(&external_instance, &ref);
  EXPECT_EQ(External::expected_value, ref.value);
}

TEST_F(ProviderInstanceRunTimeTest, ReturnsConstReferenceToExternal) {
  const auto& ref = sut.create<const External&>(container);

  EXPECT_EQ(&external_instance, &ref);
  EXPECT_EQ(External::expected_value, ref.value);
}

TEST_F(ProviderInstanceRunTimeTest, ReturnsValueCopy) {
  auto value = sut.create<External>(container);

  EXPECT_EQ(External::expected_value, value.value);
  EXPECT_NE(&external_instance, &value);  // Different addresses

  // Verify it's a true copy
  value.value = 123;
  EXPECT_EQ(External::expected_value, external_instance.value);
}

TEST_F(ProviderInstanceRunTimeTest, ReturnsConstValueCopy) {
  const auto value = sut.create<const External>(container);

  EXPECT_EQ(External::expected_value, value.value);
}

TEST_F(ProviderInstanceRunTimeTest, MultipleCallsReturnSameReference) {
  auto& ref1 = sut.create<External&>(container);
  auto& ref2 = sut.create<External&>(container);

  EXPECT_EQ(&ref1, &ref2);
  EXPECT_EQ(&external_instance, &ref1);
}

TEST_F(ProviderInstanceRunTimeTest,
       ConstAndNonConstReferencesPointToSameExternal) {
  auto& mutable_ref = sut.create<External&>(container);
  const auto& const_ref = sut.create<const External&>(container);

  EXPECT_EQ(&mutable_ref, &const_ref);
  EXPECT_EQ(&external_instance, &mutable_ref);
}

TEST_F(ProviderInstanceRunTimeTest, MutationsThroughReferenceAffectExternal) {
  auto& ref = sut.create<External&>(container);

  ref.value = 77;

  EXPECT_EQ(77, external_instance.value);
}

TEST_F(ProviderInstanceRunTimeTest, ValueCopiesAreIndependent) {
  auto copy1 = sut.create<External>(container);
  auto copy2 = sut.create<External>(container);

  copy1.value = 100;
  copy2.value = 200;

  EXPECT_EQ(100, copy1.value);
  EXPECT_EQ(200, copy2.value);
  EXPECT_EQ(External::expected_value, external_instance.value);
}

// Test with non-copyable type (can only get references/pointers)
struct ProviderInstanceNonCopyableTest : Test {
  struct NoCopy {
    int value = 42;
    NoCopy() = default;
    NoCopy(const NoCopy&) = delete;
    NoCopy(NoCopy&&) = default;
  };

  NoCopy external_instance{};
  Fixture::Container container{};
  provider::Instance<NoCopy> sut{external_instance};
};

TEST_F(ProviderInstanceNonCopyableTest, ReturnsReferenceToNonCopyable) {
  auto& ref = sut.create<NoCopy&>(container);

  EXPECT_EQ(&external_instance, &ref);
  EXPECT_EQ(42, ref.value);
}

// Test with abstract base class (can only get references/pointers)
struct ProviderInstanceAbstractTest : Test {
  struct IAbstract {
    virtual ~IAbstract() = default;
    virtual int get_value() const = 0;
  };

  struct Concrete : IAbstract {
    int value = 55;
    int get_value() const override { return value; }
  };

  Concrete concrete_instance{};
  Fixture::Container container{};
  provider::Instance<IAbstract> sut{concrete_instance};
};

TEST_F(ProviderInstanceAbstractTest, ReturnsReferenceToAbstract) {
  auto& ref = sut.create<IAbstract&>(container);

  EXPECT_EQ(&concrete_instance, &ref);
  EXPECT_EQ(55, ref.get_value());
}

TEST_F(ProviderInstanceAbstractTest, ReturnsConstReferenceToAbstract) {
  const auto& ref = sut.create<const IAbstract&>(container);

  EXPECT_EQ(&concrete_instance, &ref);
  EXPECT_EQ(55, ref.get_value());
}

// Test Provided alias
struct ProviderInstanceProvidedTest : Test {
  struct SomeType {};
  SomeType instance;
  provider::Instance<SomeType> sut{instance};
};

TEST_F(ProviderInstanceProvidedTest, ProvidedAliasIsCorrect) {
  static_assert(std::same_as<provider::Instance<int>::Provided, int>);
  static_assert(std::same_as<decltype(sut)::Provided,
                             ProviderInstanceProvidedTest::SomeType>);
}

// Test with different containers (ensure container parameter works)
struct ProviderInstanceMultipleContainersTest : Test {
  struct External {
    int value;
    explicit External(int v) : value{v} {}
  };

  struct Container1 {
    int id = 1;
  };

  struct Container2 {
    int id = 2;
  };

  External external_instance{99};
  provider::Instance<External> sut{external_instance};

  Container1 container1;
  Container2 container2;
};

TEST_F(ProviderInstanceMultipleContainersTest,
       WorksWithDifferentContainerTypes) {
  // Should work with any container type (parameter ignored for Instance)
  auto& ref1 = sut.create<External&>(container1);
  auto& ref2 = sut.create<External&>(container2);

  EXPECT_EQ(&ref1, &ref2);
  EXPECT_EQ(&external_instance, &ref1);
  EXPECT_EQ(99, ref1.value);
}

}  // namespace
}  // namespace dink::provider
