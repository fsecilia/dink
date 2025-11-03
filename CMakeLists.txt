# copyright (c) 2025 Frank Secilia
# SPDX-License-Identifier: MIT

cmake_minimum_required(VERSION 4.0.2)
project(dink VERSION 0.1.0)

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

    -Wno-c++20-compat
    -Wno-c++98-compat
    -Wno-c++98-compat-pedantic
    -Wno-documentation
    -Wno-exit-time-destructors
    -Wno-global-constructors
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
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)
set(CMAKE_CXX_VISIBILITY_PRESET hidden)
set(CMAKE_INTERPROCEDURAL_OPTIMIZATION_RELEASE TRUE)
set(CMAKE_POSITION_INDEPENDENT_CODE TRUE)
set(CMAKE_VISIBILITY_INLINES_HIDDEN TRUE)

if (CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
  add_compile_options(-fstrict-aliasing -fsized-deallocation)
endif()

if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
  add_compile_options(-fsafe-buffer-usage-suggestions)
endif()

if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
  add_compile_options(-fconcepts-diagnostics-depth=10)
endif()

if (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
  add_compile_options(/utf-8 "$<$<CONFIG:RELEASE>:/GS;/Gy;/Zc:inline>")
  add_link_options("$<$<CONFIG:RELEASE>:/LTCG;/OPT:ICF;/OPT:REF>")
endif()

# -----------------------------------------------------------------------------
# project functions
# -----------------------------------------------------------------------------

# sets up build tree for running an executable
#
# Shared objects must be located when running from the build tree. Most
# platforms use RPATH. Windows does not, but when locating dlls for an
# executable, it will search for the directory containing that executable. The
# cmake devs recommend placing output dlls next to the executable as a
# post-build step. Here, we rsync the files in a project's TARGET_RUNTIME_DLLS
# to its TARGET_FILE_DIR.
#
# This is only for running from the build tree, and only on Windows. It only
# affects dll files we build directly or indirectly through add_subdirectory(),
# FetchContent or ExternalProject.
function(dink_enable_running_from_build_tree target)
  if (CMAKE_SYSTEM_NAME MATCHES "Windows")
    add_custom_command(TARGET ${target}
      POST_BUILD
      COMMAND "${CMAKE_COMMAND}" -E echo copy_if_different
        "$<TARGET_RUNTIME_DLLS:${target}>" "$<TARGET_FILE_DIR:${target}>"
      COMMAND_EXPAND_LISTS
    )
  endif()
endfunction()

# disables a few warnings generated when using gtest
function(dink_configure_test_target_warnings target)
  if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    target_compile_options(${target} PUBLIC
      -Wno-ctad-maybe-unsupported
      -Wno-global-constructors
      -Wno-padded
      -Wno-shadow
      -Wno-shadow-field-in-constructor
      -Wno-unneeded-member-function
      -Wno-unused-member-function
      -Wno-unused-template
      -Wno-weak-vtables
    )
  endif()

  if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    target_compile_options(${target} PUBLIC
      -Wno-changes-meaning
      -Wno-missing-field-initializers
      -Wno-unused-local-typedefs
    )
  endif()
endfunction()

# -----------------------------------------------------------------------------
# project subdirectories
# -----------------------------------------------------------------------------

add_subdirectory(src/dink)
