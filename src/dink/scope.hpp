/*!
    \file
    Copyright (C) 2025 Frank Secilia
*/

#pragma once

#include <dink/lib.hpp>

namespace dink {

/*!
    ordered weights to avoid captive dependencies

    Instances resolved with high stability cannot depend on instances resolved with low stability. stability_t weighs
    stability for different types of dependencies.
*/
enum class stability_t : std::size_t { transient, singleton };

namespace scope {

struct transient_t {};
struct singleton_t {};
struct default_t {};

}  // namespace scope
}  // namespace dink
