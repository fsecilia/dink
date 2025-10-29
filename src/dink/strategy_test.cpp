// \file
// Copyright (c) 2025 Frank Secilia
// SPDX-License-Identifier: MIT

#include "strategy.hpp"
#include <dink/test.hpp>

namespace dink {

struct StrategyTest : Test {
  struct Container {};

  struct Provider;
  struct ProviderFactory;
  struct Scope;

  struct Requested {
    const Container* container{};
    const Provider* provider{};
    const Scope* scope{};
  };

  struct Provider {
    const ProviderFactory* provider_factory{};

    using Provided = Requested;
    template <typename ActualRequested>
    auto create(Container& container) -> Requested {
      static_assert(std::same_as<Requested, ActualRequested>);
      return Requested{.container = &container, .provider = this};
    }
  };

  struct ProviderFactory {
    template <typename Canonical>
    auto create() const -> Provider {
      return Provider{this};
    }
  };

  struct Scope {};

  struct ValueScope : Scope {
    static constexpr auto provides_references = false;

    template <typename Requested>
    auto resolve(Container& container, Provider& provider) const -> Requested {
      return Requested{
          .container = &container, .provider = &provider, .scope = this};
    }
  };

  struct ReferenceScope : Scope {
    static constexpr auto provides_references = true;

    mutable Requested referenced{};

    template <typename Requested>
    auto resolve(Container& container, Provider& provider) const -> Requested& {
      referenced.container = &container;
      referenced.provider = &provider;
      referenced.scope = this;
      return referenced;
    }
  };

  struct ValueBinding {
    ValueScope scope;
    Provider provider;
  };

  struct ReferenceBinding {
    ReferenceScope scope;
    Provider provider;
  };
};

// ----------------------------------------------------------------------------
// StrategyImpls
// ----------------------------------------------------------------------------

struct StrategyImplsTest : StrategyTest {};

TEST_F(StrategyImplsTest, UseLocalScopeUsesMemberScopeAndBoundProvider) {
  using Sut = strategy_impls::UseLocalScope<ValueScope>;
  auto sut = Sut{ValueScope{}};
  auto container = Container{};
  auto binding = ValueBinding{};

  auto actual = sut.template execute<Requested>(container, binding);

  ASSERT_EQ(&container, actual.container);
  ASSERT_EQ(&binding.provider, actual.provider);
  ASSERT_EQ(&sut.scope, actual.scope);
}

TEST_F(StrategyImplsTest, UseLocalScopeAndProviderUsesMembers) {
  using Sut =
      strategy_impls::UseLocalScopeAndProvider<ValueScope, ProviderFactory>;
  auto sut = Sut{ValueScope{}, ProviderFactory{}};
  auto container = Container{};
  auto binding = ValueBinding{};

  auto actual = sut.template execute<Requested>(container, binding);

  ASSERT_EQ(&container, actual.container);
  ASSERT_EQ(&sut.provider_factory, actual.provider->provider_factory);
  ASSERT_EQ(&sut.scope, actual.scope);
}

// ----------------------------------------------------------------------------
// AliasingSharedPtr
// ----------------------------------------------------------------------------

struct AliasingSharedPtrTest : StrategyTest {
  struct ReferenceResolvingContainer : Container {
    mutable Requested referenced{};

    template <typename Constructed>
    auto resolve() -> Constructed& {
      return referenced;
    }
  };

  // clang-format off
  static_assert
  (
    std::same_as<
      decltype(
        std::declval<
          aliasing_shared_ptr::ProviderFactory
            <aliasing_shared_ptr::Provider>
        >()
        .template create<Requested>()),
        aliasing_shared_ptr::Provider<Requested>
    >
  );
  // clang-format on
};

TEST_F(AliasingSharedPtrTest, ProviderCreateAliasesReferenced) {
  using Sut = aliasing_shared_ptr::Provider<Requested>;
  auto sut = Sut{};
  auto container = ReferenceResolvingContainer{};

  auto actual = sut.template create<std::shared_ptr<Requested>>(container);

  ASSERT_EQ(&container.referenced, actual.get());
}

// ----------------------------------------------------------------------------
// Strategies
// ----------------------------------------------------------------------------

struct StrategiesTest : StrategyTest {};

TEST_F(StrategiesTest, UseBindingUsesProvidedBinding) {
  using Sut = strategies::UseBinding;
  auto sut = Sut{};
  auto container = Container{};
  auto binding = ValueBinding{};

  auto actual = sut.template execute<Requested>(container, binding);

  ASSERT_EQ(&container, actual.container);
  ASSERT_EQ(&binding.provider, actual.provider);
  ASSERT_EQ(&binding.scope, actual.scope);
}

}  // namespace dink
