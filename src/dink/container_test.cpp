// \file
// Copyright (c) 2025 Frank Secilia
// SPDX-License-Identifier: MIT

#include "container.hpp"
#include <dink/test.hpp>
#include <dink/scope.hpp>

namespace dink {

struct ContainerTest : Test {
  struct SingletonBound {};

  using SingletonBinding =
      Binding<SingletonBound, scope::Singleton<provider::Ctor<SingletonBound>>>;
  using Config = Config<SingletonBinding>;
  using Sut = Container<Config>;

  Sut sut = Sut{Config{scope::Singleton<provider::Ctor<SingletonBound>>{
      provider::Ctor<SingletonBound>{}}}};
};

TEST_F(ContainerTest, canonical_shared_wraps_instance) {
  const auto shared = sut.template resolve<std::shared_ptr<SingletonBound>>();
  auto& instance = sut.template resolve<SingletonBound&>();
  ASSERT_EQ(&instance, shared.get());
}

TEST_F(ContainerTest, canonical_shared_ptr_value) {
  const auto result1 = sut.template resolve<std::shared_ptr<SingletonBound>>();
  const auto result2 = sut.template resolve<std::shared_ptr<SingletonBound>>();
  ASSERT_EQ(result1, result2);
  ASSERT_EQ(result1.use_count(), result2.use_count());
  ASSERT_EQ(result1.use_count(), 3);  // result1 + result2 + canonical

  auto& instance = sut.template resolve<SingletonBound&>();
  ASSERT_EQ(&instance, result1.get());
}

TEST_F(ContainerTest, canonical_shared_ptr_identity) {
  const auto& result1 =
      sut.template resolve<std::shared_ptr<SingletonBound>&>();
  const auto& result2 =
      sut.template resolve<std::shared_ptr<SingletonBound>&>();
  ASSERT_EQ(&result1, &result2);
  ASSERT_EQ(result1.use_count(), result2.use_count());
  ASSERT_EQ(result1.use_count(), 1);
}

TEST_F(ContainerTest, weak_ptr_from_singleton) {
  auto weak1 = sut.template resolve<std::weak_ptr<SingletonBound>>();
  auto weak2 = sut.template resolve<std::weak_ptr<SingletonBound>>();

  EXPECT_FALSE(weak1.expired());
  EXPECT_EQ(weak1.lock(), weak2.lock());
}

TEST_F(ContainerTest, const_shared_ptr) {
  auto shared = sut.template resolve<std::shared_ptr<const SingletonBound>>();
  auto& instance = sut.template resolve<SingletonBound&>();

  EXPECT_EQ(&instance, shared.get());
}

TEST_F(ContainerTest, multiple_singleton_types) {
  struct A {};
  struct B {};

  auto container = Container{bind<A>().in<scope::Singleton>(),
                             bind<B>().in<scope::Singleton>()};

  auto shared_a = container.template resolve<std::shared_ptr<A>>();
  auto shared_b = container.template resolve<std::shared_ptr<B>>();

  EXPECT_NE(shared_a.get(), nullptr);
  EXPECT_NE(shared_b.get(), nullptr);
}

}  // namespace dink
