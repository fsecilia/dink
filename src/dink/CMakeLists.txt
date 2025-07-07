#
# \file
# copyright (C) 2025 Frank Secilia
#

list(APPEND dink_files
    arg.hpp
    lib.hpp
    composer.hpp
    dink.hpp
    dispatcher.hpp
    factory.hpp
    resolver.hpp
    type_map.hpp
)

list(APPEND dink_unit_test_files
    arg_unit_test.cpp
    composer_unit_test.cpp
    dispatcher_unit_test.cpp
    factory_unit_test.cpp
    resolver_unit_test.cpp
    type_map_unit_test.cpp
    test.hpp
)

set(dink_generated_root "${CMAKE_BINARY_DIR}/gen")
set(config_file_template config.template.hpp)
set(config_file_generated "${dink_generated_root}/dink/config.gen.hpp")
file(MAKE_DIRECTORY "${dink_generated_root}")
configure_file("${config_file_template}" "${config_file_generated}")

add_library(dink INTERFACE
    ${dink_files}
    ${config_file_template}
    ${config_file_generated}
)
target_include_directories(dink INTERFACE "$<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/src>")
target_link_libraries(dink INTERFACE Threads::Threads)
set_target_properties(dink
    PROPERTIES
    VERSION ${dink_version}
    SOVERSION ${dink_so_version}
)

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)
install(FILES ${dink_files} DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/dink")
install(TARGETS dink EXPORT dink DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/dink")
install(EXPORT dink NAMESPACE dink:: DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/dink")
write_basic_package_version_file(
    dinkConfigVersion.cmake
    VERSION ${dink_version}
    COMPATIBILITY AnyNewerVersion
    ARCH_INDEPENDENT
)

if (dink_enable_testing)
    add_executable(dink_unit_test ${dink_unit_test_files})
    set_target_properties(dink_unit_test
        PROPERTIES
        VERSION ${dink_version}
        SOVERSION ${dink_so_version}
    )

    target_link_libraries(dink_unit_test PUBLIC
        dink
        GTest::gmock_main
        GTest::gtest_main
        GTest::gmock
        GTest::gtest
    )

    enable_running_from_build_tree(dink_unit_test)

    if (dink_run_tests_after_build)
        add_custom_command(TARGET dink_unit_test
            POST_BUILD
            COMMAND ctest -C "$<CONFIGURATION>" --output-on-failure
        )
    endif()

    gtest_discover_tests(dink_unit_test)
endif()
