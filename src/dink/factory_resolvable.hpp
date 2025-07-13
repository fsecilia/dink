/*!
    \file
    Copyright (C) 2025 Frank Secilia
*/

#pragma once

#include <dink/lib.hpp>

namespace dink {

//! true when invoking factory with args produces a result convertible to resolved_t
template <typename resolved_t, typename factory_t, typename... args_t>
concept factory_resolvable = factory_t::template resolvable<args_t...>;

} //
