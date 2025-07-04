#
# \file
# copyright (C) 2025 Frank Secilia
#

cmake_minimum_required(VERSION 4.0.2)
project(dink)

find_program(ccache ccache)
if (ccache)
    set_property(GLOBAL PROPERTY CMAKE_C_COMPILER_LAUNCHER "${ccache}")
    set_property(GLOBAL PROPERTY CMAKE_CXX_COMPILER_LAUNCHER "${ccache}")
    set_property(GLOBAL PROPERTY CMAKE_C_LINKER_LAUNCHER "${ccache}")
    set_property(GLOBAL PROPERTY CMAKE_CXX_LINKER_LAUNCHER "${ccache}")
endif()

set(THREADS_PREFER_PTHREAD_FLAG TRUE)
find_package(Threads REQUIRED)
