/*!
    \file
    Copyright (C) 2025 Frank Secilia
*/

#include <dink/lib.hpp>
#include <stdexcept>

namespace dink {

//! catch-all base exception type for all dink-specific exceptions
struct dink_x : std::runtime_error
{
    using std::runtime_error::runtime_error;
};

} // namespace dink
