/*!
    \file
    Copyright (C) 2025 Frank Secilia
*/

#pragma once

#include <dink/lib.hpp>
#include <dink/memory/owned_buffer.hpp>
#include <concepts>
#include <cstddef>

namespace dink {

//! callable that returns owned buffers, has size() and alignment()
template <typename page_buffer_source_t>
concept page_buffer_source = requires(page_buffer_source_t page_buffer_source) {
    { page_buffer_source.size() } -> std::same_as<std::size_t>;
    { page_buffer_source.alignment() } -> std::same_as<std::size_t>;
    { page_buffer_source() } -> std::same_as<owned_buffer_t>;
};

} // namespace dink
