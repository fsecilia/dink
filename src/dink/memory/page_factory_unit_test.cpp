/*!
    \file
    Copyright (C) 2025 Frank Secilia
*/

#include "page_factory.hpp"
#include <dink/test.hpp>
#include <dink/memory/owned_buffer.hpp>

namespace dink {
namespace {

struct page_factory_test_t : Test
{
    struct page_t
    {
        auto try_allocate(std::size_t size, std::align_val_t alignment) -> void*;

        owned_buffer_t owned_buffer;

        page_t(owned_buffer_t&& owned_buffer) : owned_buffer{std::move(owned_buffer)} {}
    };

    struct buffer_source_t
    {
        auto size() const noexcept -> std::size_t;
        auto alignment() const noexcept -> std::size_t;

        mutable owned_buffer_t owned_buffer;
        auto operator()() const noexcept -> owned_buffer_t { return std::move(owned_buffer); }
    };

    using sut_t = page_factory_t<page_t, buffer_source_t>;

    inline static auto const expected_size = 1024;
};

TEST_F(page_factory_test_t, call_operator)
{
    // arrange for owned_buffer with expected address and expected size
    auto allocation = std::make_unique<std::byte[]>(expected_size);
    auto const expected_address = allocation.get();
    sut_t sut{buffer_source_t{owned_buffer_t{std::move(allocation), expected_size}}};

    // get page
    auto page = sut();

    // check page contents
    ASSERT_EQ(expected_address, page.owned_buffer.allocation.get());
    ASSERT_EQ(expected_size, page.owned_buffer.size);
}

} // namespace
} // namespace dink
