/*!
    \file
    Copyright (C) 2025 Frank Secilia
*/

#pragma once

#include <dink/lib.hpp>
#include <dink/provider.hpp>
#include <dink/scope.hpp>
#include <dink/smart_pointer_traits.hpp>
#include <memory>
#include <type_traits>
#include <utility>

namespace dink {

// =====================================================================================================================
// Cache Traits - instance-based, parameterized on request_t
// =====================================================================================================================

// base template for value types
template <typename request_p>
class cache_adapter_t {
public:
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
class cache_adapter_t<std::shared_ptr<T>> {
public:
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
class cache_adapter_t<std::weak_ptr<T>> : public cache_adapter_t<std::shared_ptr<T>> {};

}  // namespace dink
