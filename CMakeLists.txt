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
