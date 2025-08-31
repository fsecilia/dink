/*!
    \file
    Copyright (C) 2025 Frank Secilia
*/

#include "paged_arena_allocator.hpp"
#include <dink/test.hpp>

namespace dink {
namespace {

struct paged_arena_allocator_test_t : Test
{
    struct mock_page_t
    {
        MOCK_METHOD(void*, try_allocate, (std::size_t size, std::align_val_t alignment), (const));
        virtual ~mock_page_t() = default;
    };
    StrictMock<mock_page_t> mock_page_1{};
    StrictMock<mock_page_t> mock_page_2{};
    StrictMock<mock_page_t>& mock_page = mock_page_1;

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

        mock_page_t* mock = nullptr;
    };

    struct mock_page_factory_t
    {
        MOCK_METHOD(page_t, call_operator, (), (const));
        virtual ~mock_page_factory_t() = default;
    };
    StrictMock<mock_page_factory_t> mock_page_factory{};

    struct page_factory_t
    {
        auto operator()() const -> page_t { return mock->call_operator(); }
        mock_page_factory_t* mock = nullptr;
    };

    void* const expected_allocation_1 = reinterpret_cast<void*>(0x1000);
    void* const expected_allocation_2 = reinterpret_cast<void*>(0x1001);
    void* const expected_allocation = expected_allocation_1;
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

    using sut_t = paged_arena_allocator_t<page_t, page_factory_t>;
    auto create_sut() -> sut_t
    {
        expect_add_page(mock_page_1);
        return sut_t{page_factory_t{page_factory_t{&mock_page_factory}}};
    }
    sut_t sut = create_sut();

    std::size_t const expected_size = 1024;
    std::align_val_t const expected_alignment = std::align_val_t{16};
};

TEST_F(paged_arena_allocator_test_t, try_allocate_succeeds_on_first_page)
{
    expect_allocation_succeeds(mock_page, expected_allocation);

    auto const actual_allocation = sut.try_allocate(expected_size, expected_alignment);

    ASSERT_EQ(expected_allocation, actual_allocation);
}

TEST_F(paged_arena_allocator_test_t, try_allocate_multiple_from_same_page)
{
    auto seq = InSequence{}; // ordered

    expect_allocation_succeeds(mock_page, expected_allocation_1);
    expect_allocation_succeeds(mock_page, expected_allocation_2);

    auto const actual_allocation1 = sut.try_allocate(expected_size, expected_alignment);
    auto const actual_allocation2 = sut.try_allocate(expected_size, expected_alignment);

    ASSERT_EQ(expected_allocation_1, actual_allocation1);
    ASSERT_EQ(expected_allocation_2, actual_allocation2);
}

TEST_F(paged_arena_allocator_test_t, try_allocate_creates_new_page_when_current_is_full)
{
    auto seq = InSequence{}; // ordered

    // allocation from current page fails
    expect_allocation_fails(mock_page_1);

    // sut allocates new page
    expect_add_page(mock_page_2);

    // allocation from new page succeeds
    expect_allocation_succeeds(mock_page_2, expected_allocation);

    // try allocate
    auto const actual_allocation = sut.try_allocate(expected_size, expected_alignment);

    // result is from new page
    ASSERT_EQ(expected_allocation, actual_allocation);
}

TEST_F(paged_arena_allocator_test_t, try_allocate_fails_when_new_page_is_still_too_small)
{
    auto seq = InSequence{}; // ordered

    // allocation from current page fails
    expect_allocation_fails(mock_page_1);

    // sut allocates new page
    expect_add_page(mock_page_2);

    // allocation from new page still fails
    expect_allocation_fails(mock_page_2);

    // try allocate
    auto const actual_allocation = sut.try_allocate(expected_size, expected_alignment);

    // result is allocation failed
    ASSERT_EQ(nullptr, actual_allocation);
}

} // namespace
} // namespace dink
