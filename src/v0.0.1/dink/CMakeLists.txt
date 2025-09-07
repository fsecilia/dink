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
    config/binding.hpp
    config/flatten.hpp
    dink.hpp
    dispatcher.hpp
    factory.hpp
    factory_resolvable.hpp
    has_static_construct_method.hpp
    lib.hpp
    memory/append_only_heap_allocator.hpp
    memory/arena_allocator.hpp
    memory/heap_allocator.hpp
    memory/page.hpp
    memory/page_factory.hpp
    memory/page_size.hpp
    memory/paged_allocator.hpp
    memory/type_erased_storage.hpp
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
    config/binding_unit_test.cpp
    config/flatten_unit_test.cpp
    dink_unit_test.cpp
    dispatcher_unit_test.cpp
    factory_unit_test.cpp
    has_static_construct_method_unit_test.cpp
    memory/append_only_heap_allocator_unit_test.cpp
    memory/arena_allocator_unit_test.cpp
    memory/page_factory_unit_test.cpp
    memory/page_size_unit_test.cpp
    memory/page_unit_test.cpp
    memory/paged_allocator_unit_test.cpp
    memory/type_erased_storage_unit_test.cpp
    poc.cpp
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
set(config_generated_filenamxe config.generated.hpp)
set(config_template_file config.template.hpp)
add_library(dink_v001 INTERFACE ${dink_library_files} "${config_template_file}")
target_include_directories(dink_v001 INTERFACE "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/src/v0.0.1>")
groundwork_configure_library(dink_v001 INTERFACE ${dink_major} ${dink_minor} ${dink_patch}
    "${config_generated_root}" "${config_generated_filename}" "${config_template_file}")
groundwork_install_files(dink_v001 "${CMAKE_INSTALL_INCLUDEDIR}" ${dink_library_files})

if (dink_enable_testing)
    add_executable(dink_v001_unit_test ${dink_unit_test_files})
    target_link_libraries(dink_v001_unit_test PUBLIC
        dink_v001
        groundwork
        GTest::gmock_main
        GTest::gtest_main
        GTest::gmock
        GTest::gtest
    )
    groundwork_enable_running_from_build_tree(dink_v001_unit_test)
    gtest_discover_tests(dink_v001_unit_test)

    add_executable(dink_v001_integration_test dink_integration_test.cpp)
    target_link_libraries(dink_v001_integration_test PUBLIC
        dink_v001
        groundwork
        GTest::gmock_main
        GTest::gtest_main
        GTest::gmock
        GTest::gtest
    )
    groundwork_enable_running_from_build_tree(dink_v001_integration_test)
    gtest_discover_tests(dink_v001_integration_test)
endif()
