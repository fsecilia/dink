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
