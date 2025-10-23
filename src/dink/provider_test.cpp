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

    template <typename Requested>
    constexpr auto create(auto& /*container*/) const -> Requested {
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
      static_assert(std::same_as<Factory, void>);

      return Invoker<Constructed>{TestConstructed::expected_value};
    }
  };
};

// ----------------------------------------------------------------------------
// CtorProvider
// ----------------------------------------------------------------------------

struct CtorProviderFixture : CreatorProviderFixture {
  using Sut = CtorProvider<TestConstructed, InvokerFactory>;
};

// Compile-Time Tests
// ----------------------------------------------------------------------------

struct CtorProviderCompileTimeTest : CtorProviderFixture {
  static constexpr auto creates_value() -> bool {
    Container container{};
    InvokerFactory invoker_factory{};
    Sut sut{invoker_factory};

    auto constructed = sut.create<Container, TestConstructed, DependencyChain,
                                  test_min_lifetime>(container);

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
  InvokerFactory invoker_factory{};
  Sut sut{invoker_factory};
};

TEST_F(CtorProviderRunTimeTest, CreatesSharedPtr) {
  const auto result = sut.create<Container, std::shared_ptr<TestConstructed>,
                                 DependencyChain, test_min_lifetime>(container);

  EXPECT_EQ(result->value, TestConstructed::expected_value);
}

TEST_F(CtorProviderRunTimeTest, CreatesUniquePtr) {
  const auto result = sut.create<Container, std::unique_ptr<TestConstructed>,
                                 DependencyChain, test_min_lifetime>(container);

  EXPECT_EQ(result->value, TestConstructed::expected_value);
}

}  // namespace
}  // namespace dink
