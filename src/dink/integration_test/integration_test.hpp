/*!
  \file
  \brief Common integration test fixture.

  \copyright Copyright (c) 2025 Frank Secilia \n
  SPDX-License-Identifier: MIT
*/

#pragma once

#include <dink/test.hpp>
#include <dink/binding.hpp>
#include <dink/binding_dsl.hpp>
#include <dink/cache.hpp>
#include <dink/container.hpp>
#include <dink/provider.hpp>
#include <dink/scope.hpp>

namespace dink {

struct IntegrationTest : Test {
  static inline const auto kInitialValue = int_t{7793};   // arbitrary
  static inline const auto kModifiedValue = int_t{2145};  // arbitrary

  // Base class for types that need instance counting.
  struct Counted {
    static int_t num_instances;
    int_t id;
    Counted() : id{num_instances++} {}
  };

  // Arbitrary type with a known initial value.
  struct Initialized : Counted {
    int_t value = kInitialValue;
    Initialized() = default;
  };

  // Base type for unique, local types used as singleton.
  //
  // This is a base class because types bound singleton and promoted requests
  // must be unique, local to the test, or the cached values will leak between
  // tests.
  struct Singleton : Initialized {};

  // Arbitrary type with given initial value.
  struct ValueInitialized : Counted {
    int_t value;
    explicit ValueInitialized(int_t value = 0) : value{value} {}
  };

  //
  struct Instance : ValueInitialized {
    using ValueInitialized::ValueInitialized;
  };

  // Arbitrary type used as the product of a factory.
  struct Product : ValueInitialized {
    using ValueInitialized::ValueInitialized;
  };

  // Arbitrary dependency passed as a ctor param to other types.
  struct Dependency : Initialized {};

  // Common dependencies.
  struct Dep1 : Initialized {
    Dep1() { value = 1; }
  };
  struct Dep2 : Initialized {
    Dep2() { value = 2; }
  };
  struct Dep3 : Initialized {
    Dep3() { value = 3; }
  };

  // Arbitrary common base class.
  struct IService {
    virtual ~IService();
    virtual auto get_value() const -> int_t = 0;
  };

  IntegrationTest() { Counted::num_instances = 0; }
  ~IntegrationTest() override;
};

}  // namespace dink
