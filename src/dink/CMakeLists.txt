#
# \file
# copyright (C) 2025 Frank Secilia
#

add_library(dink INTERFACE
    dink.hpp
)
target_link_libraries(dink INTERFACE Threads::Threads)

if (dink_enable_testing)
    add_executable(dink_unit_test
        unit_test.cpp
    )

    target_link_libraries(dink_unit_test
        PUBLIC
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
