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
    const ProviderFactory* provider_factory{};
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
      return Provider{.provider_factory = this};
    }
  };

  struct Scope {
    static constexpr auto provides_references = false;

    template <typename Requested>
    auto resolve(Container& container, Provider& provider) const -> Requested {
      return Requested{.container = &container,
                       .provider = &provider,
                       .provider_factory = provider.provider_factory,
                       .scope = this};
    }
  };

  struct Binding {
    Scope scope;
    Provider provider;
  };
};

// ----------------------------------------------------------------------------
// StrategyImpls
// ----------------------------------------------------------------------------

struct StrategyImplsTest : StrategyTest {};

TEST_F(StrategyImplsTest, BoundScopeWithBoundProvider) {
  using Sut = strategy_impls::BoundScopeWithBoundProvider;
  auto sut = Sut{};
  auto container = Container{};
  auto binding = Binding{};

  auto actual = sut.template execute<Requested>(container, binding);

  ASSERT_EQ(&container, actual.container);
  ASSERT_EQ(&binding.provider, actual.provider);
  ASSERT_EQ(&binding.scope, actual.scope);
}

TEST_F(StrategyImplsTest, LocalScopeWithBoundProvider) {
  using Sut = strategy_impls::LocalScopeWithBoundProvider<Scope>;
  auto sut = Sut{Scope{}};
  auto container = Container{};
  auto binding = Binding{};

  auto actual = sut.template execute<Requested>(container, binding);

  ASSERT_EQ(&container, actual.container);
  ASSERT_EQ(&binding.provider, actual.provider);
  ASSERT_EQ(&sut.scope, actual.scope);
}

TEST_F(StrategyImplsTest, LocalScopeWithLocalProvider) {
  using Sut =
      strategy_impls::LocalScopeWithLocalProvider<Scope, ProviderFactory>;
  auto sut = Sut{Scope{}, ProviderFactory{}};
  auto container = Container{};
  auto binding = Binding{};

  auto actual = sut.template execute<Requested>(container, binding);

  ASSERT_EQ(&container, actual.container);
  ASSERT_EQ(&sut.provider_factory, actual.provider_factory);
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
};

TEST_F(AliasingSharedPtrTest, ProviderCreateAliasesReferenced) {
  using Sut = aliasing_shared_ptr::Provider<Requested>;
  auto sut = Sut{};
  auto container = ReferenceResolvingContainer{};

  auto actual = sut.template create<std::shared_ptr<Requested>>(container);

  ASSERT_EQ(&container.referenced, actual.get());
}

// ----------------------------------------------------------------------------
// StrategyFactory
// ----------------------------------------------------------------------------

struct StrategyFactoryTest : StrategyTest {
  using Sut = StrategyFactory;
  Sut sut{};

  template <typename Expected, typename Requested, bool has_binding,
            bool scope_provides_references>
  static constexpr auto test =
      std::same_as<Expected,
                   decltype(sut.template create<Requested, has_binding,
                                                scope_provides_references>())>;

  static_assert(test<strategies::RelegateToTransient,
                     std::unique_ptr<Requested>, false, false>);
  static_assert(test<strategies::RelegateToTransient,
                     std::unique_ptr<Requested>, false, true>);
  static_assert(test<strategies::RelegateToTransient,
                     std::unique_ptr<Requested>, true, false>);
  static_assert(test<strategies::RelegateToTransient,
                     std::unique_ptr<Requested>, true, true>);

  static_assert(test<strategies::CacheSharedPtr, std::shared_ptr<Requested>,
                     false, false>);
  static_assert(test<strategies::CacheSharedPtr, std::shared_ptr<Requested>,
                     false, true>);
  static_assert(
      test<strategies::UseBinding, std::shared_ptr<Requested>, true, false>);
  static_assert(
      test<strategies::CacheSharedPtr, std::shared_ptr<Requested>, true, true>);

  static_assert(
      test<strategies::CacheSharedPtr, std::weak_ptr<Requested>, false, false>);
  static_assert(
      test<strategies::CacheSharedPtr, std::weak_ptr<Requested>, false, true>);
  static_assert(
      test<strategies::CacheSharedPtr, std::weak_ptr<Requested>, true, false>);
  static_assert(
      test<strategies::CacheSharedPtr, std::weak_ptr<Requested>, true, true>);

  static_assert(test<strategies::PromoteToSingleton, Requested&, false, false>);
  static_assert(test<strategies::PromoteToSingleton, Requested&, false, true>);
  static_assert(test<strategies::PromoteToSingleton, Requested&, true, false>);
  static_assert(test<strategies::UseBinding, Requested&, true, true>);

  static_assert(test<strategies::PromoteToSingleton, Requested*, false, false>);
  static_assert(test<strategies::PromoteToSingleton, Requested*, false, true>);
  static_assert(test<strategies::PromoteToSingleton, Requested*, true, false>);
  static_assert(test<strategies::UseBinding, Requested*, true, true>);

  static_assert(test<strategies::RelegateToTransient, Requested, false, false>);
  static_assert(test<strategies::RelegateToTransient, Requested, false, true>);
  static_assert(test<strategies::UseBinding, Requested, true, false>);
  static_assert(test<strategies::UseBinding, Requested, true, true>);

  static_assert(
      test<strategies::RelegateToTransient, Requested&&, false, false>);
  static_assert(
      test<strategies::RelegateToTransient, Requested&&, false, true>);
  static_assert(test<strategies::UseBinding, Requested&&, true, false>);
  static_assert(test<strategies::UseBinding, Requested&&, true, true>);
};

}  // namespace dink
