// \file
// Copyright (c) 2025 Frank Secilia
// SPDX-License-Identifier: MIT

#include "provider.hpp"
#include <dink/test.hpp>

namespace dink::provider {
namespace {

// ----------------------------------------------------------------------------
// Creators
// ----------------------------------------------------------------------------

struct CreatorFixture {
  struct Constructed {
    static inline const int_t default_value = 3;
    static inline const int_t expected_value = 5;
    int_t value;
    explicit constexpr Constructed(int_t value = default_value)
        : value{value} {}
  };

  struct Container {};
  using DependencyChain = TypeList<>;
  static constexpr auto test_min_lifetime = scope::Lifetime::kDefault;

  // Stub invoker that returns canned values.
  template <typename ExpectedConstructed>
  struct Invoker {
    int_t ctor_specialization_return_value;

    template <typename Requested, typename Container>
    constexpr auto create(Container&) const -> Requested {
      // Verify we're constructing the expected type.
      static_assert(std::same_as<Canonical<Requested>, ExpectedConstructed>);

      if constexpr (SharedPtr<Requested>) {
        return std::make_shared<ExpectedConstructed>(
            ctor_specialization_return_value);
      } else if constexpr (UniquePtr<Requested>) {
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

      if constexpr (SharedPtr<Requested>) {
        return std::make_shared<ExpectedConstructed>(factory());
      } else if constexpr (UniquePtr<Requested>) {
        return std::make_unique<ExpectedConstructed>(factory());
      } else {
        return ExpectedConstructed{factory()};
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

struct CtorFixture : CreatorFixture {
  using InvokerFactory = InvokerFactory<void>;
  using Sut = Ctor<Constructed, InvokerFactory>;
};

// Compile-Time Tests
// ----------------------------------------------------------------------------

struct CtorCompileTimeTest : CtorFixture {
  static constexpr auto creates_value() -> bool {
    Container container{};
    Sut sut{InvokerFactory{}};

    auto constructed =
        sut.create<Constructed, DependencyChain, test_min_lifetime>(container);

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
  test_result(*sut.create<std::shared_ptr<Constructed>, DependencyChain,
                          test_min_lifetime>(container));
}

TEST_F(ProviderCtorRunTimeTest, CreatesUniquePtr) {
  test_result(*sut.create<std::unique_ptr<Constructed>, DependencyChain,
                          test_min_lifetime>(container));
}

// ----------------------------------------------------------------------------
// Factory
// ----------------------------------------------------------------------------

struct FactoryFixture : CreatorFixture {
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

    auto constructed =
        sut.create<Constructed, DependencyChain, test_min_lifetime>(container);

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
  test_result(*sut.create<std::shared_ptr<Constructed>, DependencyChain,
                          test_min_lifetime>(container));
}

TEST_F(ProviderFactoryRunTimeTest, CreatesUniquePtr) {
  test_result(*sut.create<std::unique_ptr<Constructed>, DependencyChain,
                          test_min_lifetime>(container));
}

}  // namespace
}  // namespace dink::provider
