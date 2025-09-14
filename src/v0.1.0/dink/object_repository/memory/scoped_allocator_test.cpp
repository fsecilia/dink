/*!
    \file
    Copyright (C) 2025 Frank Secilia
*/

#include "scoped_allocator.hpp"
#include <dink/test.hpp>

namespace dink {
namespace {

struct scoped_allocator_test_t : Test
{
    struct mock_allocator_t
    {
        MOCK_METHOD(void*, allocate, (std::size_t size, std::align_val_t alignment));
        MOCK_METHOD(void, deallocate, (void* allocation), (noexcept));
        virtual ~mock_allocator_t() = default;
    };
    StrictMock<mock_allocator_t> mock_allocator{};

    struct allocator_t
    {
        struct deleter_t
        {
            auto operator()(void* allocation) const noexcept -> void { allocator->deallocate(allocation); }
            allocator_t* allocator;
        };
        using allocation_t = std::unique_ptr<void, deleter_t>;

        auto allocate(std::size_t size, std::align_val_t alignment) -> allocation_t
        {
            return allocation_t{mock->allocate(size, alignment), deleter_t{this}};
        }

        auto deallocate(void* allocation) noexcept -> void { mock->deallocate(allocation); }

        mock_allocator_t* mock = nullptr;
    };

    std::align_val_t const expected_alignment_1 = std::align_val_t{16};
    std::size_t const expected_size_1 = 1024;

    std::align_val_t const expected_alignment_2 = std::align_val_t{64};
    std::size_t const expected_size_2 = 640;

    std::align_val_t const expected_alignment_3 = std::align_val_t{32};
    std::size_t const expected_size_3 = 64;

    void* expected_allocation_1 = reinterpret_cast<void*>(0x1000);
    void* expected_allocation_2 = reinterpret_cast<void*>(0x1001);
    void* expected_allocation_3 = reinterpret_cast<void*>(0x1002);

    auto expect_all_allocate() -> void
    {
        EXPECT_CALL(mock_allocator, allocate(expected_size_1, expected_alignment_1))
            .WillOnce(Return(expected_allocation_1));
        EXPECT_CALL(mock_allocator, allocate(expected_size_2, expected_alignment_2))
            .WillOnce(Return(expected_allocation_2));
        EXPECT_CALL(mock_allocator, allocate(expected_size_3, expected_alignment_3))
            .WillOnce(Return(expected_allocation_3));
    }

    auto allocate_all() -> void
    {
        EXPECT_CALL(mock_allocator, allocate(expected_size_1, expected_alignment_1))
            .WillOnce(Return(expected_allocation_1));
        EXPECT_CALL(mock_allocator, allocate(expected_size_2, expected_alignment_2))
            .WillOnce(Return(expected_allocation_2));
        EXPECT_CALL(mock_allocator, allocate(expected_size_3, expected_alignment_3))
            .WillOnce(Return(expected_allocation_3));
    }

    auto expect_all_deallocate() -> void
    {
        EXPECT_CALL(mock_allocator, deallocate(expected_allocation_1));
        EXPECT_CALL(mock_allocator, deallocate(expected_allocation_2));
        EXPECT_CALL(mock_allocator, deallocate(expected_allocation_3));
    }
};

struct scoped_allocator_test_default_container_t : scoped_allocator_test_t
{
    using sut_t = scoped_allocator_t<allocator_t>;
    sut_t sut{allocator_t{&mock_allocator}};
};

TEST_F(scoped_allocator_test_default_container_t, allocate_tracks_pointers)
{
    expect_all_allocate();

    auto actual_allocation_1 = sut.allocate(expected_size_1, expected_alignment_1);
    auto actual_allocation_2 = sut.allocate(expected_size_2, expected_alignment_2);
    auto actual_allocation_3 = sut.allocate(expected_size_3, expected_alignment_3);

    ASSERT_EQ(expected_allocation_1, actual_allocation_1);
    ASSERT_EQ(expected_allocation_2, actual_allocation_2);
    ASSERT_EQ(expected_allocation_3, actual_allocation_3);

    expect_all_deallocate();
}

TEST_F(scoped_allocator_test_default_container_t, roll_back_without_allocation_is_noop)
{
    sut.roll_back();
}

TEST_F(scoped_allocator_test_default_container_t, roll_back_after_allocation)
{
    expect_all_allocate();

    sut.allocate(expected_size_1, expected_alignment_1);
    sut.allocate(expected_size_2, expected_alignment_2);
    sut.allocate(expected_size_3, expected_alignment_3);

    {
        EXPECT_CALL(mock_allocator, deallocate(expected_allocation_3));
        sut.roll_back();
    }

    EXPECT_CALL(mock_allocator, deallocate(expected_allocation_1));
    EXPECT_CALL(mock_allocator, deallocate(expected_allocation_2));
}

TEST_F(scoped_allocator_test_default_container_t, move)
{
    expect_all_allocate();

    sut.allocate(expected_size_1, expected_alignment_1);
    sut.allocate(expected_size_2, expected_alignment_2);
    sut.allocate(expected_size_3, expected_alignment_3);

    auto move_dst = sut_t{std::move(sut)};

    expect_all_deallocate();
}

struct scoped_allocator_test_throw_on_emplace_back_vector_t : scoped_allocator_test_t
{
    struct throw_on_emplace_back_vector_t
    {
        using value_type = allocator_t::allocation_t;
        auto begin() const -> value_type* { return nullptr; }
        auto end() const -> value_type* { return nullptr; }
        auto emplace_back(value_type&&) -> value_type& { throw std::bad_alloc{}; }
    };

    using sut_t = scoped_allocator_t<allocator_t, throw_on_emplace_back_vector_t>;
    sut_t sut{allocator_t{&mock_allocator}, throw_on_emplace_back_vector_t{}};
};

TEST_F(scoped_allocator_test_throw_on_emplace_back_vector_t, allocate_cleans_up_and_rethrows_on_bad_alloc)
{
    EXPECT_CALL(mock_allocator, allocate(expected_size_1, expected_alignment_1))
        .WillOnce(Return(expected_allocation_1));
    EXPECT_CALL(mock_allocator, deallocate(expected_allocation_1));

    EXPECT_THROW(sut.allocate(expected_size_1, expected_alignment_1), std::bad_alloc);
}

} // namespace
} // namespace dink
