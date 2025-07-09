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
};

} // namespace dink
