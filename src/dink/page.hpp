/*!
    \file
    Copyright (C) 2025 Frank Secilia
*/

#pragma once

#include <dink/memory.hpp>

#include <dink/lib.hpp>
#include <cstddef>
#include <memory>
#include <new>
#include <vector>

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

//! memory page for paged allocator
class page_t
{
public:
    //! allocates range from within page; returns nullptr if allocation doesn't fit
    auto try_allocate(std::size_t size, std::size_t alignment) -> void*
    {
        // find first aligned offset
        auto const aligned_begin = (cur_ + alignment - 1) & ~(alignment - 1);
        auto const end = aligned_begin + size;

        // make sure the allocation fits
        auto const fits = end <= end_;
        if (!fits) return nullptr;

        // commit allocation
        cur_ = end;
        return reinterpret_cast<void*>(aligned_begin);
    }

    page_t(owned_buffer_t&& buffer) noexcept
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

// allocates arrays using new
class array_allocator_t
{
public:
    using dynamic_array_t = std::unique_ptr<std::byte[]>;
    auto operator()(std::size_t size, std::align_val_t alignment) const -> dynamic_array_t
    {
        return dynamic_array_t{new (alignment) std::byte[size]};
    }
};

template <typename type_t>
concept array_allocator = requires(type_t const& instance, size_t size, std::align_val_t alignment) {
    { instance(size, alignment) } -> std::same_as<std::unique_ptr<std::byte[]>>;
};

//! provides owned buffers from the heap, aligned to and in power of two multiples of the os page size
template <array_allocator array_allocator_t, typename page_size_t>
class heap_page_buffer_source_t
{
public:
    static constexpr auto const pages_per_buffer = 16;

    auto alignment() const noexcept -> std::size_t { return alignment_; }
    auto size() const noexcept -> std::size_t { return size_; }

    auto operator()() const -> owned_buffer_t
    {
        return owned_buffer_t{array_allocator_(size_, std::align_val_t{alignment_}), size_};
    }

    explicit heap_page_buffer_source_t(array_allocator_t array_allocator, page_size_t os_page_size) noexcept
        : array_allocator_{std::move(array_allocator)}, alignment_{os_page_size()}, size_{alignment_ * pages_per_buffer}
    {}

private:
    [[no_unique_address]] array_allocator_t array_allocator_;
    std::size_t alignment_;
    std::size_t size_;
};

//! creates pages dynamically from a buffer source
template <typename buffer_source_t>
class page_factory_t
{
public:
    auto operator()() const -> page_t { return page_t{buffer_source_()}; }

    explicit page_factory_t(buffer_source_t buffer_source) noexcept : buffer_source_{std::move(buffer_source)} {}

private:
    buffer_source_t buffer_source_;
};

template <typename page_factory_t>
class paged_arena_t
{
public:
    auto try_allocate(std::size_t size, std::size_t alignment) -> void*
    {
        // try allocating from most recent page
        auto result = pages_.back().try_allocate(size, alignment);
        if (result) return result;

        // try allocating from new page
        auto new_page = page_factory_();
        result = new_page.try_allocate(size, alignment);
        if (!result)
        {
            // after alignment, size is too large to fit into a fresh page
            return nullptr;
        }

        // commit new page
        pages_.emplace_back(std::move(new_page));

        return result;
    }

    paged_arena_t(page_factory_t page_factory) noexcept : page_factory_{std::move(page_factory)}
    {
        pages_.emplace_back(page_factory_());
    }

private:
    page_factory_t page_factory_{};
    std::vector<page_t> pages_{};
};

} // namespace dink
