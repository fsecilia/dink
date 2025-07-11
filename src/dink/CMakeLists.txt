#
# \file
# copyright (C) 2025 Frank Secilia
#

set(dink_major 0)
set(dink_minor 0)
set(dink_patch 1)

list(APPEND dink_library_files
    arg.hpp
    bindings.hpp
    composer.hpp
    dink.hpp
    dispatcher.hpp
    factory.hpp
    lib.hpp
    resolver.hpp
    type_map.hpp
    version.hpp
)

list(APPEND dink_unit_test_files
    arg_unit_test.cpp
    bindings_unit_test.cpp
    composer_unit_test.cpp
    dink_unit_test.cpp
    dispatcher_unit_test.cpp
    factory_unit_test.cpp
    resolver_unit_test.cpp
    test.hpp
    type_map_unit_test.cpp
    version_unit_test.cpp
)

if (NOT dink_max_deduced_params)
    set(dink_max_deduced_params 10 CACHE STRING "largest number of arguments dink will try to deduce")
endif()

set(config_template_file config.template.hpp)
set(config_generated_root "${PROJECT_BINARY_DIR}/generated")
set(config_generated_filename config.generated.hpp)
add_library(dink INTERFACE
    ${config_template_file}
    ${config_generated_file}
    ${dink_library_files}
)
target_include_directories(dink INTERFACE "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/src>")
groundwork_install_files(dink "${CMAKE_INSTALL_INCLUDEDIR}" ${dink_library_files})
groundwork_configure_target(dink
    ${dink_major} ${dink_minor} ${dink_patch}
    "${config_template_file}" "${config_generated_root}" "${config_generated_filename}"
)

if (dink_enable_testing)
    add_executable(dink_unit_test ${dink_unit_test_files})
    target_link_libraries(dink_unit_test PUBLIC
        dink
        groundwork
        GTest::gmock_main
        GTest::gtest_main
        GTest::gmock
        GTest::gtest
    )
    groundwork_set_target_version(dink_unit_test ${dink_major} ${dink_minor} ${dink_patch})
    groundwork_enable_running_from_build_tree(dink_unit_test)
    gtest_discover_tests(dink_unit_test)

    add_executable(dink_integration_test dink_integration_test.cpp)
    target_link_libraries(dink_integration_test PUBLIC
        dink
        groundwork
        GTest::gmock_main
        GTest::gtest_main
        GTest::gmock
        GTest::gtest
    )
    groundwork_set_target_version(dink_integration_test ${dink_major} ${dink_minor} ${dink_patch})
    groundwork_enable_running_from_build_tree(dink_integration_test)
    gtest_discover_tests(dink_integration_test)
endif()
