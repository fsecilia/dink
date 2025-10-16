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

// forward declaration for resolved_t
template <typename requested_t>
struct request_traits_t;

template <typename requested_t>
using resolved_t = typename request_traits_t<requested_t>::value_type;

// =====================================================================================================================
// Cache Traits - instance-based, parameterized on request_t
// =====================================================================================================================

// base template for value types
template <typename request_p>
struct cache_traits_t {
    using request_t  = request_p;
    using value_type = resolved_t<request_t>;
    using key_type   = resolved_t<request_t>;

    template <typename cache_t>
    auto find(cache_t& cache) const -> decltype(auto) {
        return cache.template get_instance<value_type>();
    }

    template <typename provided_t, typename cache_t, typename factory_t>
    auto get_or_create(cache_t& cache, factory_t&& factory) const -> decltype(auto) {
        return cache.template get_or_create_instance<provided_t>(std::forward<factory_t>(factory));
    }
};

// specialization for shared_ptr<T>
template <typename T>
struct cache_traits_t<std::shared_ptr<T>> {
    using request_t  = std::shared_ptr<T>;
    using value_type = std::remove_cvref_t<T>;
    using key_type   = std::shared_ptr<T>;

    template <typename cache_t>
    auto find(cache_t& cache) const -> decltype(auto) {
        return cache.template get_shared<value_type>();
    }

    template <typename provided_t, typename cache_t, typename factory_t>
    auto get_or_create(cache_t& cache, factory_t&& factory) const -> decltype(auto) {
        return cache.template get_or_create_shared<provided_t>(std::forward<factory_t>(factory));
    }
};

// specialization for weak_ptr<T>
template <typename T>
struct cache_traits_t<std::weak_ptr<T>> : cache_traits_t<std::shared_ptr<T>> {};

template <typename request_t>
using cache_key_t = typename cache_traits_t<request_t>::key_type;

}  // namespace dink
