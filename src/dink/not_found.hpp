/*!
    \file
    Copyright (C) 2025 Frank Secilia
*/

#pragma once

#include <dink/lib.hpp>

namespace dink {

struct not_found_t {};
inline static constexpr auto not_found = not_found_t{};

inline static constexpr auto npos = std::size_t(-1);

}  // namespace dink
