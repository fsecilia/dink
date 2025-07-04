#
# \file
# copyright (C) 2025 Frank Secilia
#

list(APPEND dink_files
    dink.hpp
)

add_library(dink INTERFACE ${dink_files})
target_include_directories(dink INTERFACE $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/src>)
target_link_libraries(dink INTERFACE Threads::Threads)

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)
install(FILES ${dink_files} DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/dink")
install(TARGETS dink EXPORT dink DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/dink")
install(EXPORT dink NAMESPACE dink:: DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/dink")
write_basic_package_version_file(
    "dinkConfigVersion.cmake"
    VERSION 0.0.1
    COMPATIBILITY AnyNewerVersion
    ARCH_INDEPENDENT
)

if (dink_enable_testing)
    add_executable(dink_unit_test
        unit_test.cpp
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
