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
template <typename cache_adapter_t, typename request_adapter_t, typename strategy_factory_t,
          typename dependency_chain_t, stability_t stability, typename container_t>
class resolver_t {
public:
    using request_t = typename request_adapter_t::request_t;
    using cache_t   = typename container_t::cache_t;

    resolver_t(container_t& container, cache_t& cache)
        : container_{container}, cache_{cache}, cache_adapter_{}, request_adapter_{}, strategy_factory_{} {}

    auto resolve() -> as_returnable_t<request_t> {
        // check cache first
        if (auto cached = cache_adapter_.find(cache_)) { return request_adapter_.from_cached(cached); }

        // search for binding in container hierarchy
        return resolve_or_delegate(
            [&](auto binding) -> as_returnable_t<request_t> { return resolve_with_binding(binding); },
            [&]() -> as_returnable_t<request_t> { return resolve_without_binding(); });
    }

private:
    auto resolve_or_delegate(auto&& on_found, auto&& on_not_found) -> as_returnable_t<request_t> {
        // check local cache again (might have been populated by a parent)
        if (auto cached = cache_adapter_.find(cache_)) { return request_adapter_.from_cached(cached); }

        // check local binding
        auto local_binding = container_.config_.template find_binding<resolved_t<request_t>>();
        if constexpr (!std::is_same_v<decltype(local_binding), not_found_t>) { return on_found(local_binding); }

        // delegate to parent
        return container_.delegate_.template find_in_parent<request_t>(
            std::forward<decltype(on_found)>(on_found), std::forward<decltype(on_not_found)>(on_not_found));
    }

    auto resolve_with_binding(auto binding) -> as_returnable_t<request_t> {
        auto const strategy = strategy_factory_.create(binding);
        return strategy.resolve(cache_, cache_adapter_, binding->provider, request_adapter_, container_);
    }

    auto resolve_without_binding() -> as_returnable_t<request_t> {
        auto       default_provider = container_.default_provider_factory_.template create<resolved_t<request_t>>();
        auto const strategy         = strategy_factory_.create(not_found);
        return strategy.resolve(cache_, cache_adapter_, default_provider, request_adapter_, container_);
    }

    container_t&       container_;
    cache_t&           cache_;
    cache_adapter_t    cache_adapter_;
    request_adapter_t  request_adapter_;
    strategy_factory_t strategy_factory_;
};

}  // namespace dink
