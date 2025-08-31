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
    factory_resolvable.hpp
    has_static_construct_method.hpp
    lib.hpp
    memory/append_only_heap_allocator.hpp
    memory/array_allocator.hpp
    memory/heap_allocator.hpp
    memory/page_allocator.hpp
    memory/owned_buffer.hpp
    memory/page.hpp
    memory/page_buffer_source.hpp
    memory/page_factory.hpp
    memory/page_size.hpp
    memory/paged_arena_allocator.hpp
    resolver.hpp
    scope.hpp
    strong_type.hpp
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
    has_static_construct_method_unit_test.cpp
    memory/append_only_heap_allocator_unit_test.cpp
    memory/page_allocator_unit_test.cpp
    memory/page_factory_unit_test.cpp
    memory/page_size_unit_test.cpp
    memory/page_unit_test.cpp
    memory/paged_arena_allocator_unit_test.cpp
    resolver_unit_test.cpp
    scope_unit_test.cpp
    strong_type_unit_test.cpp
    test.hpp
    type_map_unit_test.cpp
    version_unit_test.cpp
)

if (NOT DEFINED dink_max_deduced_params)
    set(dink_max_deduced_params 10 CACHE STRING "largest number of arguments dink will try to deduce")
endif()

set(config_generated_root "${PROJECT_BINARY_DIR}/generated")
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
