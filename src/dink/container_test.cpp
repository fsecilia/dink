// \file
// Copyright (c) 2025 Frank Secilia
// SPDX-License-Identifier: MIT

#include "container.hpp"
#include <dink/test.hpp>
#include <dink/scope.hpp>

namespace dink {

// ----------------------------------------------------------------------------
// Singleton Scope Tests
// ----------------------------------------------------------------------------

struct ContainerSingletonTest : Test {};

TEST_F(ContainerSingletonTest, canonical_shared_wraps_instance) {
  struct SingletonBound {};
  auto sut = Container{bind<SingletonBound>().in<scope::Singleton>()};

  const auto shared = sut.template resolve<std::shared_ptr<SingletonBound>>();
  auto& instance = sut.template resolve<SingletonBound&>();
  ASSERT_EQ(&instance, shared.get());
}

TEST_F(ContainerSingletonTest, canonical_shared_ptr_value) {
  struct SingletonBound {};
  auto sut = Container{bind<SingletonBound>().in<scope::Singleton>()};

  const auto result1 = sut.template resolve<std::shared_ptr<SingletonBound>>();
  const auto result2 = sut.template resolve<std::shared_ptr<SingletonBound>>();
  ASSERT_EQ(result1, result2);
  ASSERT_EQ(result1.use_count(), result2.use_count());
  ASSERT_EQ(result1.use_count(), 3);  // result1 + result2 + canonical

  auto& instance = sut.template resolve<SingletonBound&>();
  ASSERT_EQ(&instance, result1.get());
}

TEST_F(ContainerSingletonTest, canonical_shared_ptr_identity) {
  struct SingletonBound {};
  auto sut = Container{bind<SingletonBound>().in<scope::Singleton>()};

  const auto& result1 =
      sut.template resolve<std::shared_ptr<SingletonBound>&>();
  const auto& result2 =
      sut.template resolve<std::shared_ptr<SingletonBound>&>();
  ASSERT_EQ(&result1, &result2);
  ASSERT_EQ(result1.use_count(), result2.use_count());
  ASSERT_EQ(result1.use_count(), 1);
}

TEST_F(ContainerSingletonTest, weak_ptr_from_singleton) {
  struct SingletonBound {};
  auto sut = Container{bind<SingletonBound>().in<scope::Singleton>()};

  auto weak1 = sut.template resolve<std::weak_ptr<SingletonBound>>();
  auto weak2 = sut.template resolve<std::weak_ptr<SingletonBound>>();

  EXPECT_FALSE(weak1.expired());
  EXPECT_EQ(weak1.lock(), weak2.lock());
}

TEST_F(ContainerSingletonTest, weak_ptr_does_not_expire_while_singleton_alive) {
  struct SingletonBound {};
  auto sut = Container{bind<SingletonBound>().in<scope::Singleton>()};

  const auto& weak = sut.template resolve<std::weak_ptr<SingletonBound>>();

  // Even with no shared_ptr in scope, weak_ptr should not expire
  // because it tracks the canonical shared_ptr which aliases the static
  EXPECT_FALSE(weak.expired());

  auto shared = weak.lock();
  EXPECT_NE(nullptr, shared);
}

TEST_F(ContainerSingletonTest, weak_ptr_expires_with_canonical_shared_ptr) {
  struct SingletonBound {};
  auto sut = Container{bind<SingletonBound>().in<scope::Singleton>()};

  // resolve reference directly to canonical shared_ptr
  auto& canonical_shared_ptr =
      sut.template resolve<std::shared_ptr<SingletonBound>&>();
  const auto weak = sut.template resolve<std::weak_ptr<SingletonBound>>();

  EXPECT_FALSE(weak.expired());
  canonical_shared_ptr.reset();
  EXPECT_TRUE(weak.expired());
}

TEST_F(ContainerSingletonTest, const_shared_ptr) {
  struct SingletonBound {};
  auto sut = Container{bind<SingletonBound>().in<scope::Singleton>()};

  auto shared = sut.template resolve<std::shared_ptr<const SingletonBound>>();
  auto& instance = sut.template resolve<SingletonBound&>();

  EXPECT_EQ(&instance, shared.get());
}

TEST_F(ContainerSingletonTest, multiple_singleton_types) {
  struct A {};
  struct B {};

  auto sut = Container{bind<A>().in<scope::Singleton>(),
                       bind<B>().in<scope::Singleton>()};

  auto shared_a = sut.template resolve<std::shared_ptr<A>>();
  auto shared_b = sut.template resolve<std::shared_ptr<B>>();

  EXPECT_NE(shared_a.get(), nullptr);
  EXPECT_NE(shared_b.get(), nullptr);
}

// ----------------------------------------------------------------------------
// Transient Scope Tests
// ----------------------------------------------------------------------------

struct ContainerTransientTest : Test {};

TEST_F(ContainerTransientTest, creates_new_shared_ptr_per_resolve) {
  struct TransientBound {};
  auto sut = Container{bind<TransientBound>().in<scope::Transient>()};

  auto shared1 = sut.template resolve<std::shared_ptr<TransientBound>>();
  auto shared2 = sut.template resolve<std::shared_ptr<TransientBound>>();

  EXPECT_NE(shared1.get(), shared2.get());  // Different instances
}

// ----------------------------------------------------------------------------
// Instance Scope Tests
// ----------------------------------------------------------------------------

struct ContainerInstanceTest : Test {};

TEST_F(ContainerInstanceTest, shared_ptr_wraps_external_instance) {
  struct External {
    int value = 42;
  };

  External external_obj;

  auto sut = Container{bind<External>().to(external_obj)};

  // shared_ptr should wrap the external instance
  auto shared1 = sut.template resolve<std::shared_ptr<External>>();
  auto shared2 = sut.template resolve<std::shared_ptr<External>>();

  EXPECT_EQ(&external_obj, shared1.get());  // Points to external
  EXPECT_EQ(shared1.get(), shared2.get());  // Same cached shared_ptr
  EXPECT_EQ(3, shared1.use_count());        // canonical + shared1 + shared2

  // Reference to the external instance
  auto& ref = sut.template resolve<External&>();
  EXPECT_EQ(&external_obj, &ref);
  EXPECT_EQ(&ref, shared1.get());
}

TEST_F(ContainerInstanceTest, canonical_shared_ptr_reference) {
  struct External {};
  External external_obj;

  auto sut = Container{bind<External>().to(external_obj)};

  auto& canonical1 = sut.template resolve<std::shared_ptr<External>&>();
  auto& canonical2 = sut.template resolve<std::shared_ptr<External>&>();

  EXPECT_EQ(&canonical1, &canonical2);         // Same cached shared_ptr
  EXPECT_EQ(&external_obj, canonical1.get());  // Wraps external
  EXPECT_EQ(1, canonical1.use_count());        // Only canonical exists
}

TEST_F(ContainerInstanceTest, weak_ptr_tracks_external_instance) {
  struct External {};
  External external_obj;

  auto sut = Container{bind<External>().to(external_obj)};

  auto weak = sut.template resolve<std::weak_ptr<External>>();

  EXPECT_FALSE(weak.expired());
  EXPECT_EQ(&external_obj, weak.lock().get());
}

TEST_F(ContainerInstanceTest, weak_ptr_does_not_expire_while_instance_alive) {
  struct External {};
  External external_obj;

  auto container = Container{bind<External>().to(external_obj)};

  auto weak = container.template resolve<std::weak_ptr<External>>();

  // Even with no shared_ptr in scope, weak_ptr should not expire
  // because it tracks the canonical shared_ptr which aliases the static
  EXPECT_FALSE(weak.expired());

  auto shared = weak.lock();
  EXPECT_NE(nullptr, shared);
}

TEST_F(ContainerInstanceTest, weak_ptr_expires_with_canonical_shared_ptr) {
  struct External {};
  External external_obj;

  auto container = Container{bind<External>().to(external_obj)};

  // resolve reference directly to canonical shared_ptr
  auto& canonical_shared_ptr =
      container.template resolve<std::shared_ptr<External>&>();
  const auto weak = container.template resolve<std::weak_ptr<External>>();

  EXPECT_FALSE(weak.expired());
  canonical_shared_ptr.reset();
  EXPECT_TRUE(weak.expired());
}

// ----------------------------------------------------------------------------
// Mixed Scopes Tests
// ----------------------------------------------------------------------------

struct ContainerMixedScopesTest : Test {};

TEST_F(ContainerMixedScopesTest, transient_and_singleton_coexist) {
  struct Transient {};
  struct Singleton {};

  auto sut = Container{bind<Transient>().in<scope::Transient>(),
                       bind<Singleton>().in<scope::Singleton>()};

  auto t1 = sut.template resolve<std::shared_ptr<Transient>>();
  auto t2 = sut.template resolve<std::shared_ptr<Transient>>();
  EXPECT_NE(t1.get(), t2.get());

  auto s1 = sut.template resolve<std::shared_ptr<Singleton>>();
  auto s2 = sut.template resolve<std::shared_ptr<Singleton>>();
  EXPECT_EQ(s1.get(), s2.get());
}

// ----------------------------------------------------------------------------
// Default Scope Tests
// ----------------------------------------------------------------------------

struct ContainerDefaultScopeTest : Test {};

TEST_F(ContainerDefaultScopeTest, unbound_type_uses_default_scope) {
  struct SingletonBound {};
  struct Unbound {};
  auto sut = Container{bind<SingletonBound>()};

  [[maybe_unused]] auto instance = sut.template resolve<Unbound>();
  // Should work - uses default Deduced scope
}

}  // namespace dink
