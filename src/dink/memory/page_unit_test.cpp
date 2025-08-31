/*!
    \file
    Copyright (C) 2025 Frank Secilia
*/

#include "page.hpp"
#include <dink/test.hpp>

namespace dink {
namespace {

struct page_test_t : Test
{
    inline static constexpr auto const page_size = 1024;

    using sut_t = page_t;
    auto create_sut() -> sut_t
    {
        auto buffer = std::make_unique<std::byte[]>(page_size);
        return sut_t{owned_buffer_t{std::move(buffer), page_size}};
    }
    sut_t sut = create_sut();

    auto expect_allocation_succeeded(void* allocation) const noexcept -> void { ASSERT_NE(allocation, nullptr); }
    auto expect_allocation_failed(void* allocation) const noexcept -> void { ASSERT_EQ(allocation, nullptr); }
};

TEST_F(page_test_t, allocate_succeeds_on_new_page)
{
    auto const allocation = sut.try_allocate(page_size / 4, 16);

    expect_allocation_succeeded(allocation);
}

TEST_F(page_test_t, allocate_succeeds_when_filling_page_exactly)
{
    auto const allocation = sut.try_allocate(page_size, 1);

    expect_allocation_succeeded(allocation);
}

TEST_F(page_test_t, allocate_fails_when_size_exceeds_remaining_capacity)
{
    auto const allocation = sut.try_allocate(page_size + 1, 1);

    expect_allocation_failed(allocation);
}

TEST_F(page_test_t, allocation_is_correctly_aligned)
{
    // deliberately misalign internal pointer
    sut.try_allocate(1, 1);

    // allocate with specific alignment
    constexpr std::size_t alignment = 64;
    auto allocation = sut.try_allocate(128, alignment);

    // allocation succeeds
    expect_allocation_succeeded(allocation);

    // the returned pointer is correctly aligned
    ASSERT_EQ(reinterpret_cast<uintptr_t>(allocation) & (alignment - 1), 0);
}

TEST_F(page_test_t, allocate_zero_bytes_succeeds_and_advances_pointer)
{
    // allocate zero bytes
    auto zero_byte_allocation = sut.try_allocate(0, 1);

    // allocate again; beginning of next allocation is end of previous
    auto next_allocation = sut.try_allocate(1, 1);

    // both allocations succeed
    expect_allocation_succeeded(zero_byte_allocation);
    expect_allocation_succeeded(next_allocation);

    // allocation size is at least 1
    ASSERT_GE(reinterpret_cast<uintptr_t>(next_allocation), reinterpret_cast<uintptr_t>(zero_byte_allocation) + 1);
}

TEST_F(page_test_t, sequential_allocations_are_contiguous)
{
    constexpr auto size = std::size_t{32};

    // make two sequential allocations
    auto allocation1 = sut.try_allocate(size, 8);
    auto allocation2 = sut.try_allocate(size * 2, 8);

    // both allocations succeed
    expect_allocation_succeeded(allocation1);
    expect_allocation_succeeded(allocation2);

    // allocations are directly adjacent; second pointer immediately follows first
    auto const expected = reinterpret_cast<uintptr_t>(allocation1) + size;
    ASSERT_EQ(reinterpret_cast<uintptr_t>(allocation2), expected);
}

} // namespace
} // namespace dink
