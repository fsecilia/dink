/*!
    \file
    Copyright (C) 2025 Frank Secilia
*/

#include "paged_sub_allocator.hpp"
#include <dink/test.hpp>

namespace dink {
namespace {

struct paged_sub_allocator_test_t : Test
{
    struct mock_page_t
    {
        MOCK_METHOD(void*, try_allocate, (std::size_t size, std::align_val_t alignment), ());
        MOCK_METHOD(bool, roll_back, (), (noexcept));
        virtual ~mock_page_t() = default;
    };
    StrictMock<mock_page_t> mock_page_1{};
    StrictMock<mock_page_t> mock_page_2{};

    auto expect_add_page(mock_page_t& mock_page) -> void
    {
        EXPECT_CALL(mock_page_factory, call_operator()).WillOnce(Return(page_t{&mock_page}));
    }

    struct page_t
    {
        auto try_allocate(std::size_t size, std::align_val_t alignment) const -> void*
        {
            return mock->try_allocate(size, alignment);
        }

        auto roll_back() noexcept -> bool { return mock->roll_back(); }

        mock_page_t* mock = nullptr;
    };

    struct mock_page_factory_t
    {
        MOCK_METHOD(page_t, call_operator, (), (const));
        virtual ~mock_page_factory_t() = default;
    };
    StrictMock<mock_page_factory_t> mock_page_factory{};

    inline static std::size_t const page_size = 2048;
    inline static std::size_t const expected_max_allocation_size = 256;
    struct page_factory_t
    {
        auto size() const noexcept -> std::size_t { return page_size; }
        auto alignment() const noexcept -> std::size_t;
        auto operator()() -> page_t { return mock->call_operator(); }
        mock_page_factory_t* mock = nullptr;
    };

    void* const expected_allocation_1 = reinterpret_cast<void*>(0x1000);
    void* const expected_allocation_2 = reinterpret_cast<void*>(0x1001);
    auto expect_try_allocate(mock_page_t& mock_page, void* expected_allocation) -> void*
    {
        EXPECT_CALL(mock_page, try_allocate(expected_size, expected_alignment)).WillOnce(Return(expected_allocation));
        return expected_allocation;
    }

    auto expect_allocation_succeeds(mock_page_t& mock_page, void* expected_allocation) -> void
    {
        expect_try_allocate(mock_page, expected_allocation);
    }

    auto expect_allocation_fails(mock_page_t& mock_page) -> void { expect_try_allocate(mock_page, nullptr); }

    using ctor_params_t = paged_sub_allocator_ctor_params_t<page_t, page_factory_t>;
    using sut_t = paged_sub_allocator_t<page_t, page_factory_t, ctor_params_t>;
    auto create_sut() -> sut_t
    {
        return sut_t{ctor_params_t{page_factory_t{page_factory_t{&mock_page_factory}}, page_t{&mock_page_1}}};
    }
    sut_t sut = create_sut();

