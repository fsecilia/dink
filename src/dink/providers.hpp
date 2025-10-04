/*!
    \file
    Copyright (C) 2025 Frank Secilia
*/

#pragma once

#include <dink/lib.hpp>
#include <utility>

namespace dink::providers {

template <typename factory_p>
class factory_t
{
public:
    using provided_t = decltype(std::declval<factory_p>()());

    template <typename resolver_t>
    constexpr auto operator()(resolver_t& resolver) const -> provided_t
    {
        return resolver.construct_from_factory(factory);
    }

    explicit factory_t(factory_p factory) noexcept : factory{std::move(factory)} {}

private:
    factory_p factory;
};

} // namespace dink::providers
