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

template <typename request_t, typename dependency_chain_t, stability_t stability>
struct resolver_policy_t {
    using cache_adapter_t    = cache_adapter_t<request_t>;
    using request_adapter_t  = request_adapter_t<request_t>;
    using strategy_factory_t = resolution::strategy_factory_t<request_t, dependency_chain_t, stability>;

    cache_adapter_t    cache_adapter;
    request_adapter_t  request_adapter;
    strategy_factory_t strategy_factory;
};

//! per-request resolution engine - encapsulates resolution logic for a specific request type
template <typename policy_t, typename request_t, typename dependency_chain_t, stability_t stability,
          typename container_t>
class resolver_t {
public:
    using cache_adapter_t    = policy_t::cache_adapter_t;
    using request_adapter_t  = policy_t::request_adapter_t;
    using strategy_factory_t = policy_t::strategy_factory_t;

    using cache_t = typename container_t::cache_t;

    resolver_t(policy_t policy, container_t& container, cache_t& cache)
        : container_{container},
          cache_{cache},
          cache_adapter_{std::move(policy).cache_adapter},
          request_adapter_{std::move(policy).request_adapter},
          strategy_factory_{std::move(policy).strategy_factory} {}

    auto resolve() -> as_returnable_t<request_t> {
        // search for binding in container hierarchy
        return resolve_or_delegate(
            [&](auto binding) -> as_returnable_t<request_t> { return resolve_with_binding(binding); },
            [&]() -> as_returnable_t<request_t> { return resolve_without_binding(); });
    }

private:
    auto resolve_or_delegate(auto&& on_found, auto&& on_not_found) -> as_returnable_t<request_t> {
        // check local cache
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
