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

// ---------------------------------------------------------------------------------------------------------------------

struct thresholding_allocator_test_t : Test
{
    struct reservation_t
    {
        enum id_t
        {
            default_initialized,
            small_object,
            large_object
        };
        id_t id = id_t::default_initialized;
    };

    struct mock_small_object_allocator_t
    {
        MOCK_METHOD(std::size_t, max_allocation_size, (), (const, noexcept));
        MOCK_METHOD(reservation_t::id_t, reserve, (std::size_t size, std::align_val_t align_val));
        virtual ~mock_small_object_allocator_t() = default;
    };
    StrictMock<mock_small_object_allocator_t> mock_small_object_allocator;

    struct small_object_allocator_t
    {
        auto max_allocation_size() const noexcept -> std::size_t { return mock->max_allocation_size(); }
        auto reserve(std::size_t size, std::align_val_t align_val) -> reservation_t::id_t
        {
            return mock->reserve(size, align_val);
        }
        mock_small_object_allocator_t* mock = nullptr;
    };

    struct mock_large_object_allocator_t
    {
        MOCK_METHOD(reservation_t::id_t, reserve, (std::size_t size, std::align_val_t align_val));
        virtual ~mock_large_object_allocator_t() = default;
    };
    StrictMock<mock_large_object_allocator_t> mock_large_object_allocator;

    struct large_object_allocator_t
    {
        auto reserve(std::size_t size, std::align_val_t align_val) -> reservation_t::id_t
        {
            return mock->reserve(size, align_val);
        }
        mock_large_object_allocator_t* mock = nullptr;
    };

    struct policy_t
    {
        struct small_object_policy_t
        {
            using allocator_t = small_object_allocator_t;
        };

        struct large_object_policy_t
        {
            using allocator_t = large_object_allocator_t;
        };

        using reservation_t = reservation_t;
    };

    using sut_t = allocator_t<policy_t>;
    sut_t sut{
        small_object_allocator_t{&mock_small_object_allocator}, large_object_allocator_t{&mock_large_object_allocator}
    };

    std::size_t const small_object_allocator_max_allocation_size = 1024;
    std::align_val_t const expected_align_val = std::align_val_t{16};
    std::size_t const aligned_threshold_size
        = small_object_allocator_max_allocation_size - (static_cast<std::size_t>(expected_align_val) - 1);
};

TEST_F(thresholding_allocator_test_t, threshold_forwards_to_small_object_allocator)
{
    EXPECT_CALL(mock_small_object_allocator, max_allocation_size())
        .WillOnce(Return(small_object_allocator_max_allocation_size));
    ASSERT_EQ(small_object_allocator_max_allocation_size, sut.threshold());
}

TEST_F(thresholding_allocator_test_t, reserve_small_allocation_forwards_to_small_object_allocator)
{
    EXPECT_CALL(mock_small_object_allocator, max_allocation_size())
        .WillOnce(Return(small_object_allocator_max_allocation_size));

    auto const expected_allocation_size = aligned_threshold_size;
    auto const expected_reservation_id = reservation_t::id_t::small_object;
    EXPECT_CALL(mock_small_object_allocator, reserve(expected_allocation_size, expected_align_val))
        .WillOnce(Return(expected_reservation_id));

    auto const acutal_reservation = sut.reserve(expected_allocation_size, expected_align_val);

    ASSERT_EQ(expected_reservation_id, acutal_reservation.id);
}

TEST_F(thresholding_allocator_test_t, reserve_large_allocation_forwards_to_large_object_allocator)
{
    EXPECT_CALL(mock_small_object_allocator, max_allocation_size())
        .WillOnce(Return(small_object_allocator_max_allocation_size));

    auto const expected_allocation_size = aligned_threshold_size + 1;
    auto const expected_reservation_id = reservation_t::id_t::large_object;
    EXPECT_CALL(mock_large_object_allocator, reserve(expected_allocation_size, expected_align_val))
        .WillOnce(Return(expected_reservation_id));

    auto const acutal_reservation = sut.reserve(expected_allocation_size, expected_align_val);

    ASSERT_EQ(expected_reservation_id, acutal_reservation.id);
}

} // namespace
} // namespace dink::thresholding
