/*!
    \file
    Copyright (C) 2025 Frank Secilia
*/

#pragma once

#include <dink/lib.hpp>
#include <groundwork/version.hpp>

namespace dink {

constexpr auto version() noexcept -> groundwork::version_t
{
    return groundwork::version_t{dink_version_major, dink_version_minor, dink_version_patch};
}

} // namespace dink
