/*!
    \file
    Copyright (C) 2025 Frank Secilia
*/

#pragma once

#include <dink/lib.hpp>
#include <algorithm>
#include <cassert>
#include <concepts>
#include <cstddef>
#include <new>

namespace dink {

//! callable that returns owned buffers, has size() and alignment()
template <typename page_t>
concept page = requires(page_t page, size_t size, std::align_val_t alignment) {
    { page.try_allocate(size, alignment) } -> std::same_as<void*>;
};

/*!
    memory page for paged allocator

    page_t does not own the memory range it allocates from.
*/
class page_t
{
public:
    //! allocates range from within page; returns nullptr if allocation doesn't fit
    auto try_allocate(std::size_t size, std::align_val_t alignment_val) -> void*
    {
        auto const alignment = static_cast<std::size_t>(alignment_val);

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

    explicit page_t(void* begin, std::size_t size) noexcept
        : cur_{reinterpret_cast<uintptr_t>(begin)}, end_{cur_ + size}
    {}

    page_t(page_t const&) = delete;
    auto operator=(page_t const&) -> page_t& = delete;

    page_t(page_t&&) = default;
    auto operator=(page_t&&) -> page_t& = default;

private:
    uintptr_t cur_;
    uintptr_t end_;
};

} // namespace dink
