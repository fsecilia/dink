/*!
    \file
    Copyright (C) 2025 Frank Secilia
*/

#pragma once

#include <dink/lib.hpp>
#include <utility>

namespace dink {

//! factory that forwards directly to ctors; this adapts direct ctor calls to the generic discoverable factory api
template <typename constructed_t>
class ctor_factory_t
{
public:
    template <typename... args_t>
    requires std::constructible_from<constructed_t, args_t...>
    constexpr auto operator()(args_t&&... args) const -> constructed_t
    {
        return constructed_t{std::forward<args_t>(args)...};
    }
};

} // namespace dink
