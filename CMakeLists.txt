#
# \file
# copyright (C) 2025 Frank Secilia
#

cmake_minimum_required(VERSION 4.0.2)
project(dink)

set(dink_version_major 0)
set(dink_version_minor 0)
set(dink_version_patch 1)
set(dink_version ${dink_version_major}.${dink_version_minor}.${dink_version_patch})
set(dink_so_version ${dink_version_major})

find_program(ccache ccache)
if (ccache)
    set_property(GLOBAL PROPERTY CMAKE_C_COMPILER_LAUNCHER "${ccache}")
    set_property(GLOBAL PROPERTY CMAKE_CXX_COMPILER_LAUNCHER "${ccache}")
    set_property(GLOBAL PROPERTY CMAKE_C_LINKER_LAUNCHER "${ccache}")
    set_property(GLOBAL PROPERTY CMAKE_CXX_LINKER_LAUNCHER "${ccache}")
endif()

set(THREADS_PREFER_PTHREAD_FLAG TRUE)
find_package(Threads REQUIRED)

if (NOT DEFINED dink_enable_testing)
    set(dink_enable_testing TRUE CACHE BOOL "If true, include tests and a dependency on gtest in the build.")
endif()

if (dink_enable_testing)
    if (NOT DEFINED dink_fetch_gtest)
        set(dink_fetch_gtest TRUE CACHE BOOL "If true, satisfy gtest dependency using cmake's FetchContent, find_package otherwise.")
    endif()

    if (NOT DEFINED dink_run_tests_after_build)
        set(dink_run_tests_after_build TRUE CACHE BOOL "If true, automatically run tests after build.")
    endif()
endif()

if (dink_enable_testing)
    if (dink_fetch_gtest)
        include(FetchContent)
        FetchContent_Declare(
            googletest
            GIT_REPOSITORY https://github.com/google/googletest.git
            GIT_TAG v1.17.0
            GIT_PROGRESS TRUE
            SYSTEM
        )
        FetchContent_MakeAvailable(googletest)
        if (CMAKE_SYSTEM_NAME MATCHES "Windows")
            set(gtest_force_shared_crt ON CACHE BOOL "prevent googletest from overriding /MDd with /MTd." FORCE)
        endif()
        set(INSTALL_GTEST FALSE CACHE BOOL "remove fetched googletest from cmake install" FORCE)
        include(GoogleTest)
    else()
        find_package(GTest REQUIRED)
    endif()

    enable_testing()
endif()

set(CMAKE_CXX_EXTENSIONS FALSE)
set(CMAKE_CXX_SCAN_FOR_MODULES False)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)
set(CMAKE_CXX_VISIBILITY_PRESET hidden)
set(CMAKE_INTERPROCEDURAL_OPTIMIZATION_RELEASE TRUE)
set(CMAKE_POSITION_INDEPENDENT_CODE TRUE)
set(CMAKE_VISIBILITY_INLINES_HIDDEN TRUE)

if (CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    add_compile_options(
        -fstrict-aliasing
        -fsized-deallocation

        -pedantic

        -Wall
        -Werror
        -Wextra
        -Wstrict-aliasing=2
        -Wswitch-enum
        -Wno-invalid-offsetof
        -Wno-missing-braces
        -Wno-non-virtual-dtor
        -Wno-old-style-cast
        -Wno-sign-compare
    )
endif()

if (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    add_compile_options(
        -fsafe-buffer-usage-suggestions

        -Weverything
        -Wno-bad-function-cast
        -Wno-c++20-compat
        -Wno-c++98-compat
        -Wno-c++98-compat-pedantic
        -Wno-c99-extensions
        -Wno-covered-switch-default
        -Wno-ctad-maybe-unsupported
        -Wno-declaration-after-statement
        -Wno-documentation
        -Wno-exit-time-destructors
        -Wno-global-constructors
        -Wno-language-extension-token
        -Wno-missing-prototypes
        -Wno-padded
        -Wno-reserved-identifier
        -Wno-shadow
        -Wno-shadow-field
        -Wno-unused-function
        -Wno-unused-member-function
        -Wno-unused-parameter
        -Wno-unused-variable
        -Wno-weak-vtables
   )
endif()

if (CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
    add_compile_options(
        "$<$<CONFIG:RELEASE>:/GS;/Gy;/Zc:inline>"

        /WX /W4
        /wd4201 /wd4250 /wd4251 /wd4275 /wd4459 /wd4576
        -D_SCL_SECURE_NO_WARNINGS
        -D_CRT_SECURE_NO_WARNINGS
    )
    add_link_options(
        "$<$<CONFIG:RELEASE>:/LTCG;/OPT:ICF;/OPT:REF>"
        /WX
    )
endif()

function(enable_running_from_build_tree target)
    if (CMAKE_SYSTEM_NAME MATCHES "Windows")
        add_custom_command(TARGET ${target}
            POST_BUILD
            COMMAND "${CMAKE_COMMAND}" -E echo copy_if_different
                "$<TARGET_RUNTIME_DLLS:${target}>" "$<TARGET_FILE_DIR:${target}>"
            COMMAND_EXPAND_LISTS
        )
    endif()
endfunction()

add_subdirectory(src/dink)
