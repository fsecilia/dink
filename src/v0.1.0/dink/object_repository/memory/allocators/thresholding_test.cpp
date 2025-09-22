/*!
    \file
    Copyright (C) 2025 Frank Secilia
*/

#include "thresholding.hpp"
#include <dink/test.hpp>

namespace dink::thresholding {
namespace {

struct thresholding_allocator_reservation_test_t : Test
{
    struct mock_reservation_impl_t
    {
        MOCK_METHOD(void*, allocation, (), (const, noexcept));
        MOCK_METHOD(void, commit, (), (noexcept));
        virtual ~mock_reservation_impl_t() = default;
    };
    StrictMock<mock_reservation_impl_t> mock_reservation_impl;

    struct reservation_impl_t
    {
        auto allocation() const noexcept -> void* { return mock->allocation(); }
        auto commit() noexcept -> void { return mock->commit(); }
        mock_reservation_impl_t* mock = nullptr;
    };

    struct small_object_reservation_t : reservation_impl_t
    {};

    struct large_object_reservation_t : reservation_impl_t
    {};

    struct policy_t
    {
        struct small_object_policy_t
        {
            using reservation_t = small_object_reservation_t;
        };

        struct large_object_policy_t
        {
            using reservation_t = large_object_reservation_t;
        };
    };

    using sut_t = reservation_t<policy_t>;

    void* const expected_allocation = this;
};

struct thresholding_allocator_reservation_test_small_object_allocator_t : thresholding_allocator_reservation_test_t
{
    sut_t sut{small_object_reservation_t{&mock_reservation_impl}};
};

TEST_F(thresholding_allocator_reservation_test_small_object_allocator_t, allocation_is_delegated_to_small_object_strat)
{
    EXPECT_CALL(mock_reservation_impl, allocation()).WillOnce(Return(expected_allocation));

    auto const actual_allocation = sut.allocation();

    ASSERT_EQ(expected_allocation, actual_allocation);
}

TEST_F(thresholding_allocator_reservation_test_small_object_allocator_t, commit_is_delegated_to_small_object_strat)
{
    EXPECT_CALL(mock_reservation_impl, commit());
    sut.commit();
}

struct thresholding_allocator_reservation_test_large_object_allocator_t : thresholding_allocator_reservation_test_t
{
    sut_t sut{large_object_reservation_t{&mock_reservation_impl}};
};

TEST_F(thresholding_allocator_reservation_test_large_object_allocator_t, allocation_is_delegated_to_large_object_strat)
{
    EXPECT_CALL(mock_reservation_impl, allocation()).WillOnce(Return(expected_allocation));

    auto const actual_allocation = sut.allocation();

    ASSERT_EQ(expected_allocation, actual_allocation);
}

TEST_F(thresholding_allocator_reservation_test_large_object_allocator_t, commit_is_delegated_to_large_object_strat)
{
    EXPECT_CALL(mock_reservation_impl, commit());
    sut.commit();
}

} // namespace
} // namespace dink::thresholding
