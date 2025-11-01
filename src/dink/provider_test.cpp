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
    static inline const auto expected_value = int_t{2322};  // arbitrary

    int_t value;
  };

  struct Container {};

  // Stub invoker; returns canned ctor calls, passes factories through.
  template <typename Expected>
  struct Invoker {
    int_t ctor_param;

    // Returns Expected, passing ctor_param to the ctor.
    template <typename Requested, typename Container>
    constexpr auto create(Container&) const -> Requested {
      // Verify we're constructing the expected type.
      static_assert(std::same_as<Canonical<Requested>, Expected>);

      if constexpr (meta::IsSharedPtr<Requested>) {
        return std::make_shared<Expected>(ctor_param);
      } else if constexpr (meta::IsUniquePtr<Requested>) {
        return std::make_unique<Expected>(ctor_param);
      } else {
        return Expected{ctor_param};
      }
    }

    template <typename Requested, typename Container, typename Factory>
    constexpr auto create(Container&, Factory& factory) const -> Requested {
      // Verify we're constructing the expected type.
      static_assert(std::same_as<Canonical<Requested>, Expected>);

      if constexpr (meta::IsSharedPtr<Requested>) {
        return std::make_shared<Expected>(factory());
      } else if constexpr (meta::IsUniquePtr<Requested>) {
        return std::make_unique<Expected>(factory());
      } else {
        return Expected{factory()};
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
  static constexpr auto creates_expected_value() -> bool {
    Container container{};
    Sut sut{InvokerFactory{}};

    auto constructed = sut.create<Constructed>(container);

    return constructed.value == Constructed::expected_value;
  }

  constexpr CtorCompileTimeTest() { static_assert(creates_expected_value()); }
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
  static constexpr auto creates_expected_value() -> bool {
    Container container{};
    InvokerFactory invoker_factory{};
    Sut sut{ConstructedFactory{}, invoker_factory};

    auto constructed = sut.create<Constructed>(container);

    return constructed.value == Constructed::expected_value;
  }

  constexpr FactoryCompileTimeTest() {
    static_assert(creates_expected_value());
  }
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
  Constructed external_instance{Constructed::expected_value};

  using Sut = provider::External<Constructed>;
};

// Compile-Time Tests
// ----------------------------------------------------------------------------

struct InstanceCompileTimeTest : InstanceFixture {
  static constexpr auto returns_expected_reference() -> bool {
    Constructed ext{Constructed::expected_value};
    Container container{};
    Sut sut{ext};

    auto& ref = sut.create<Constructed&>(container);

    return &ref == &ext && ref.value == Constructed::expected_value;
  }

  static constexpr auto returns_value_copy() -> bool {
    Constructed ext{Constructed::expected_value};
    Container container{};
    Sut sut{ext};

    auto value = sut.create<Constructed>(container);

    // Value should match but be different address
    return value.value == Constructed::expected_value;
  }

  constexpr InstanceCompileTimeTest() {
    static_assert(returns_expected_reference(),
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
  auto& ref = sut.create<Constructed&>(container);

  EXPECT_EQ(&external_instance, &ref);
  EXPECT_EQ(Constructed::expected_value, ref.value);
}

TEST_F(ProviderInstanceRunTimeTest, ReturnsConstReferenceToExternal) {
  const auto& ref = sut.create<const Constructed&>(container);

  EXPECT_EQ(&external_instance, &ref);
  EXPECT_EQ(Constructed::expected_value, ref.value);
}

TEST_F(ProviderInstanceRunTimeTest, ReturnsValueCopy) {
  auto value = sut.create<Constructed>(container);

  EXPECT_EQ(Constructed::expected_value, value.value);
  EXPECT_NE(&external_instance, &value);  // Different addresses

  // Verify it's a true copy
  value.value = 123;
  EXPECT_EQ(Constructed::expected_value, external_instance.value);
}

TEST_F(ProviderInstanceRunTimeTest, ReturnsConstValueCopy) {
  const auto value = sut.create<const Constructed>(container);

  EXPECT_EQ(Constructed::expected_value, value.value);
}

TEST_F(ProviderInstanceRunTimeTest, MultipleCallsReturnSameReference) {
  auto& ref1 = sut.create<Constructed&>(container);
  auto& ref2 = sut.create<Constructed&>(container);

  EXPECT_EQ(&ref1, &ref2);
  EXPECT_EQ(&external_instance, &ref1);
}

TEST_F(ProviderInstanceRunTimeTest,
       ConstAndNonConstReferencesPointToSameExternal) {
  auto& mutable_ref = sut.create<Constructed&>(container);
  const auto& const_ref = sut.create<const Constructed&>(container);

  EXPECT_EQ(&mutable_ref, &const_ref);
  EXPECT_EQ(&external_instance, &mutable_ref);
}

TEST_F(ProviderInstanceRunTimeTest, MutationsThroughReferenceAffectExternal) {
  auto& ref = sut.create<Constructed&>(container);

  ref.value = 77;

  EXPECT_EQ(77, external_instance.value);
}

TEST_F(ProviderInstanceRunTimeTest, ValueCopiesAreIndependent) {
  auto copy1 = sut.create<Constructed>(container);
  auto copy2 = sut.create<Constructed>(container);

  copy1.value = 100;
  copy2.value = 200;

  EXPECT_EQ(100, copy1.value);
  EXPECT_EQ(200, copy2.value);
  EXPECT_EQ(Constructed::expected_value, external_instance.value);
}

// Test with non-copyable type (can only get references/pointers)
struct ProviderInstanceNonCopyableTest : Test {
  struct NoCopy {
    int_t value = 42;
    NoCopy() = default;
    NoCopy(const NoCopy&) = delete;
    NoCopy(NoCopy&&) = default;
  };

  NoCopy external_instance{};
  Fixture::Container container{};
  provider::External<NoCopy> sut{external_instance};
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
    virtual int_t get_value() const = 0;
  };

  struct Concrete : IAbstract {
    int_t value = 55;
    int_t get_value() const override { return value; }
  };

  Concrete concrete_instance{};
  Fixture::Container container{};
  provider::External<IAbstract> sut{concrete_instance};
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
  provider::External<SomeType> sut{instance};
};

TEST_F(ProviderInstanceProvidedTest, ProvidedAliasIsCorrect) {
  static_assert(std::same_as<provider::External<int_t>::Provided, int_t>);
  static_assert(std::same_as<decltype(sut)::Provided,
                             ProviderInstanceProvidedTest::SomeType>);
}

// Test with different containers (ensure container parameter works)
struct ProviderInstanceMultipleContainersTest : Test {
  struct Instance {
    int_t value;
    explicit Instance(int_t v) : value{v} {}
  };

  struct Container1 {
  };

  struct Container2 {
  };

  Instance external_instance{99};
  provider::External<Instance> sut{external_instance};

  Container1 container1;
  Container2 container2;
};

TEST_F(ProviderInstanceMultipleContainersTest,
       WorksWithDifferentContainerTypes) {
  // Should work with any container type (parameter ignored for Instance)
  auto& ref1 = sut.create<Instance&>(container1);
  auto& ref2 = sut.create<Instance&>(container2);

  EXPECT_EQ(&ref1, &ref2);
  EXPECT_EQ(&external_instance, &ref1);
  EXPECT_EQ(99, ref1.value);
}

}  // namespace
}  // namespace dink::provider
