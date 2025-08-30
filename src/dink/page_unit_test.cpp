/*!
    \file
    Copyright (C) 2025 Frank Secilia
*/

#include "page.hpp"
#include <dink/test.hpp>

namespace dink {
namespace {

struct page_test_t : Test
{
    using sut_t = page_t;

    using memory_api_t = memory::api_t;
    using page_size_fallback_t = memory::fallback::page_size_t;
    using page_size_t = memory::page_size_t;
    using buffer_source_t = dink::heap_page_buffer_source_t<array_allocator_t, page_size_t>;
    using page_factory_t = dink::page_factory_t<buffer_source_t>;
    using paged_arena_t = dink::paged_arena_t<page_factory_t>;

    paged_arena_t paged_arena{
        page_factory_t{buffer_source_t{array_allocator_t{}, page_size_t{memory_api_t{}, page_size_fallback_t{}}}}
    };
};

} // namespace
} // namespace dink
