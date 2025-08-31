/*!
    \file
    Copyright (C) 2025 Frank Secilia
*/

#pragma once

#include <dink/lib.hpp>
#include <dink/memory/array_allocator.hpp>
#include <dink/memory/owned_buffer.hpp>
#include <dink/memory/page_size.hpp>
#include <cstddef>

namespace dink {

//! provides heap buffers sized and aligned to a power of two multiple of the os page size
template <array_allocator array_allocator_t, page_size os_page_size_t>
class page_allocator_t
{
public:
    //! power of two os page size multiplier
    static constexpr auto const pages_per_buffer = 16;

    //! all buffers produced have this size
    auto size() const noexcept -> std::size_t { return size_; }

    //! all buffers produced have this alignment
    auto alignment() const noexcept -> std::size_t { return alignment_; }

    //! acquires buffer from array allocator
    auto operator()() const -> owned_buffer_t
    {
        return owned_buffer_t{array_allocator_(size_, std::align_val_t{alignment_}), size_};
    }

    page_allocator_t(array_allocator_t array_allocator, os_page_size_t os_page_size) noexcept
        : page_allocator_t{std::move(array_allocator), os_page_size()}
    {}

private:
    [[no_unique_address]] array_allocator_t array_allocator_;
    std::size_t size_;
    std::size_t alignment_;

    page_allocator_t(array_allocator_t array_allocator, size_t os_page_size) noexcept
        : array_allocator_{std::move(array_allocator)}, size_{os_page_size * pages_per_buffer}, alignment_{os_page_size}
    {}
};

} // namespace dink
