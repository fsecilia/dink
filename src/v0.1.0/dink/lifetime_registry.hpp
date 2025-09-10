/*!
    \file
    Copyright (C) 2025 Frank Secilia
*/

#pragma once

#include <dink/lib.hpp>
#include <dink/exceptions.hpp>
#include <dink/lifetime.hpp>
#include <typeindex>
#include <unordered_map>

namespace dink {

struct lifetime_mismatch_x : dink_x
{
    using dink_x::dink_x;

    template <typename resolved_t>
    static auto emit(lifetime_t) -> void
    {
        // this will eventually need to get the type name in a portable way
        throw lifetime_mismatch_x{""};
    }
};

/*!
    tracks and enforces the "first use locks lifetime" rule to ensure consistent object lifetimes
    
    This type is used to prevent cases where a user initially resolves a type with one lifetime, then later tries to
    resolve it with a different lifetime. That can lead to a whole class of bugs where state changes in one part of the
    application don't appear in another, or do appear unexpectedly, which are difficult to track down.
*/
class lifetime_registry_t
{
public:
    template <typename resolved_t>
    auto ensure(lifetime_t lifetime) -> void
    {
        auto const key = std::type_index{typeid(resolved_t)};
        auto const [element, inserted] = lifetimes_by_type_.try_emplace(key, lifetime);
        auto const mismatch = !inserted && element->second != lifetime;
        if (mismatch) lifetime_mismatch_x::template emit<resolved_t>(lifetime);
    }

private:
    std::unordered_map<std::type_index, lifetime_t> lifetimes_by_type_{};
};

} // namespace dink
