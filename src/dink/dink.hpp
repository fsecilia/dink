/*!
    \file
    Copyright (C) 2025 Frank Secilia
*/

#pragma once

#include <dink/lib.hpp>
#include <dink/arg.hpp>
#include <dink/composer.hpp>
#include <dink/dispatcher.hpp>
#include <dink/factory.hpp>
#include <dink/resolver.hpp>
#include <dink/type_map.hpp>
#include <dink/version.hpp>

namespace dink {

class dink_t : public composer_t<resolvers::transient_t<dispatcher_t, factory_t, arg_t>, resolvers::shared_t>
{
public:
    constexpr auto version() const noexcept -> version_t
    {
        return version_t{dink_version_major, dink_version_minor, dink_version_patch};
    }
};

} // namespace dink
