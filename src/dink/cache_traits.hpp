/*!
    \file
    Copyright (C) 2025 Frank Secilia
*/

#pragma once

#include <dink/lib.hpp>
#include <dink/not_found.hpp>
#include <dink/provider.hpp>
#include <dink/scope.hpp>
#include <dink/smart_pointer_traits.hpp>
#include <memory>
#include <type_traits>
#include <utility>

namespace dink {

// cache_traits - what form to store/retrieve from cache
template <typename request_t>
struct cache_traits_f {
    using value_type = std::remove_cvref_t<request_t>;
    using key_type   = resolved_t<request_t>;

    template <typename cache_t>
    static auto find(cache_t& cache) -> auto {
        return cache.template get_instance<value_type>();
    }

    template <typename provided_t, typename cache_t, typename factory_t>
    static auto get_or_create(cache_t& cache, factory_t&& factory) -> decltype(auto) {
        return cache.template get_or_create_instance<provided_t>(std::forward<factory_t>(factory));
    }
};

// specialization for shared_ptr/weak_ptr
template <typename T>
struct cache_traits_f<std::shared_ptr<T>> {
    using value_type = std::remove_cvref_t<T>;
    using key_type   = std::shared_ptr<T>;

    template <typename cache_t>
    static auto find(cache_t& cache) -> auto {
        return cache.template get_shared<value_type>();
    }

    template <typename provided_t, typename cache_t, typename factory_t>
    static auto get_or_create(cache_t& cache, factory_t&& factory) -> decltype(auto) {
        return cache.template get_or_create_shared<provided_t>(std::forward<factory_t>(factory));
    }
};

template <typename T>
struct cache_traits_f<std::weak_ptr<T>> : cache_traits_f<std::shared_ptr<T>> {};

//! instance-based api adapter over cache_traits_t's static api
struct cache_traits_t {
    template <typename request_t, typename cache_t>
    auto find(cache_t& cache) noexcept -> auto {
        return cache_traits_f<request_t>::find(cache);
    }

    template <typename request_t, typename provided_t, typename cache_t, typename factory_t>
    auto get_or_create(cache_t& cache, factory_t&& factory) -> decltype(auto) {
        return cache_traits_f<request_t>::template get_or_create<provided_t>(cache, std::forward<factory_t>(factory));
    }
};

template <typename request_t>
using cache_key_t = typename cache_traits_f<request_t>::key_type;

}  // namespace dink
