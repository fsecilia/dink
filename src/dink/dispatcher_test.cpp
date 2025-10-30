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
  struct UntypedBinding {
    using Id = int_t;
    Id id{};

    static constexpr auto initialized_id = Id{3};

    auto operator==(const UntypedBinding&) const noexcept -> bool = default;
  };

  struct ReferenceBinding : UntypedBinding {
    struct ScopeType {
      static constexpr auto provides_references = true;
    };
  };

  struct ValueBinding : UntypedBinding {
    struct ScopeType {
      static constexpr auto provides_references = false;
    };
  };

  struct Config {};
  Config config{};

  struct Container {};
  Container container{};

  struct Requested {};
  Requested requested;

  struct MockStrategy {
    MOCK_METHOD(Requested&, execute,
                (Container & container, UntypedBinding& binding), (const));
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

  struct MockStrategyFactory {
    MOCK_METHOD(Strategy, create,
                (bool has_binding, bool scope_provides_references));
    virtual ~MockStrategyFactory() = default;
  };
  StrictMock<MockStrategyFactory> mock_strategy_factory;

  struct StrategyFactory {
    MockStrategyFactory* mock = nullptr;

    template <typename Requested, bool has_binding,
              bool scope_provides_references>
    constexpr auto create() -> Strategy {
      static_assert(std::same_as<DispatcherTest::Requested&, Requested>);
      return mock->create(has_binding, scope_provides_references);
    }
  };
};

// Binding Found
// ----------------------------------------------------------------------------

struct DispatcherTestBindingFound : DispatcherTest {
  template <typename Binding>
  struct MockBindingLocator {
    MOCK_METHOD(Binding*, find, (Config & config), (const, noexcept));
    virtual ~MockBindingLocator() = default;
  };

  template <typename Binding>
  struct BindingLocator {
    MockBindingLocator<Binding>* mock = nullptr;

    template <typename FromType, typename Config>
    constexpr auto find(Config& config) const -> Binding* {
      static_assert(std::same_as<Requested, FromType>);
      static_assert(std::same_as<DispatcherTest::Config, Config>);
      return mock->find(config);
    }
  };

  struct FallbackBindingFactory {};
};

TEST_F(DispatcherTestBindingFound,
       ResolveExecutesStrategyWithReferenceBinding) {
  using Binding = ReferenceBinding;
  Binding binding{Binding::initialized_id};
  StrictMock<MockBindingLocator<Binding>> mock_binding_locator;

  using Sut = Dispatcher<BindingLocator<Binding>, FallbackBindingFactory,
                         StrategyFactory>;
  Sut sut{BindingLocator{&mock_binding_locator}, FallbackBindingFactory{},
          StrategyFactory{&mock_strategy_factory}};

  EXPECT_CALL(mock_binding_locator, find(Ref(config)))
      .WillOnce(Return(&binding));
  EXPECT_CALL(mock_strategy_factory, create(true, true))
      .WillOnce(Return(Strategy{&mock_strategy}));

  EXPECT_CALL(mock_strategy, execute(Ref(container), Ref(binding)))
      .WillOnce(ReturnRef(requested));

  auto& result = sut.template resolve<Requested&>(container, config, nullptr);

  ASSERT_EQ(&result, &requested);
}

TEST_F(DispatcherTestBindingFound, ResolveExecutesStrategyWithValueBinding) {
  using Binding = ValueBinding;
  Binding binding{Binding::initialized_id};
  StrictMock<MockBindingLocator<Binding>> mock_binding_locator;

  using Sut = Dispatcher<BindingLocator<Binding>, FallbackBindingFactory,
                         StrategyFactory>;
  Sut sut{BindingLocator{&mock_binding_locator}, FallbackBindingFactory{},
          StrategyFactory{&mock_strategy_factory}};

  EXPECT_CALL(mock_binding_locator, find(Ref(config)))
      .WillOnce(Return(&binding));
  EXPECT_CALL(mock_strategy_factory, create(true, false))
      .WillOnce(Return(Strategy{&mock_strategy}));

  EXPECT_CALL(mock_strategy, execute(Ref(container), binding))
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
  using Binding = ValueBinding;
  Binding binding{Binding::initialized_id};

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
          StrategyFactory{&mock_strategy_factory}};
};

TEST_F(DispatcherTestBindingNotFoundUseFallback,
       ResolveExecutesFallbackStrategy) {
  EXPECT_CALL(mock_strategy_factory, create(false, false))
      .WillOnce(Return(Strategy{&mock_strategy}));
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
          StrategyFactory{&mock_strategy_factory}};
};

TEST_F(DispatcherTestBindingNotFoundHasParent, ResolveDelegatesToParent) {
  auto parent = Parent{&mock_parent};
  EXPECT_CALL(mock_parent, resolve()).WillOnce(ReturnRef(requested));

  auto& result = sut.template resolve<Requested&>(container, config, &parent);

  ASSERT_EQ(&result, &requested);
}

}  // namespace
}  // namespace dink
