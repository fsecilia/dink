/*!
    \file
    Copyright (C) 2025 Frank Secilia
*/

#include "arena_allocator.hpp"
#include <dink/test.hpp>

namespace dink {
namespace {

struct arena_allocator_test_t : Test
{
    struct mock_allocator_t
    {
        MOCK_METHOD(void*, allocate, (std::size_t size, std::align_val_t alignment), ());
        MOCK_METHOD(void, roll_back, (), (noexcept));
        virtual ~mock_allocator_t() = default;
    };
    StrictMock<mock_allocator_t> mock_large_object_allocator{};
    StrictMock<mock_allocator_t> mock_small_object_allocator{};

    struct allocator_t
    {
        auto allocate(std::size_t size, std::align_val_t alignment) -> void* { return mock->allocate(size, alignment); }
        auto roll_back() -> void { mock->roll_back(); }
        mock_allocator_t* mock = nullptr;
    };

    struct large_object_allocator_t : allocator_t
    {};

    static auto const small_object_threshold = std::size_t{1234};

    struct small_object_allocator_t : allocator_t
    {
        std::size_t max_allocation_size() const noexcept { return small_object_threshold; }
    };

    static auto const alignment_value = std::size_t{256};

    using sut_t = arena_allocator_t<large_object_allocator_t, small_object_allocator_t>;
    sut_t sut{
        large_object_allocator_t{&mock_large_object_allocator}, small_object_allocator_t{&mock_small_object_allocator}
    };

    auto test_allocation(std::size_t size, std::size_t alignment, mock_allocator_t& expected_mock_allocator) -> void
    {
        auto expected_result = this;
        EXPECT_CALL(expected_mock_allocator, allocate(size, std::align_val_t{alignment}))
            .WillOnce(Return(expected_result));

        auto const actual_result = sut.allocate(size, std::align_val_t{alignment});

        ASSERT_EQ(expected_result, actual_result);
    }
};

TEST_F(arena_allocator_test_t, smallest_allocation)
{
    test_allocation(0, 1, mock_small_object_allocator);
}

TEST_F(arena_allocator_test_t, largest_small_allocation)
{
    test_allocation(small_object_threshold - (alignment_value - 1), alignment_value, mock_small_object_allocator);
}

TEST_F(arena_allocator_test_t, smallest_large_allocation)
{
    test_allocation(small_object_threshold - (alignment_value - 1) + 1, alignment_value, mock_large_object_allocator);
}

} // namespace
} // namespace dink
