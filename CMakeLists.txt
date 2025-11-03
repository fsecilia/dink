# copyright (c) 2025 Frank Secilia
# SPDX-License-Identifier: MIT

cmake_minimum_required(VERSION 4.0.2)
project(dink VERSION 0.1.0)

# Suppress cmake's "Up-to-date" message for every installed file.
set(CMAKE_INSTALL_MESSAGE NEVER)

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
# Documentation
# -----------------------------------------------------------------------------

# find doxygen and optionally dot
find_package(Doxygen OPTIONAL_COMPONENTS dot QUIET)

if (Doxygen_FOUND)
  set(DOXYGEN_WARN_LOGFILE "${CMAKE_CURRENT_BINARY_DIR}/doxygen-warnings.log")

  # Specify paths.
  set(DOXYGEN_INPUT_DIR "${PROJECT_SOURCE_DIR}/src")
  set(DOXYGEN_OUTPUT_DIR "${CMAKE_CURRENT_BINARY_DIR}/doxygen")

  # Use dot, if found.
  if(DOXYGEN_HAVE_DOT)
    set(DOXYGEN_CALL_GRAPH "YES")
    set(DOXYGEN_CALLER_GRAPH "YES")
  else()
    set(DOXYGEN_CALL_GRAPH "NO")
    set(DOXYGEN_CALLER_GRAPH "NO")
  endif()

  # Generate the actual Doxygen config file from our template.
  configure_file(
    "${PROJECT_SOURCE_DIR}/Doxyfile.in"
    "${CMAKE_CURRENT_BINARY_DIR}/Doxyfile"
    @ONLY
  )

  # Add target named doc.
  doxygen_add_docs(
    doc
    [QUIET]
    ${DOXYGEN_INPUT_DIR}
    CONFIG_FILE "${CMAKE_CURRENT_BINARY_DIR}/Doxyfile"
    COMMENT "Generating API documentation with Doxygen."
  )

  # Add a pre-install step to build the 'doc' target This runs *during* the
  # install step, ensuring the 'doc' target is built before the
  # 'install(DIRECTORY...)' command runs.
  install(CODE "
    message(STATUS \"Building documentation before install...\")
    execute_process(
      COMMAND \"${CMAKE_COMMAND}\" --build \"${CMAKE_BINARY_DIR}\" --target doc
      WORKING_DIRECTORY \"${CMAKE_BINARY_DIR}\"
      RESULT_VARIABLE result
    )
    if(NOT result EQUAL 0)
      message(FATAL_ERROR \"Failed to build 'doc' target during install.\")
    endif()
  ")


  # Add doxygen output to install.
  install(
    DIRECTORY "${DOXYGEN_OUTPUT_DIR}/html"
    DESTINATION "share/doc/${PROJECT_NAME}"
  )
else()
  message(STATUS "Doxygen not found. Documentation will not be generated.")
endif()

# -----------------------------------------------------------------------------
# project subdirectories
# -----------------------------------------------------------------------------

add_subdirectory(src/dink)
