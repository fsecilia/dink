# copyright (c) 2025 Frank Secilia
# SPDX-License-Identifier: MIT

# -----------------------------------------------------------------------------
# project files
# -----------------------------------------------------------------------------

list(APPEND dink_library_files
  lib.hpp
  type_list.hpp
  version.hpp
)

list(APPEND dink_test_files
  test.hpp
  type_list_test.cpp
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

# -----------------------------------------------------------------------------
# project definition
# -----------------------------------------------------------------------------

add_library(dink INTERFACE
  ${dink_library_files}
  "${dink_config_gen_file}"
  config.template.hpp
)
target_include_directories(dink INTERFACE
  "$<BUILD_INTERFACE:${dink_config_gen_root}>"
  "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/src>"
)
set_target_properties(dink
  PROPERTIES
  VERSION ${dink_major} ${dink_minor} ${dink_patch}
  SOVERSION ${dink_major} ${dink_minor}
)

# -----------------------------------------------------------------------------
# installation
# -----------------------------------------------------------------------------

include(GNUInstallDirs)

# Install library and headers.
# -----------------------------------------------------------------------------

# Destination for public headers (e.g., /usr/local/include/dink)
set(dink_header_install_root "${CMAKE_INSTALL_INCLUDEDIR}/dink")

# Add headers to install to project property PUBLIC_HEADER.
set_target_properties(dink PROPERTIES PUBLIC_HEADER "${dink_library_files}")

# Install library and source header components.
install(TARGETS dink
  # associate installed items with an export set named "dink"
  EXPORT dink

  # install static library, if configured
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}

  # install .so, if configured
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}

  # install .dll, if configured
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}

  # install public headers
  PUBLIC_HEADER DESTINATION ${dink_header_install_root}
)

# Install generated config file.
install(FILES "${dink_config_gen_file}"
  DESTINATION "${dink_header_install_root}"
)

# Install CMake package files.
# -----------------------------------------------------------------------------

include(CMakePackageConfigHelpers)

# Destination for CMake config files (e.g., /usr/local/lib/cmake/dink)
set(dink_cmake_install_root "${CMAKE_INSTALL_LIBDIR}/cmake/dink")

# Generate and install "dink-targets.cmake". This file contains the install
# paths for the library and headers previously specified by export set "dink".
install(EXPORT dink
  NAMESPACE dink::
  DESTINATION "${dink_cmake_install_root}"
  FILE dink-targets.cmake
)

# Destination file to let find_package() check the project's configured
# version.
set(dink_cmake_version_file "${PROJECT_BINARY_DIR}/dink-config-version.cmake")

# Generate dink_cmake_version_file.
write_basic_package_version_file(
  "${dink_cmake_version_file}"
  VERSION ${dink_version}
  COMPATIBILITY AnyNewerVersion
)

# Install dink_cmake_version_file.
install(
  FILES "${dink_cmake_version_file}"
  DESTINATION "${dink_cmake_install_root}"

  # This literal filename is required by cmake.
  RENAME "dink-config-version.cmake"
)

# Generate dink-config.cmake from dink_config.gen.cmake
configure_package_config_file(
  "dink-config.template.cmake"
  "${PROJECT_BINARY_DIR}/dink-config.cmake"
  INSTALL_DESTINATION "${dink_cmake_install_root}"
)

# Install dink-config.cmake.
install(
  FILES "${PROJECT_BINARY_DIR}/dink-config.cmake"
  DESTINATION "${dink_cmake_install_root}"
)

# -----------------------------------------------------------------------------
# test projects
# -----------------------------------------------------------------------------

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

  add_executable(dink_integration_test dink_integration_test.cpp)
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
