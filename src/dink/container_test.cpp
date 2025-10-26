// \file
// Copyright (c) 2025 Frank Secilia
// SPDX-License-Identifier: MIT

#include "container.hpp"
#include <dink/test.hpp>
#include <dink/scope.hpp>

namespace dink {

struct ContainerTest : Test {
};

TEST_F(ContainerTest, canonical_shared_wraps_instance) {
  struct SingletonBound {};
  auto sut = Container{bind<SingletonBound>().in<scope::Singleton>()};

  const auto shared = sut.template resolve<std::shared_ptr<SingletonBound>>();
  auto& instance = sut.template resolve<SingletonBound&>();
  ASSERT_EQ(&instance, shared.get());
}

TEST_F(ContainerTest, canonical_shared_ptr_value) {
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

TEST_F(ContainerTest, canonical_shared_ptr_identity) {
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

TEST_F(ContainerTest, weak_ptr_from_singleton) {
  struct SingletonBound {};
  auto sut = Container{bind<SingletonBound>().in<scope::Singleton>()};

  auto weak1 = sut.template resolve<std::weak_ptr<SingletonBound>>();
  auto weak2 = sut.template resolve<std::weak_ptr<SingletonBound>>();

  EXPECT_FALSE(weak1.expired());
  EXPECT_EQ(weak1.lock(), weak2.lock());
}

TEST_F(ContainerTest, const_shared_ptr) {
  struct SingletonBound {};
  auto sut = Container{bind<SingletonBound>().in<scope::Singleton>()};

  auto shared = sut.template resolve<std::shared_ptr<const SingletonBound>>();
  auto& instance = sut.template resolve<SingletonBound&>();

  EXPECT_EQ(&instance, shared.get());
}

TEST_F(ContainerTest, multiple_singleton_types) {
  struct SingletonBound {};

  struct A {};
  struct B {};

  auto sut = Container{bind<A>().in<scope::Singleton>(),
                       bind<B>().in<scope::Singleton>()};

  auto shared_a = sut.template resolve<std::shared_ptr<A>>();
  auto shared_b = sut.template resolve<std::shared_ptr<B>>();

  EXPECT_NE(shared_a.get(), nullptr);
  EXPECT_NE(shared_b.get(), nullptr);
}

// Transient scope should NOT cache shared_ptr
TEST_F(ContainerTest, transient_creates_new_shared_ptr) {
  struct SingletonBound {};
  auto sut = Container{bind<SingletonBound>().in<scope::Transient>()};

  auto shared1 = sut.template resolve<std::shared_ptr<SingletonBound>>();
  auto shared2 = sut.template resolve<std::shared_ptr<SingletonBound>>();

  EXPECT_NE(shared1.get(), shared2.get());  // Different instances
}

// Mixed scopes
TEST_F(ContainerTest, mixed_scopes) {
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

// Default scope (no binding)
TEST_F(ContainerTest, unbound_type_uses_default_scope) {
  struct SingletonBound {};
  struct Unbound {};
  auto sut = Container{bind<SingletonBound>()};

  [[maybe_unused]] auto instance = sut.template resolve<Unbound>();
  // Should work - uses default Deduced scope
}

TEST_F(ContainerTest, instance_scope_with_shared_ptr) {
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

TEST_F(ContainerTest, instance_scope_canonical_shared_ptr_reference) {
  struct External {};
  External external_obj;

  auto sut = Container{bind<External>().to(external_obj)};

  auto& canonical1 = sut.template resolve<std::shared_ptr<External>&>();
  auto& canonical2 = sut.template resolve<std::shared_ptr<External>&>();

  EXPECT_EQ(&canonical1, &canonical2);         // Same cached shared_ptr
  EXPECT_EQ(&external_obj, canonical1.get());  // Wraps external
  EXPECT_EQ(1, canonical1.use_count());        // Only canonical exists
}

TEST_F(ContainerTest, instance_scope_weak_ptr) {
  struct External {};
  External external_obj;

  auto sut = Container{bind<External>().to(external_obj)};

  auto weak = sut.template resolve<std::weak_ptr<External>>();

  EXPECT_FALSE(weak.expired());
  EXPECT_EQ(&external_obj, weak.lock().get());
}

}  // namespace dink
