/*!
    \file
    Copyright (C) 2025 Frank Secilia
*/

#pragma once

#include <dink/lib.hpp>
#include <dink/cache_adapter.hpp>
#include <dink/request_traits.hpp>
#include <dink/resolver_strategy.hpp>
#include <utility>

namespace dink::resolver {

template <typename request_p, typename dependency_chain_p, lifetime_t min_lifetime_p>
struct policy_t {
    using request_t          = request_p;
    using dependency_chain_t = dependency_chain_p;

    inline static constexpr lifetime_t min_lifetime = min_lifetime_p;

    using cache_adapter_t    = cache_adapter_t<request_t>;
    using request_traits_t   = request_traits_t<request_t>;
    using strategy_factory_t = strategy::factory_t<request_t, dependency_chain_t, min_lifetime>;

    cache_adapter_t    cache_adapter;
    request_traits_t   request_traits;
    strategy_factory_t strategy_factory;
};

//! per-request resolution engine - encapsulates resolution logic for a specific request type
template <typename policy_t>
class resolver_t {
public:
    using request_t          = policy_t::request_t;
    using dependency_chain_t = policy_t::dependency_chain_t;
    using cache_adapter_t    = policy_t::cache_adapter_t;
    using request_traits_t   = policy_t::request_traits_t;
    using strategy_factory_t = policy_t::strategy_factory_t;

    inline static constexpr auto min_lifetime = policy_t::min_lifetime;

    explicit resolver_t(policy_t policy)
        : cache_adapter_{std::move(policy).cache_adapter},
          request_traits_{std::move(policy).request_traits},
          strategy_factory_{std::move(policy).strategy_factory} {}

    // searches for cached instance or binding in container hierarchy
    template <typename container_t, typename cache_t, typename config_t, typename parent_link_t,
              typename default_provider_factory_t>
    auto resolve(container_t& container, cache_t& cache, config_t& config, parent_link_t& parent_link,
                 default_provider_factory_t& default_provider_factory) -> as_returnable_t<request_t> {
        return resolve_hierarchically(
            cache, config, parent_link,
            [&](auto binding) -> as_returnable_t<request_t> { return resolve_with_binding(container, cache, binding); },
            [&]() -> as_returnable_t<request_t> {
                return resolve_without_binding(container, cache, default_provider_factory);
            });
    }

    // returns existing if in local cache, new if in bindings, or delegates to parent
    template <typename cache_t, typename config_t, typename parent_link_t>
    auto resolve_hierarchically(cache_t& cache, config_t& config, parent_link_t& parent_link, auto&& on_found,
                                auto&& on_not_found) -> as_returnable_t<request_t> {
        // check local cache
        if (auto cached = cache_adapter_.find(cache)) return request_traits_.from_lookup(cached);

        // check local binding
        auto local_binding = config.template find_binding<resolved_t<request_t>>();
        if constexpr (!std::is_same_v<decltype(local_binding), std::nullptr_t>) { return on_found(local_binding); }

        // recurse to parent via parent_link
        return parent_link.template find_in_parent<request_t>(*this, std::forward<decltype(on_found)>(on_found),
                                                              std::forward<decltype(on_not_found)>(on_not_found));
    }

private:
    template <typename container_t, typename cache_t, typename binding_t>
    auto resolve_with_binding(container_t& container, cache_t& cache, binding_t* binding)
        -> as_returnable_t<request_t> {
        auto const strategy = strategy_factory_.template create<binding_t>();
        return strategy.resolve(cache, cache_adapter_, binding->provider, request_traits_, container);
    }

    template <typename container_t, typename cache_t, typename default_provider_factory_t>
    auto resolve_without_binding(container_t& container, cache_t& cache,
                                 default_provider_factory_t& default_provider_factory) -> as_returnable_t<request_t> {
        auto       default_provider = default_provider_factory.template create<resolved_t<request_t>>();
        auto const strategy         = strategy_factory_.template create<std::nullptr_t>();
        return strategy.resolve(cache, cache_adapter_, default_provider, request_traits_, container);
    }

    cache_adapter_t    cache_adapter_;
    request_traits_t   request_traits_;
    strategy_factory_t strategy_factory_;
};

struct factory_t {
    template <typename request_t, typename dependency_chain_t, lifetime_t min_lifetime>
    auto create() {
        using policy_t = policy_t<request_t, dependency_chain_t, min_lifetime>;
        return resolver_t<policy_t>{policy_t{}};
    }
};

}  // namespace dink::resolver
