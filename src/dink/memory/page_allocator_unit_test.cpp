/*!
    \file
    Copyright (C) 2025 Frank Secilia
*/

#include "page_allocator.hpp"
#include <dink/test.hpp>

namespace dink {
namespace {

struct page_allocator_test_t : Test
{
    using dynamic_array_t = std::unique_ptr<std::byte[]>;

    struct mock_array_allocator_t
    {
        MOCK_METHOD(
            std::unique_ptr<std::byte[]>, call_operator, (std::size_t size, std::align_val_t alignment), (const)
        );

        auto operator()(std::size_t size, std::align_val_t alignment) const -> dynamic_array_t
        {
            return call_operator(size, alignment);
        }

        virtual ~mock_array_allocator_t() = default;
    };
    StrictMock<mock_array_allocator_t> mock_array_allocator;

    struct array_allocator_t
    {
        auto operator()(std::size_t size, std::align_val_t alignment) const -> dynamic_array_t
        {
            return (*mock)(size, alignment);
        }
        mock_array_allocator_t* mock = nullptr;
    };

    inline static auto const os_page_size = std::size_t{1024};
    struct os_page_size_t
    {
        auto operator()() const noexcept -> std::size_t { return os_page_size; }
    };

    using sut_t = page_allocator_t<array_allocator_t, os_page_size_t>;
    sut_t sut{array_allocator_t{&mock_array_allocator}, os_page_size_t{}};

    inline static auto const expected_size = os_page_size * sut_t::pages_per_buffer;
    inline static auto const expected_alignment = os_page_size;
};

TEST_F(page_allocator_test_t, size)
{
    ASSERT_EQ(expected_size, sut.size());
}

TEST_F(page_allocator_test_t, alignment)
{
    ASSERT_EQ(expected_alignment, sut.alignment());
}

TEST_F(page_allocator_test_t, call_operator)
{
    // arrange for owned_buffer with expected address and expected size
    auto expected_allocation = std::make_unique<std::byte[]>(expected_size);
    auto expected_address = expected_allocation.get();
    EXPECT_CALL(mock_array_allocator, call_operator(expected_size, std::align_val_t{expected_alignment}))
        .WillOnce(Return(std::move(expected_allocation)));

    // get buffer
    auto actual = sut();

    // compare contents
    ASSERT_EQ(expected_address, actual.allocation.get());
    ASSERT_EQ(expected_size, actual.size);
}

} // namespace
} // namespace dink
