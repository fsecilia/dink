/*!
    \file
    Copyright (C) 2025 Frank Secilia
*/

#include "container.hpp"
#include <dink/test.hpp>

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

    struct mock_delegate_t
    {
        virtual ~mock_delegate_t() = default;
    };
    StrictMock<mock_delegate_t> mock_delegate;

    struct delegate_t
    {
        mock_delegate_t* mock_delegate = nullptr;
    };
};

} // namespace
} // namespace dink
