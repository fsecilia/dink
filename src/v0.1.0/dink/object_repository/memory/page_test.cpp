/*!
    \file
    Copyright (C) 2025 Frank Secilia
*/

#include "page.hpp"
#include <dink/test.hpp>

namespace dink {
namespace {

struct page_pending_allocation_test_t : Test
{
    struct mock_page_t
    {
        MOCK_METHOD(void, commit, (void*), (noexcept));
        virtual ~mock_page_t() = default;
    };
    StrictMock<mock_page_t> mock_page;

    struct page_t
    {
        auto commit(void* allocation) noexcept -> void { mock->commit(allocation); }
        mock_page_t* mock = nullptr;
    };
    page_t page{&mock_page};

    void* const allocation_begin = this;
    void* const allocation_end = this + 1;

    using sut_t = page_pending_allocation_t<page_t>;
    sut_t sut{page, allocation_begin, allocation_end};
};

TEST_F(page_pending_allocation_test_t, allocation)
{
    ASSERT_EQ(allocation_begin, sut.allocation());
}

TEST_F(page_pending_allocation_test_t, commit)
{
    EXPECT_CALL(mock_page, commit(allocation_end));

    sut.commit();
}

// ---------------------------------------------------------------------------------------------------------------------

struct page_test_t : Test
{
    template <typename page_t>
    struct pending_allocation_t
    {
        page_t* page;
        void* allocation_begin;
        void* allocation_end;

        pending_allocation_t(page_t& page, void* allocation_begin, void* allocation_end) noexcept
            : page{&page}, allocation_begin{allocation_begin}, allocation_end{allocation_end}
        {}
    };

    static inline constexpr auto const region_size = 1024;
    std::size_t const max_allocation_size = region_size / 4;
    std::size_t const size = alignment * 2;

    static inline constexpr auto const alignment = 16;
    std::align_val_t const align_val = static_cast<std::align_val_t>(alignment);

    alignas(alignment) std::array<std::byte, region_size> region;
    using sut_t = page_t<pending_allocation_t>;
    sut_t sut{std::data(region), region_size, max_allocation_size};
};

TEST_F(page_test_t, max_allocation_size_returns_constructed_value)
{
    ASSERT_EQ(max_allocation_size, sut.max_allocation_size());
}

TEST_F(page_test_t, reserve_sets_page_field_in_pending_allocation)
{
    auto const result = sut.reserve(size, align_val);
    ASSERT_EQ(&sut, result.page);
}

TEST_F(page_test_t, reserve_returns_current_address_when_current_address_is_already_aligned)
{
    // start at first aligned location after beginning
    auto const expected_begin = std::data(region) + alignment;
    sut.commit(expected_begin);

    auto const result = sut.reserve(size, align_val);

    // result should be that the same location
    ASSERT_EQ(expected_begin, result.allocation_begin);
    ASSERT_EQ(expected_begin + size, result.allocation_end);
}

TEST_F(page_test_t, reserve_returns_next_aligned_address_when_current_address_is_misaligned)
{
    // misalign allocation end by one
    sut.commit(std::data(region) + 1);

    // result should be at the first aligned location after the beginning
    auto const expected_begin = std::data(region) + alignment;

    auto const result = sut.reserve(size, align_val);

    ASSERT_EQ(expected_begin, result.allocation_begin);
    ASSERT_EQ(expected_begin + size, result.allocation_end);
}

TEST_F(page_test_t, reserve_succeeds_when_worst_case_is_exactly_max_allocation_size)
{
    // set up worst-case alignment where size + padding equals the limit: size + (alignment - 1) == max_allocation_size
    auto const exact_size = max_allocation_size - (alignment - 1);

    // misalign cur_ by 1 to force the maximum padding
    sut.commit(std::data(region) + 1);
    auto const expected_begin = std::data(region) + alignment;

    auto pending_allocation = sut.reserve(exact_size, align_val);

    ASSERT_EQ(expected_begin, pending_allocation.allocation_begin);
}

TEST_F(page_test_t, reserve_returns_nullptr_when_size_exceeds_max_allocation_size)
{
    // request exceeds limit, but would fit otherwise
    auto pending_allocation = sut.reserve(max_allocation_size + 1, std::align_val_t{1});

    ASSERT_EQ(nullptr, pending_allocation.allocation_begin);
}

TEST_F(page_test_t, reserve_returns_nonempty_allocation_when_size_is_zero)
{
    auto pending_allocation = sut.reserve(0, align_val);
    ASSERT_LT(pending_allocation.allocation_begin, pending_allocation.allocation_end);
}

TEST_F(page_test_t, reserve_succeeds_when_size_exactly_fits_region)
{
    // commit end of allocation near end of region
    auto const expected_allocation_begin = std::data(region) + region_size - size;
    sut.commit(expected_allocation_begin);

    // try to reserve allocation that just fits
    auto pending_allocation = sut.reserve(size, std::align_val_t{1});

    ASSERT_EQ(expected_allocation_begin, pending_allocation.allocation_begin);
    ASSERT_EQ(expected_allocation_begin + size, pending_allocation.allocation_end);
}

TEST_F(page_test_t, reserve_returns_nullptr_when_worst_case_alignment_forces_size_past_max_allocation_size)
{
    // allocation size is small enough, but total requested size request exceeds limit
    auto pending_allocation = sut.reserve(max_allocation_size, align_val);

    ASSERT_EQ(nullptr, pending_allocation.allocation_begin);
}

TEST_F(page_test_t, reserve_returns_nullptr_when_size_doesnt_fit_at_end_of_region)
{
    // commit end of allocation near end of region, leaving less room than size requires
    sut.commit(std::data(region) + region_size - (size - 1));

    // try to reserve allocation that extends past end of region
    auto pending_allocation = sut.reserve(size, std::align_val_t{1});

    ASSERT_EQ(nullptr, pending_allocation.allocation_begin);
    ASSERT_EQ(nullptr, pending_allocation.allocation_end);
}

TEST_F(page_test_t, reserve_returns_nullptr_when_alignment_doesnt_fit_at_end_of_region)
{
    // commit end of allocation near end of region, leaving less room than alignment requires
    sut.commit(std::data(region) + region_size - (alignment - 1));

    // try to reserve allocation that aligns to end of region
    auto pending_allocation = sut.reserve(1, align_val);

    ASSERT_EQ(nullptr, pending_allocation.allocation_begin);
    ASSERT_EQ(nullptr, pending_allocation.allocation_end);
}

} // namespace
} // namespace dink
