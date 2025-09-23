/*!
    \file
    Copyright (C) 2025 Frank Secilia
*/

#include <dink/lib.hpp>
#include <tuple>
#include <utility>

namespace dink::deleters {

//! applies each deleter, in order, to a given instance
template <typename instance_t, typename... deleters_t>
struct composite_t
{
    auto operator()(instance_t* instance) const noexcept -> void
    {
        if (!instance) return;
        std::apply([instance](auto const&... deleter) { (deleter(instance), ...); }, deleters);
    }

    composite_t(deleters_t... deleters) : deleters{std::move(deleters)...} {}

    [[no_unique_address]] std::tuple<deleters_t...> deleters;
};

//! destroys given instance
template <typename instance_t>
struct destroying_t
{
    auto operator()(instance_t* instance) const noexcept -> void { instance->~instance_t(); }
};

} // namespace dink::deleters
