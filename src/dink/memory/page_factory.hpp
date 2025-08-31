/*!
    \file
    Copyright (C) 2025 Frank Secilia
*/

#pragma once

#include <dink/lib.hpp>
#include <dink/memory/page.hpp>
#include <dink/memory/page_buffer_source.hpp>
#include <concepts>
#include <utility>

namespace dink {

//! callable that returns pages
template <typename page_factory_t, typename page_t>
concept page_factory = requires(page_factory_t page_factory) {
    { page_factory() } -> std::same_as<page_t>;
};

//! creates pages dynamically from a buffer source
template <page page_t, page_buffer_source buffer_source_t>
class page_factory_t
{
public:
    auto operator()() const -> page_t { return page_t{buffer_source_()}; }

    explicit page_factory_t(buffer_source_t buffer_source) noexcept : buffer_source_{std::move(buffer_source)} {}

private:
    buffer_source_t buffer_source_;
};

} // namespace dink
