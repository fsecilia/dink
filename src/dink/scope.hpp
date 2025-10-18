/*!
    \file
    Copyright (C) 2025 Frank Secilia
*/

#pragma once

#include <dink/lib.hpp>

namespace dink {

/*!
    ordered weights to avoid captive dependencies

    Instances resolved with longer lifetime cannot depend on instances resolved with shorter lifetime. lifetime_t
    weighs lifetime for different types of dependencies.
*/
enum class lifetime_t : std::size_t { unconstrained, transient, singleton };

namespace scope {

struct transient_t {};
struct singleton_t {};
struct default_t {};

}  // namespace scope
}  // namespace dink
