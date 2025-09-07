#
# \file
# copyright (C) 2025 Frank Secilia
#

cmake_minimum_required(VERSION 4.0.2)
project(dink)

if (NOT DEFINED dink_enable_testing)
    set(dink_enable_testing TRUE CACHE BOOL "include tests and a dependency on gtest in the build")
endif()

include(deps/groundwork/groundwork.cmake)

add_subdirectory(deps/groundwork)
groundwork_find_ccache()
groundwork_find_threads()
groundwork_configure_compilers()

add_subdirectory(src)
