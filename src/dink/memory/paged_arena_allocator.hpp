/*!
    \file
    Copyright (C) 2025 Frank Secilia
*/

#pragma once

#include <dink/lib.hpp>
#include <dink/memory/page.hpp>
#include <dink/memory/page_factory.hpp>
#include <cstddef>
#include <new>
#include <vector>

namespace dink {

/*!
    paged arena allocator

    This is a typical O(1) append-only, paged arena allocator, but it is written using a pure DI style, so the shape is
    a little different. The algorithm is the same, though.

    It has an array of pages. It tries to allocate from the leaf page. If that allocation fails, it creates a new page
    and tries to allocate from that. If allocating from that new page fails, the allocation fails, otherwise, the new
    page becomes the new leaf. That's it. The details are deferred to its composition.

    There are two failure conditions to be aware of:
        1) If allocating a page fails, that allocation throws std::bad_alloc.
        2) If a requested allocation, after aligning, is too large to fit in a page, nullptr is returned.

    This implementation doesn't try to track holes at the end of individual pages, but future work could. The page size
    is fairly robust, so slack should only be a problem for large allocations. Larger allocations should come from the
    heap directly anyway, but that must be handled at an implementation level above this allocator.
*/
template <page page_t, page_factory<page_t> page_factory_t>
class paged_arena_allocator_t
{
public:
    //! allocates from leaf page, tries allocating from new page if that fails; returns nullptr if allocation fails
    auto try_allocate(std::size_t size, std::align_val_t alignment) -> void*
    {
        // try allocating from most recent page
        auto result = pages_.back().try_allocate(size, alignment);
        if (result) return result;

        // try allocating from new page
        auto new_page = page_factory_();
        result = new_page.try_allocate(size, alignment);
        if (!result)
        {
            // after alignment, size is still too large to fit into a new page, so this request can never be satisfied
            return nullptr;
        }

        // commit new page
        pages_.emplace_back(std::move(new_page));

        return result;
    }

    paged_arena_allocator_t(page_factory_t page_factory) noexcept : page_factory_{std::move(page_factory)}
    {
        pages_.emplace_back(page_factory_());
    }

private:
    page_factory_t page_factory_{};
    std::vector<page_t> pages_{};
};

} // namespace dink
