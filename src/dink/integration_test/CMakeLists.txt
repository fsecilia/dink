# copyright (c) 2025 Frank Secilia
# SPDX-License-Identifier: MIT

# -----------------------------------------------------------------------------
# Project Files
# -----------------------------------------------------------------------------

list(APPEND dink_integration_test_files
  composition.cpp
  hierarchy.cpp
  integration_test.cpp
  integration_test.hpp
  multiple_containers.cpp
  promotion.cpp
  scopes.cpp
)

# -----------------------------------------------------------------------------
# Test Projects
# -----------------------------------------------------------------------------

if (dink_enable_testing)
  add_executable(dink_integration_test ${dink_integration_test_files})
  target_link_libraries(dink_integration_test PUBLIC
    dink
    GTest::gmock_main
    GTest::gtest_main
    GTest::gmock
    GTest::gtest
  )
  dink_configure_test_target_warnings(dink_integration_test)
  dink_enable_running_from_build_tree(dink_integration_test)
  gtest_discover_tests(dink_integration_test)
endif()
