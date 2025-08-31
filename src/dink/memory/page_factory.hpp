/*!
    \file
    Copyright (C) 2025 Frank Secilia
*/

#pragma once

#include <dink/lib.hpp>
#include <dink/memory/append_only_heap_allocator.hpp>
#include <dink/memory/page.hpp>
#include <dink/memory/page_size.hpp>
#include <concepts>
#include <cstddef>

namespace dink {

//! callable that returns page_t, has size() and alignment()
template <typename page_factory_t, typename page_t>
concept page_factory = requires(page_factory_t page_factory) {
    { page_factory.size() } -> std::same_as<std::size_t>;
    { page_factory.alignment() } -> std::same_as<std::size_t>;
    { page_factory() } -> std::same_as<page_t>;
};

//! provides pages sized and aligned to a power of two multiple of the os page size
template <page page_t, append_only_heap_allocator heap_allocator_t, page_size os_page_size_t>
class page_factory_t
{
public:
    //! power of two os page size multiplier
    static constexpr auto const pages_per_buffer = 16;

    //! all buffers produced have this size
    auto size() const noexcept -> std::size_t { return size_; }

    //! all buffers produced have this alignment
    auto alignment() const noexcept -> std::size_t { return alignment_; }

    //! acquires buffer from heap_allocator
    auto operator()() const -> page_t
    {
        return page_t{heap_allocator_.allocate(size_, std::align_val_t{alignment_}), size_};
    }

    page_factory_t(heap_allocator_t heap_allocator, os_page_size_t os_page_size) noexcept
        : page_factory_t{std::move(heap_allocator), os_page_size()}
    {}

private:
    [[no_unique_address]] heap_allocator_t heap_allocator_;
    std::size_t size_;
    std::size_t alignment_;

    page_factory_t(heap_allocator_t heap_allocator, size_t os_page_size) noexcept
        : heap_allocator_{std::move(heap_allocator)}, size_{os_page_size * pages_per_buffer}, alignment_{os_page_size}
    {}
};

} // namespace dink
