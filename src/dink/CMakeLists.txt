# \file
# copyright (c) 2025 Frank Secilia

# -----------------------------------------------------------------------------
# project files
# -----------------------------------------------------------------------------

list(APPEND dink_library_files
  config.template.hpp
  lib.hpp
  version.hpp
)

list(APPEND dink_test_files
  test.hpp
  version_test.cpp
)

# -----------------------------------------------------------------------------
# generated configuration
# -----------------------------------------------------------------------------

set(dink_config_template_file config.template.hpp)
set(dink_config_gen_filename config.gen.hpp)
set(dink_config_gen_root "${CMAKE_BINARY_DIR}/gen")
set(dink_config_gen_file
  "${dink_config_gen_root}/dink/${dink_config_gen_filename}")
configure_file("${dink_config_template_file}" "${dink_config_gen_file}")

add_library(dink INTERFACE ${dink_library_files} "${dink_config_gen_file}")
target_include_directories(dink INTERFACE
  "$<BUILD_INTERFACE:${dink_config_gen_root}>"
  "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/src>"
)

if (dink_enable_testing)
    add_executable(dink_test ${dink_test_files})
    target_link_libraries(dink_test PUBLIC
        dink
        GTest::gmock_main
        GTest::gtest_main
        GTest::gmock
        GTest::gtest
    )
    dink_configure_test_target_warnings(dink_test)
    dink_enable_running_from_build_tree(dink_test)
    gtest_discover_tests(dink_test)
endif()