    inline static std::size_t const expected_size = 128;
    inline static std::align_val_t const expected_alignment = std::align_val_t{16};
};

TEST_F(paged_sub_allocator_test_t, max_allocation_size)
{
    ASSERT_EQ(expected_max_allocation_size, sut.max_allocation_size());
}

TEST_F(paged_sub_allocator_test_t, allocate_preconditions_met)
{
    ASSERT_TRUE(sut.allocate_preconditions_met(expected_max_allocation_size, std::align_val_t{1}));
    ASSERT_FALSE(sut.allocate_preconditions_met(expected_max_allocation_size + 1, std::align_val_t{1}));
    ASSERT_FALSE(sut.allocate_preconditions_met(expected_max_allocation_size, std::align_val_t{2}));

    ASSERT_TRUE(sut.allocate_preconditions_met(
        expected_max_allocation_size / 2 + 1, std::align_val_t{expected_max_allocation_size / 2}
    ));
    ASSERT_FALSE(sut.allocate_preconditions_met(
        expected_max_allocation_size / 2 + 2, std::align_val_t{expected_max_allocation_size / 2}
    ));
}

TEST_F(paged_sub_allocator_test_t, allocate_succeeds_on_first_page)
{
    expect_allocation_succeeds(mock_page_1, expected_allocation_1);

    auto const actual_allocation = sut.allocate(expected_size, expected_alignment);

    ASSERT_EQ(expected_allocation_1, actual_allocation);
}

TEST_F(paged_sub_allocator_test_t, allocate_multiple_from_same_page)
{
    auto seq = InSequence{}; // ordered

    expect_allocation_succeeds(mock_page_1, expected_allocation_1);
    expect_allocation_succeeds(mock_page_1, expected_allocation_2);

    auto const actual_allocation1 = sut.allocate(expected_size, expected_alignment);
    auto const actual_allocation2 = sut.allocate(expected_size, expected_alignment);

    ASSERT_EQ(expected_allocation_1, actual_allocation1);
    ASSERT_EQ(expected_allocation_2, actual_allocation2);
}

TEST_F(paged_sub_allocator_test_t, allocate_creates_new_page_when_current_is_full)
{
    auto seq = InSequence{}; // ordered

    // allocation from current page fails
    expect_allocation_fails(mock_page_1);

    // sut allocates new page
    expect_add_page(mock_page_2);

    // allocation from new page succeeds
    expect_allocation_succeeds(mock_page_2, expected_allocation_1);

    // allocate
    auto const actual_allocation = sut.allocate(expected_size, expected_alignment);

    // result is from new page
    ASSERT_EQ(expected_allocation_1, actual_allocation);
}

TEST_F(paged_sub_allocator_test_t, roll_back_to_empty_first_page_does_not_pop_page)
{
    EXPECT_CALL(mock_page_1, roll_back()).WillOnce(Return(true));
    sut.roll_back();

    // next allocation comes from first page
    expect_allocation_succeeds(mock_page_1, expected_allocation_1);
    auto const actual_allocation_1 = sut.allocate(expected_size, expected_alignment);
    ASSERT_EQ(expected_allocation_1, actual_allocation_1);
}

TEST_F(paged_sub_allocator_test_t, roll_back_to_nonempty_first_page_does_not_pop_page)
{
    EXPECT_CALL(mock_page_1, roll_back()).WillOnce(Return(false));
    sut.roll_back();

    // next allocation comes from first page
    expect_allocation_succeeds(mock_page_1, expected_allocation_1);
    auto const actual_allocation_1 = sut.allocate(expected_size, expected_alignment);
    ASSERT_EQ(expected_allocation_1, actual_allocation_1);
}

TEST_F(paged_sub_allocator_test_t, roll_back_to_nonempty_second_page_does_not_pop_page)
{
    auto seq = InSequence{}; // ordered

    // drive sut into state with an allocation on second page
    expect_allocation_fails(mock_page_1);
    expect_add_page(mock_page_2);
    expect_allocation_succeeds(mock_page_2, expected_allocation_1);
    sut.allocate(expected_size, expected_alignment);

    // page is not empty after rollback
    EXPECT_CALL(mock_page_2, roll_back()).WillOnce(Return(false));
    sut.roll_back();

    // next allocation still comes from second page
    expect_allocation_succeeds(mock_page_2, expected_allocation_2);
    auto const actual_allocation_2 = sut.allocate(expected_size, expected_alignment);
    ASSERT_EQ(expected_allocation_2, actual_allocation_2);
}

TEST_F(paged_sub_allocator_test_t, roll_back_to_empty_second_page_pops_page)
{
    auto seq = InSequence{}; // ordered

    // drive sut into state with an allocation on second page
    expect_allocation_fails(mock_page_1);
    expect_add_page(mock_page_2);
    expect_allocation_succeeds(mock_page_2, expected_allocation_1);
    sut.allocate(expected_size, expected_alignment);

    // page is empty after rollback
    EXPECT_CALL(mock_page_2, roll_back()).WillOnce(Return(true));
    sut.roll_back();

    // next allocation comes from first page
    expect_allocation_succeeds(mock_page_1, expected_allocation_2);
    auto const actual_allocation_2 = sut.allocate(expected_size, expected_alignment);
    ASSERT_EQ(expected_allocation_2, actual_allocation_2);
}

} // namespace
} // namespace dink
