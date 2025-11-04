/*
  Copyright (c) 2025 Frank Secilia
  SPDX-License-Identifier: MIT
*/

#include "cache.hpp"
#include <dink/test.hpp>

namespace dink::cache {
namespace {

struct CacheTest : Test {
  struct Container {};
  Container container{};

  struct Requested {};

  template <std::size_t id>
  struct UniqueProvider {
    using Provided = Requested;
    template <typename, typename Container>
    auto create(Container&) -> Provided {
      return Provided{};
    }
  };

  using Provider = UniqueProvider<0>;
  Provider provider{};

  using OtherProvider = UniqueProvider<1>;
  OtherProvider other_provider{};
};

// ----------------------------------------------------------------------------
// Type
// ----------------------------------------------------------------------------

struct CacheTypeTest : CacheTest {
  using Sut = Type;
  Sut sut{};
  Sut other_sut = Sut{};
};

TEST_F(CacheTypeTest, same_cache_same_provider_same_instance) {
  auto& instance1 = sut.get_or_create(container, provider);
  auto& instance2 = sut.get_or_create(container, provider);

  ASSERT_EQ(&instance1, &instance2);
}

TEST_F(CacheTypeTest, same_cache_different_provider_different_instance) {
  auto& instance1 = sut.get_or_create(container, provider);
  auto& instance2 = sut.get_or_create(container, other_provider);

  ASSERT_NE(&instance1, &instance2);
}

TEST_F(CacheTypeTest, different_cache_same_provider_same_instance) {
  auto& instance1 = sut.get_or_create(container, provider);
  auto& instance2 = other_sut.get_or_create(container, provider);

  ASSERT_EQ(&instance1, &instance2);
}

// ----------------------------------------------------------------------------
// Instance
// ----------------------------------------------------------------------------

struct CacheInstanceTest : CacheTest {
  using Sut = Instance;
  Sut sut{};
  Sut other_sut = Sut{};
};

TEST_F(CacheInstanceTest, same_cache_same_provider_same_instance) {
  auto& instance1 = sut.get_or_create(container, provider);
  auto& instance2 = sut.get_or_create(container, provider);

  ASSERT_EQ(&instance1, &instance2);
}

TEST_F(CacheInstanceTest, same_cache_different_provider_different_instance) {
  auto& instance1 = sut.get_or_create(container, provider);
  auto& instance2 = sut.get_or_create(container, other_provider);

  ASSERT_NE(&instance1, &instance2);
}

TEST_F(CacheInstanceTest, different_cache_same_provider_different_instance) {
  auto other_sut = Sut{};

  auto& instance1 = sut.get_or_create(container, provider);
  auto& instance2 = other_sut.get_or_create(container, provider);

  ASSERT_NE(&instance1, &instance2);
}

}  // namespace
}  // namespace dink::cache
