/*
  Copyright (c) 2025 Frank Secilia \n
  SPDX-License-Identifier: MIT
*/

#include "integration_test.hpp"

namespace dink {

int_t IntegrationTest::Counted::num_instances = 0;

IntegrationTest::IService::~IService() = default;

IntegrationTest::~IntegrationTest() = default;

}  // namespace dink
