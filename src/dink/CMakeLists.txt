#
# \file
# copyright (C) 2025 Frank Secilia
#

set(dink_major 0)
set(dink_minor 1)
set(dink_patch 0)

list(APPEND dink_library_files
    ctor.hpp
    dink.hpp
    exceptions.hpp
    lib.hpp
    meta.hpp
    type_list.hpp
    version.hpp
)

list(APPEND dink_unit_test_files
    ctor_test.cpp
    type_list_test.cpp
    version_test.cpp
)

if (NOT DEFINED dink_max_deduced_arity)
    set(dink_max_deduced_arity 10 CACHE STRING "max arity dink will try to deduce when resolving constructor args")
endif()

set(config_generated_root "${CMAKE_BINARY_DIR}/generated")
set(config_generated_filename config.generated.hpp)
set(config_template_file config.template.hpp)
add_library(dink INTERFACE ${dink_library_files} "${config_template_file}")
target_include_directories(dink INTERFACE "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/src>")
groundwork_configure_library(dink INTERFACE ${dink_major} ${dink_minor} ${dink_patch}
    "${config_generated_root}" "${config_generated_filename}" "${config_template_file}")
groundwork_install_files(dink "${CMAKE_INSTALL_INCLUDEDIR}" ${dink_library_files})

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
    groundwork_enable_running_from_build_tree(dink_integration_test)
    gtest_discover_tests(dink_integration_test)
endif()
