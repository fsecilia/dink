/*!
    \file
    Copyright (C) 2025 Frank Secilia
*/

#pragma once

#include <dink/lib.hpp>
#include <cstddef>
#include <memory>

namespace dink {

//! move-only composition of an owning pointer and its size
struct owned_buffer_t
{
    using allocation_t = std::unique_ptr<std::byte[]>;

    allocation_t allocation = {};
    std::size_t size = {};

    owned_buffer_t(allocation_t&& allocation, std::size_t size) noexcept : allocation{std::move(allocation)}, size{size}
    {}

    owned_buffer_t(owned_buffer_t const&) = delete;
    auto operator=(owned_buffer_t const&) -> owned_buffer_t& = delete;

    owned_buffer_t(owned_buffer_t&&) = default;
    auto operator=(owned_buffer_t&&) -> owned_buffer_t& = default;
};

} // namespace dink
