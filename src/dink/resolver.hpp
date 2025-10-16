/*!
    \file
    Copyright (C) 2025 Frank Secilia
*/

#pragma once

#include <dink/lib.hpp>
#include <dink/cache_adapter.hpp>
#include <dink/not_found.hpp>
#include <dink/request_adapter.hpp>
#include <dink/resolution_strategy.hpp>
#include <utility>

namespace dink {

//! per-request resolution engine - encapsulates resolution logic for a specific request type
template <typename cache_traits_t, typename request_traits_t, typename dependency_chain_t, stability_t stability,
          typename container_t>
class resolver_t {
public:
    using request_t = typename request_traits_t::request_t;
    using cache_t   = typename container_t::cache_t;

    resolver_t(container_t& container, cache_t& cache)
        : container_{container}, cache_{cache}, cache_traits_{}, request_traits_{} {}

    auto resolve() -> as_returnable_t<request_t> {
        // check cache first
        if (auto cached = cache_traits_.find(cache_)) { return request_traits_.from_cached(cached); }

        // search for binding in container hierarchy
        return resolve_or_delegate(
            [&](auto binding) -> as_returnable_t<request_t> { return resolve_with_binding(binding); },
            [&]() -> as_returnable_t<request_t> { return resolve_without_binding(); });
    }

private:
    auto resolve_or_delegate(auto&& on_found, auto&& on_not_found) -> as_returnable_t<request_t> {
        // check local cache again (might have been populated by a parent)
        if (auto cached = cache_traits_.find(cache_)) { return request_traits_.from_cached(cached); }

        // check local binding
        auto local_binding = container_.config_.template find_binding<resolved_t<request_t>>();
        if constexpr (!std::is_same_v<decltype(local_binding), not_found_t>) { return on_found(local_binding); }

        // delegate to parent
        return container_.delegate_.template find_in_parent<request_t>(
            std::forward<decltype(on_found)>(on_found), std::forward<decltype(on_not_found)>(on_not_found));
    }

    auto resolve_with_binding(auto binding) -> as_returnable_t<request_t> {
        constexpr auto resolution = select_resolution<request_t, decltype(binding)>();
        auto           strategy   = resolution_strategy_t<request_t, dependency_chain_t, stability, resolution>{};
        return strategy.resolve(cache_, cache_traits_, binding->provider, request_traits_, container_);
    }

    auto resolve_without_binding() -> as_returnable_t<request_t> {
        constexpr auto resolution       = select_resolution<request_t, not_found_t>();
        auto           strategy         = resolution_strategy_t<request_t, dependency_chain_t, stability, resolution>{};
        auto           default_provider = container_.default_provider_factory_.template create<resolved_t<request_t>>();
        return strategy.resolve(cache_, cache_traits_, default_provider, request_traits_, container_);
    }

    container_t&     container_;
    cache_t&         cache_;
    cache_traits_t   cache_traits_;
    request_traits_t request_traits_;
};

}  // namespace dink
