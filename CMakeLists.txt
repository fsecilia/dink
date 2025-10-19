#
# \file
# copyright (c) 2025 Frank Secilia
#

cmake_minimum_required(VERSION 4.0.2)
project(dink)

# -----------------------------------------------------------------------------
# version
# -----------------------------------------------------------------------------

set(dink_version_major 0)
set(dink_version_minor 1)
set(dink_version_patch 0)
set(dink_version
    ${dink_version_major}.${dink_version_minor}.${dink_version_patch})

# -----------------------------------------------------------------------------
# ccache
# -----------------------------------------------------------------------------

# use ccache, if available
find_program(ccache ccache)
if (ccache)
  set(CMAKE_C_COMPILER_LAUNCHER "${ccache}")
  set(CMAKE_CXX_COMPILER_LAUNCHER "${ccache}")
  set(CMAKE_C_LINKER_LAUNCHER "${ccache}")
  set(CMAKE_CXX_LINKER_LAUNCHER "${ccache}")
endif()

# -----------------------------------------------------------------------------
# dependencies
# -----------------------------------------------------------------------------

# find system threading library
set(THREADS_PREFER_PTHREAD_FLAG True)
find_package(Threads REQUIRED)

# find googletest
find_package(GTest)
if (GTEST_FOUND)
  set(dink_enable_testing True)
endif()

# -----------------------------------------------------------------------------
# enable warnings
# -----------------------------------------------------------------------------

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

# -----------------------------------------------------------------------------
# compiler settings
# -----------------------------------------------------------------------------

set(CMAKE_CXX_EXTENSIONS FALSE)
set(CMAKE_CXX_SCAN_FOR_MODULES FALSE)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)
set(CMAKE_CXX_VISIBILITY_PRESET hidden)
set(CMAKE_INTERPROCEDURAL_OPTIMIZATION_RELEASE TRUE)
set(CMAKE_POSITION_INDEPENDENT_CODE TRUE)
set(CMAKE_VISIBILITY_INLINES_HIDDEN TRUE)

if (CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
  add_compile_options(-fstrict-aliasing -fsized-deallocation)
endif()

if (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-fsafe-buffer-usage-suggestions)
endif()

if (CMAKE_CXX_COMPILER_ID MATCHES "GNU")
  add_compile_options(-fconcepts-diagnostics-depth=10)
endif()

if (CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
  add_compile_options(/utf-8 "$<$<CONFIG:RELEASE>:/GS;/Gy;/Zc:inline>")
  add_link_options("$<$<CONFIG:RELEASE>:/LTCG;/OPT:ICF;/OPT:REF>")
endif()
