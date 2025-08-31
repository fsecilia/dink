/*!
    \file
    Copyright (C) 2025 Frank Secilia
*/

#pragma once

#include <dink/lib.hpp>
#include <dink/memory/owned_buffer.hpp>
#include <algorithm>
#include <cassert>
#include <concepts>
#include <cstddef>
#include <new>

namespace dink {

//! const callable that returns owned buffers, has size() and alignment()
template <typename page_t>
concept page = requires(page_t page, size_t size, std::align_val_t alignment) {
    { page.try_allocate(size, alignment) } -> std::same_as<void*>;
};

//! memory page for paged allocator
class page_t
{
public:
    //! allocates range from within page; returns nullptr if allocation doesn't fit
    auto try_allocate(std::size_t size, std::size_t alignment) -> void*
    {
        // alignment must be a nonzero power of two
        assert(alignment && !(alignment & (alignment - 1)));

        // treat empty requests as 1-byte requests so they still get unique addresses
        size = std::max(size, std::size_t{1});

        // find first aligned offset
        auto const aligned_begin = (cur_ + alignment - 1) & ~(alignment - 1);
        auto const aligned_end = aligned_begin + size;

        // make sure the allocation fits
        auto const fits = aligned_end <= end_;
        if (!fits) return nullptr;

        // commit allocation
        cur_ = aligned_end;
        return reinterpret_cast<void*>(aligned_begin);
    }

    explicit page_t(owned_buffer_t&& buffer) noexcept
        : allocation_{std::move(buffer.allocation)}, cur_{reinterpret_cast<uintptr_t>(allocation_.get())},
          end_{cur_ + buffer.size}
    {}

    page_t(page_t const&) = delete;
    auto operator=(page_t const&) -> page_t& = delete;

    page_t(page_t&&) = default;
    auto operator=(page_t&&) -> page_t& = default;

private:
    owned_buffer_t::allocation_t allocation_;
    uintptr_t cur_;
    uintptr_t end_;
};

} // namespace dink
