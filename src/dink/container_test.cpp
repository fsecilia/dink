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
  const auto result1 = sut.template resolve<std::shared_ptr<SingletonBound>&>();
  const auto result2 = sut.template resolve<std::shared_ptr<SingletonBound>&>();
  ASSERT_EQ(&result1, &result2);
  ASSERT_EQ(result1.use_count(), result2.use_count());
  ASSERT_EQ(result1.use_count(), 1);
}

}  // namespace dink
