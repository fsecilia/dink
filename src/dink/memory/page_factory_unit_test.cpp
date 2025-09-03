/*!
    \file
    Copyright (C) 2025 Frank Secilia
*/

#include "page_factory.hpp"
#include <dink/test.hpp>

namespace dink {
namespace {

struct page_factory_test_t : Test
{
    struct page_t
    {
        auto try_allocate(std::size_t size, std::align_val_t alignment) -> void*;
        auto roll_back() noexcept -> bool;

        void* begin;
        std::size_t size;
    };

    struct mock_heap_allocator_t
    {
        MOCK_METHOD(void*, allocate, (std::size_t size, std::align_val_t alignment), (const));
        MOCK_METHOD(void, roll_back, (), (noexcept));
        virtual ~mock_heap_allocator_t() = default;
    };
    StrictMock<mock_heap_allocator_t> mock_heap_allocator;

    struct heap_allocator_t
    {
        auto allocate(std::size_t size, std::align_val_t alignment) const -> void*
        {
            return mock->allocate(size, alignment);
        }

        auto roll_back() noexcept -> void { mock->roll_back(); }

        mock_heap_allocator_t* mock = nullptr;
    };

    inline static auto const os_page_size = std::size_t{1024};
    struct os_page_size_t
    {
        auto operator()() const noexcept -> std::size_t { return os_page_size; }
    };

    using sut_t = page_factory_t<page_t, heap_allocator_t, os_page_size_t>;
    sut_t sut{heap_allocator_t{&mock_heap_allocator}, os_page_size_t{}};

    std::size_t const expected_size = os_page_size * sut_t::pages_per_buffer;
    std::size_t const expected_alignment = os_page_size;
    void* const expected_allocation = reinterpret_cast<void*>(0x1000);
};

TEST_F(page_factory_test_t, size)
{
    ASSERT_EQ(expected_size, sut.size());
}

TEST_F(page_factory_test_t, alignment)
{
    ASSERT_EQ(expected_alignment, sut.alignment());
}

TEST_F(page_factory_test_t, call_operator)
{
    EXPECT_CALL(mock_heap_allocator, allocate(expected_size, std::align_val_t{expected_alignment}))
        .WillOnce(Return(std::move(expected_allocation)));

    auto page = sut();

    ASSERT_EQ(expected_allocation, page.begin);
    ASSERT_EQ(expected_size, page.size);
}

} // namespace
} // namespace dink
