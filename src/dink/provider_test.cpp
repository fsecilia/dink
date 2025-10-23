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

// Ctor
// ----------------------------------------------------------------------------

struct CtorFixture : CreatorFixture {
  using InvokerFactory = InvokerFactory<void>;
  using Sut = Ctor<Constructed, InvokerFactory>;
};

// Compile-Time Tests

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

// ----------------------------------------------------------------------------
// Accessors
// ----------------------------------------------------------------------------

struct ProviderAccessorTest : Test {
  struct Instance {
    static inline auto copy_ctors = int_t{0};
    static inline auto move_ctors = int_t{0};
    static inline auto copy_assigns = int_t{0};
    static inline auto move_assigns = int_t{0};
    static auto reset() -> void {
      copy_ctors = 0;
      move_ctors = 0;
      copy_assigns = 0;
      move_assigns = 0;
    }

    static constexpr auto default_id = int_t{3};
    static constexpr auto initialized_id = int_t{5};
    static constexpr auto mutated_id = int_t{7};
    int_t id = default_id;

    explicit Instance(int_t id) noexcept : id{id} {}
    Instance() = default;

    Instance(const Instance& src) noexcept : id{src.id} { ++copy_ctors; }
    auto operator=(const Instance& src) noexcept -> Instance& {
      id = src.id;
      ++copy_assigns;
      return *this;
    }

    Instance(Instance&& src) noexcept : id{std::move(src).id} { ++move_ctors; }
    auto operator=(Instance&& src) noexcept -> Instance& {
      id = std::move(src).id;
      ++move_assigns;
      return *this;
    }
  };

  ProviderAccessorTest() noexcept { Instance::reset(); }
};

TEST_F(ProviderAccessorTest, InternalReference) {
  using Sut = InternalReference<Instance>;

  auto src = Instance{Instance::initialized_id};
  Sut sut{std::move(src)};

  ASSERT_EQ(Instance::copy_ctors, 0);
  ASSERT_EQ(Instance::move_ctors, 1);

  ASSERT_EQ(Instance::initialized_id, sut.get().id);

  ASSERT_NE(&src, &sut.get());
  ASSERT_EQ(&sut.get(), &static_cast<const Sut&>(sut).get());

  sut.get().id = Instance::mutated_id;

  ASSERT_EQ(Instance::copy_ctors, 0);
  ASSERT_EQ(Instance::move_ctors, 1);

  ASSERT_EQ(Instance::mutated_id, sut.get().id);
}

TEST_F(ProviderAccessorTest, ExternalReference) {
  using Sut = ExternalReference<Instance>;

  auto src = Instance{Instance::initialized_id};
  Sut sut{src};

  ASSERT_EQ(Instance::copy_ctors, 0);
  ASSERT_EQ(Instance::move_ctors, 0);

  ASSERT_EQ(Instance::initialized_id, sut.get().id);

  ASSERT_EQ(&src, &sut.get());
  ASSERT_EQ(&sut.get(), &static_cast<const Sut&>(sut).get());

  src.id = Instance::mutated_id;

  ASSERT_EQ(Instance::copy_ctors, 0);
  ASSERT_EQ(Instance::move_ctors, 0);

  ASSERT_EQ(Instance::mutated_id, sut.get().id);
}

TEST_F(ProviderAccessorTest, InternalPrototype) {
  using Sut = InternalPrototype<Instance>;

  auto src = Instance{Instance::initialized_id};
  Sut sut{std::move(src)};

  ASSERT_EQ(Instance::copy_ctors, 0);
  ASSERT_EQ(Instance::move_ctors, 1);

  ASSERT_EQ(Instance::initialized_id, sut.get().id);
  ASSERT_EQ(Instance::copy_ctors, 1);

  ASSERT_EQ(Instance::initialized_id, static_cast<const Sut&>(sut).get().id);
  ASSERT_EQ(Instance::copy_ctors, 2);
}

TEST_F(ProviderAccessorTest, ExternalPrototype) {
  using Sut = ExternalPrototype<Instance>;

  auto src = Instance{Instance::initialized_id};
  Sut sut{src};

  ASSERT_EQ(Instance::copy_ctors, 0);
  ASSERT_EQ(Instance::move_ctors, 0);

  ASSERT_EQ(Instance::initialized_id, sut.get().id);
  ASSERT_EQ(Instance::copy_ctors, 1);

  ASSERT_EQ(Instance::initialized_id, static_cast<const Sut&>(sut).get().id);
  ASSERT_EQ(Instance::copy_ctors, 2);

  src.id = Instance::mutated_id;

  ASSERT_EQ(Instance::mutated_id, sut.get().id);
}

}  // namespace
}  // namespace dink::provider
