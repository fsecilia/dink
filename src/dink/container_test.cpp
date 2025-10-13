/*!
    \file
    Copyright (C) 2025 Frank Secilia
*/

#include "container.hpp"
#include <dink/test.hpp>
#include <any>

namespace dink {
namespace {

struct container_test_t
{
    struct mock_cache_t
    {
        virtual ~mock_cache_t() = default;
    };
    StrictMock<mock_cache_t> mock_cache;

    struct cache_t
    {
        mock_cache_t* mock = nullptr;
    };

    struct mock_config_t
    {
        virtual ~mock_config_t() = default;
    };
    StrictMock<mock_config_t> mock_config;

    struct config_t
    {
        mock_config_t* mock = nullptr;
    };

    struct mock_delegate_t
    {
        virtual ~mock_delegate_t() = default;
    };
    StrictMock<mock_delegate_t> mock_delegate;

    struct delegate_t
    {
        mock_delegate_t* mock_delegate = nullptr;
    };

    struct mock_request_traits_t
    {
        // Type-erased mock methods
        MOCK_METHOD(std::any, find_in_cache, (std::any cache));
        MOCK_METHOD(std::any, resolve_from_cache, (std::any cache, std::any factory));
        MOCK_METHOD(std::any, as_requested, (std::any source));

        virtual ~mock_request_traits_t() = default;
    };

    struct request_traits_t
    {
        // Type-safe template wrappers
        template <typename request_t, typename cache_t>
        auto find_in_cache(cache_t& cache) noexcept -> request_t*
        {
            return std::any_cast<request_t*>(mock->find_in_cache(std::any{&cache}));
        }

        template <typename request_t, typename provided_t, typename cache_t, typename factory_t>
        auto resolve_from_cache(cache_t& cache, factory_t&& factory) -> provided_t&
        {
            return *std::any_cast<provided_t*>(mock->resolve_from_cache(std::any{&cache}, std::any{&factory}));
        }

        template <typename request_t, typename source_t>
        auto as_requested(source_t&& source) -> request_t
        {
            return *std::any_cast<request_t*>(mock->as_requested(std::any{&source}));
        }

        mock_request_traits_t* mock = nullptr;
    };
};

} // namespace
} // namespace dink
