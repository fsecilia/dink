#
# \file
# copyright (c) 2025 Frank Secilia
#

cmake_minimum_required(VERSION 4.0.2)
project(dink)

# use ccache, if available
find_program(ccache ccache)
if (ccache)
    set(CMAKE_C_COMPILER_LAUNCHER "${ccache}")
    set(CMAKE_CXX_COMPILER_LAUNCHER "${ccache}")
    set(CMAKE_C_LINKER_LAUNCHER "${ccache}")
    set(CMAKE_CXX_LINKER_LAUNCHER "${ccache}")
endif()

# find system threading library
set(THREADS_PREFER_PTHREAD_FLAG True)
find_package(Threads REQUIRED)

# find googletest
find_package(GTest)
if (GTEST_FOUND)
    set(dink_enable_testing True)
endif()

# enable warnings
if (CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    add_compile_options(
        -pedantic
        -Wall
        -Werror
        -Wextra
        -Wstrict-aliasing=2
        -Wswitch-enum
    )
endif()

if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    add_compile_options(
        -Weverything
    )
endif()

if (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    add_compile_options(-D_SCL_SECURE_NO_WARNINGS -D_CRT_SECURE_NO_WARNINGS
                        /WX /W4 /wd4201 /wd4250 /wd4251 /wd4275 /wd4458 /wd4459
                        /wd4576)
    add_link_options(/WX)
endif()
