// \file
// Copyright (c) 2025 Frank Secilia
// SPDX-License-Identifier: MIT

#include "provider.hpp"
#include <dink/test.hpp>

namespace dink {
namespace {

// ----------------------------------------------------------------------------
// Fixtures
// ----------------------------------------------------------------------------

struct ProviderFixture {
  struct TestConstructed {
    static inline const int_t default_value = 3;
    static inline const int_t expected_value = 5;
    int_t value;
    explicit constexpr TestConstructed(int_t value = default_value)
        : value{value} {}
  };
};

struct CreatorProviderFixture : ProviderFixture {
  struct Container {};
  using DependencyChain = TypeList<>;
  static constexpr auto test_min_lifetime = scope::Lifetime::kDefault;

  // Stub invoker that returns canned values.
  template <typename ExpectedConstructed>
  struct Invoker {
    int_t return_value;

    template <typename Requested, typename Container>
    constexpr auto create(Container&) const -> Requested {
      // Verify we're constructing the expected type.
      static_assert(std::same_as<Canonical<Requested>, ExpectedConstructed>);

      if constexpr (SharedPtr<Requested>) {
        return std::make_shared<ExpectedConstructed>(return_value);
      } else if constexpr (UniquePtr<Requested>) {
        return std::make_unique<ExpectedConstructed>(return_value);
      } else {
        return ExpectedConstructed{return_value};
      }
    }

    template <typename Requested, typename Container, typename Factory>
    constexpr auto create(Container&, Factory&) const -> Requested {
      // Verify we're constructing the expected type.
      static_assert(std::same_as<Canonical<Requested>, ExpectedConstructed>);

      if constexpr (SharedPtr<Requested>) {
        return std::make_shared<ExpectedConstructed>(return_value);
      } else if constexpr (UniquePtr<Requested>) {
        return std::make_unique<ExpectedConstructed>(return_value);
      } else {
        return ExpectedConstructed{return_value};
      }
    }
  };

  // InvokerFactory spy that verifies template args and returns StubInvoker.
  template <typename ExpectedFactory>
  struct InvokerFactory {
    template <typename Container, typename DependencyChain,
              scope::Lifetime min_lifetime, typename Constructed,
              typename Factory>
    constexpr auto create() -> Invoker<Constructed> {
      // Verify the factory was called with expected template parameters
      static_assert(std::same_as<Container, Container>);
      static_assert(std::same_as<DependencyChain, DependencyChain>);
      static_assert(min_lifetime == test_min_lifetime);
      static_assert(std::same_as<Constructed, TestConstructed>);
      static_assert(std::same_as<Factory, ExpectedFactory>);

      return Invoker<Constructed>{TestConstructed::expected_value};
    }
  };
};

// ----------------------------------------------------------------------------
// CtorProvider
// ----------------------------------------------------------------------------

struct CtorProviderFixture : CreatorProviderFixture {
  using InvokerFactory = InvokerFactory<void>;
  using Sut = CtorProvider<TestConstructed, InvokerFactory>;
};

// Compile-Time Tests
// ----------------------------------------------------------------------------

struct CtorProviderCompileTimeTest : CtorProviderFixture {
  static constexpr auto creates_value() -> bool {
    Container container{};
    Sut sut{InvokerFactory{}};

    auto constructed =
        sut.create<TestConstructed, DependencyChain, test_min_lifetime>(
            container);

    return constructed.value == TestConstructed::expected_value;
  }

  constexpr CtorProviderCompileTimeTest() { static_assert(creates_value()); }
};
[[maybe_unused]] constexpr auto ctor_provider_compile_time_test =
    CtorProviderCompileTimeTest{};

// Run-time Tests
// ----------------------------------------------------------------------------

struct CtorProviderRunTimeTest : CtorProviderFixture, Test {
  Container container{};
  Sut sut{InvokerFactory{}};
};

TEST_F(CtorProviderRunTimeTest, CreatesSharedPtr) {
  const auto result = sut.create<std::shared_ptr<TestConstructed>,
                                 DependencyChain, test_min_lifetime>(container);

  EXPECT_EQ(result->value, TestConstructed::expected_value);
}

TEST_F(CtorProviderRunTimeTest, CreatesUniquePtr) {
  const auto result = sut.create<std::unique_ptr<TestConstructed>,
                                 DependencyChain, test_min_lifetime>(container);

  EXPECT_EQ(result->value, TestConstructed::expected_value);
}

// ----------------------------------------------------------------------------
// FactoryProvider
// ----------------------------------------------------------------------------

struct FactoryProviderFixture : CreatorProviderFixture {
  struct ConstructedFactory {
    auto operator()() noexcept -> TestConstructed {
      return TestConstructed{TestConstructed::expected_value};
    }
  };
  using InvokerFactory = InvokerFactory<ConstructedFactory>;

  using Sut =
      FactoryProvider<TestConstructed, ConstructedFactory, InvokerFactory>;
};

// Compile-Time Tests
// ----------------------------------------------------------------------------

struct FactoryProviderCompileTimeTest : FactoryProviderFixture {
  static constexpr auto creates_value() -> bool {
    Container container{};
    InvokerFactory invoker_factory{};
    Sut sut{ConstructedFactory{}, invoker_factory};

    auto constructed =
        sut.create<TestConstructed, DependencyChain, test_min_lifetime>(
            container);

    return constructed.value == TestConstructed::expected_value;
  }

  constexpr FactoryProviderCompileTimeTest() { static_assert(creates_value()); }
};
[[maybe_unused]] constexpr auto Factory_provider_compile_time_test =
    FactoryProviderCompileTimeTest{};

// Run-time Tests
// ----------------------------------------------------------------------------

struct FactoryProviderRunTimeTest : FactoryProviderFixture, Test {
  Container container{};
  Sut sut{ConstructedFactory{}, InvokerFactory{}};
};

TEST_F(FactoryProviderRunTimeTest, CreatesSharedPtr) {
  const auto result = sut.create<std::shared_ptr<TestConstructed>,
                                 DependencyChain, test_min_lifetime>(container);

  EXPECT_EQ(result->value, TestConstructed::expected_value);
}

TEST_F(FactoryProviderRunTimeTest, CreatesUniquePtr) {
  const auto result = sut.create<std::unique_ptr<TestConstructed>,
                                 DependencyChain, test_min_lifetime>(container);

  EXPECT_EQ(result->value, TestConstructed::expected_value);
}

}  // namespace
}  // namespace dink
