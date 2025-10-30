// \file
// Copyright (c) 2025 Frank Secilia
// SPDX-License-Identifier: MIT

#include "dispatcher.hpp"
#include <dink/test.hpp>

namespace dink {
namespace {

// ----------------------------------------------------------------------------
// defaults::BindingLocator
// ----------------------------------------------------------------------------

struct DefaultsBindingLocatorTest {
  struct FromType {
    int_t id{};
  };
  static constexpr auto expected_from = FromType{3};

  struct Config {
    template <typename FromType>
    constexpr auto find_binding() const noexcept -> FromType {
      return expected_from;
    }
  };
  static constexpr Config config{};

  using Sut = defaults::BindingLocator;
  static constexpr Sut sut{};

  constexpr DefaultsBindingLocatorTest() {
    static_assert(expected_from.id == sut.template find<FromType>(config).id);
  }
};
[[maybe_unused]] constexpr auto defaults_bindings_locator_test =
    DefaultsBindingLocatorTest{};

// ----------------------------------------------------------------------------
// defaults::BindingLocator
// ----------------------------------------------------------------------------

struct DefaultsFallbackBindingFactoryTest {
  struct FromType {};

  using Sut = defaults::FallbackBindingFactory;
  static constexpr Sut sut{};

  constexpr DefaultsFallbackBindingFactoryTest() {
    static constexpr auto binding = sut.template create<FromType>();
    using Binding = decltype(binding);
    static_assert(std::same_as<FromType, Binding::FromType>);
    static_assert(std::same_as<scope::Transient, Binding::ScopeType>);
    static_assert(
        std::same_as<provider::Ctor<FromType>, Binding::ProviderType>);
  }
};
[[maybe_unused]] constexpr auto defaults_fallback_binding_factory_test =
    DefaultsFallbackBindingFactoryTest{};

// ----------------------------------------------------------------------------
// Dispatcher
// ----------------------------------------------------------------------------

struct DispatcherTest : Test {
  struct Binding {
    struct ScopeType {
      static constexpr auto provides_references = true;
    };

    using Id = int_t;
    Id id{};

    static constexpr auto initialized_id = Id{3};

    auto operator==(const Binding&) const noexcept -> bool = default;
  };
  Binding binding{Binding::initialized_id};

  struct Config {};
  Config config{};

  struct Container {};
  Container container{};

  struct Requested {};
  Requested requested;

  struct MockStrategy {
    MOCK_METHOD(Requested&, execute, (Container & container, Binding& binding),
                (const));
    virtual ~MockStrategy() = default;
  };
  StrictMock<MockStrategy> mock_strategy{};

  struct Strategy {
    MockStrategy* mock = nullptr;

    template <typename Requested, typename Container, typename Binding>
    auto execute(Container& container, Binding& binding) const -> Requested& {
      return mock->execute(container, binding);
    }
  };

  struct StrategyFactory {
    MockStrategy* mock_strategy = nullptr;
    template <typename Requested, bool has_binding,
              bool scope_provides_references>
    constexpr auto create() -> auto {
      static_assert(std::same_as<DispatcherTest::Requested&, Requested>);
      return Strategy{mock_strategy};
    }
  };
};

// Binding Found
// ----------------------------------------------------------------------------

struct DispatcherTestBindingFound : DispatcherTest {
  struct MockBindingLocator {
    MOCK_METHOD(Binding*, find, (Config & config), (const, noexcept));
    virtual ~MockBindingLocator() = default;
  };
  StrictMock<MockBindingLocator> mock_binding_locator;

  struct BindingLocator {
    MockBindingLocator* mock = nullptr;

    template <typename FromType, typename Config>
    constexpr auto find(Config& config) const -> Binding* {
      static_assert(std::same_as<Requested, FromType>);
      static_assert(std::same_as<DispatcherTest::Config, Config>);
      return mock->find(config);
    }
  };

  struct FallbackBindingFactory {};

  using Sut =
      Dispatcher<BindingLocator, FallbackBindingFactory, StrategyFactory>;
  Sut sut{BindingLocator{&mock_binding_locator}, FallbackBindingFactory{},
          StrategyFactory{&mock_strategy}};
};

TEST_F(DispatcherTestBindingFound, ResolveExecutesStrategyWithBinding) {
  EXPECT_CALL(mock_binding_locator, find(Ref(config)))
      .WillOnce(Return(&binding));
  EXPECT_CALL(mock_strategy, execute(Ref(container), Ref(binding)))
      .WillOnce(ReturnRef(requested));

  auto& result = sut.template resolve<Requested&>(container, config, nullptr);

  ASSERT_EQ(&result, &requested);
}

// Binding Not Found
// ----------------------------------------------------------------------------

struct DispatcherTestBindingNotFound : DispatcherTest {
  struct BindingLocator {
    template <typename FromType, typename Config>
    constexpr auto find(Config&) const -> std::nullptr_t {
      static_assert(std::same_as<Requested, FromType>);
      static_assert(std::same_as<DispatcherTest::Config, Config>);
      return nullptr;
    }
  };
};

// Binding Not Found, Use Fallback
// ----------------------------------------------------------------------------

struct DispatcherTestBindingNotFoundUseFallback
    : DispatcherTestBindingNotFound {
  struct FallbackBindingFactory {
    mutable Binding binding;

    template <typename FromType>
    constexpr auto create() const -> Binding& {
      static_assert(std::same_as<Requested, FromType>);
      return binding;
    }
  };

  using Sut =
      Dispatcher<BindingLocator, FallbackBindingFactory, StrategyFactory>;
  Sut sut{BindingLocator{}, FallbackBindingFactory{binding},
          StrategyFactory{&mock_strategy}};
};

TEST_F(DispatcherTestBindingNotFoundUseFallback,
       ResolveExecutesFallbackStrategy) {
  EXPECT_CALL(mock_strategy, execute(Ref(container), binding))
      .WillOnce(ReturnRef(requested));

  auto& result = sut.template resolve<Requested&>(container, config, nullptr);

  ASSERT_EQ(&result, &requested);
}

// Binding Not Found, Has Parent
// ----------------------------------------------------------------------------

struct DispatcherTestBindingNotFoundHasParent : DispatcherTestBindingNotFound {
  struct FallbackBindingFactory {};

  struct MockParent {
    MOCK_METHOD(Requested&, resolve, ());
    virtual ~MockParent() = default;
  };
  StrictMock<MockParent> mock_parent{};

  struct Parent {
    template <typename Requested>
    auto resolve() -> Requested& {
      return mock->resolve();
    }
    MockParent* mock = nullptr;
  };

  using Sut =
      Dispatcher<BindingLocator, FallbackBindingFactory, StrategyFactory>;
  Sut sut{BindingLocator{}, FallbackBindingFactory{},
          StrategyFactory{&mock_strategy}};
};

TEST_F(DispatcherTestBindingNotFoundHasParent, ResolveDelegatesToParent) {
  auto parent = Parent{&mock_parent};
  EXPECT_CALL(mock_parent, resolve()).WillOnce(ReturnRef(requested));

  auto& result = sut.template resolve<Requested&>(container, config, &parent);

  ASSERT_EQ(&result, &requested);
}

}  // namespace
}  // namespace dink
