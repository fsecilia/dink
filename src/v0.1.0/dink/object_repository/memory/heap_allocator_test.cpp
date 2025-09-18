/*!
    \file
    Copyright (C) 2025 Frank Secilia
*/

#include "heap_allocator.hpp"
#include <dink/test.hpp>

namespace dink {
namespace {

struct heap_allocator_test_t : Test
{
    struct deleter_t
    {
        auto operator()(void*) const noexcept {}
    };

    struct mock_api_t
    {
        MOCK_METHOD(void*, malloc, (std::size_t), (const, noexcept));
        MOCK_METHOD(void*, aligned_alloc, (std::size_t, std::size_t), (const, noexcept));

        virtual ~mock_api_t() = default;
    };
    StrictMock<mock_api_t> mock_api{};

    struct api_t
    {
        auto malloc(std::size_t size) const noexcept -> void* { return mock->malloc(size); }
        auto aligned_alloc(std::size_t alignment, std::size_t size) const noexcept -> void*
        {
            return mock->aligned_alloc(alignment, size);
        }

        mock_api_t* mock = nullptr;
    };

    using sut_t = heap_allocator_t<deleter_t, api_t>;
    sut_t sut{api_t{&mock_api}};

    std::size_t expected_size = 1024;
    std::size_t expected_alignment = 128;
    void* expected_result = this; // arbitrary
};

TEST_F(heap_allocator_test_t, unaligned_allocate_forwards_to_malloc)
{
    EXPECT_CALL(mock_api, malloc(expected_size)).WillOnce(Return(expected_result));

    auto const actual_result = sut.allocate(expected_size);

    ASSERT_EQ(expected_result, actual_result.get());
}

TEST_F(heap_allocator_test_t, unaligned_allocate_throws_on_failed_alloc)
{
    EXPECT_CALL(mock_api, malloc(expected_size)).WillOnce(Return(nullptr));

    EXPECT_THROW((void)sut.allocate(expected_size), std::bad_alloc);
}

TEST_F(heap_allocator_test_t, aligned_allocate_forwards_to_aligned_alloc)
{
    EXPECT_CALL(mock_api, aligned_alloc(expected_alignment, expected_size)).WillOnce(Return(expected_result));

    auto const actual_result = sut.allocate(expected_size, std::align_val_t{expected_alignment});

    ASSERT_EQ(expected_result, actual_result.get());
}

TEST_F(heap_allocator_test_t, aligned_allocate_throws_on_failed_alloc)
{
    EXPECT_CALL(mock_api, aligned_alloc(expected_alignment, expected_size)).WillOnce(Return(nullptr));

    EXPECT_THROW((void)sut.allocate(expected_size, std::align_val_t{expected_alignment}), std::bad_alloc);
}

} // namespace
} // namespace dink
