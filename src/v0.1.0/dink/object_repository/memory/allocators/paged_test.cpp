/*!
    \file
    Copyright (C) 2025 Frank Secilia
*/

#include "paged.hpp"
#include <dink/test.hpp>
#include <array>

namespace dink::paged {
namespace {

struct paged_node_factory_test_t : Test
{
    struct allocation_deleter_t
    {
        auto operator()(void*) noexcept -> void {}

        static auto const default_id = int_t{3};
        int_t id = default_id;
    };
    using allocation_t = std::unique_ptr<void, allocation_deleter_t>;

    struct mock_allocator_t
    {
        MOCK_METHOD(allocation_t, allocate, (std::size_t size, std::align_val_t align_val));
        virtual ~mock_allocator_t() = default;
    };
    StrictMock<mock_allocator_t> mock_allocator;

    struct allocator_t
    {
        auto allocate(std::size_t size, std::align_val_t align_val) -> allocation_t
        {
            return mock_allocator->allocate(size, align_val);
        }

        mock_allocator_t* mock_allocator = nullptr;
    };

    struct page_ctor_x
    {};

    struct page_t
    {
        page_t(void* begin, std::size_t size, std::size_t max_allocation_size)
            : begin{begin}, size{size}, max_allocation_size{max_allocation_size}
        {
            if (throw_exception) throw page_ctor_x{};
        }

        static inline auto throw_exception = false;

        void* begin;
        std::size_t size;
        std::size_t max_allocation_size;
    };

    struct node_t
    {
        node_t* next;
        page_t page;
    };

    struct node_deleter_t
    {
        allocation_deleter_t allocation_deleter;

        auto operator()(node_t* node) noexcept -> void { node->~node_t(); }

        node_deleter_t(allocation_deleter_t&& allocation_deleter) : allocation_deleter{std::move(allocation_deleter)} {}
        node_deleter_t() = default;
    };

    static inline constexpr auto const os_page_size = std::size_t{512};
    static inline constexpr auto const page_size = std::size_t{2048};
    static inline constexpr auto const max_allocation_size = std::size_t{256};
    std::array<std::byte, page_size> page_allocation{};

    struct page_size_config_t
    {
        std::size_t os_page_size = paged_node_factory_test_t::os_page_size;
        std::size_t page_size = paged_node_factory_test_t::page_size;
        std::size_t max_allocation_size = paged_node_factory_test_t::max_allocation_size;
    };

    struct policy_t
    {
        using allocator_t = allocator_t;
        using node_deleter_t = node_deleter_t;
        using node_t = node_t;
        using page_size_config_t = page_size_config_t;
        using page_t = page_t;
    };

    using sut_t = node_factory_t<policy_t>;
    sut_t sut{allocator_t{&mock_allocator}, page_size_config_t{}};
};

TEST_F(paged_node_factory_test_t, construction_succeeds)
{
    auto const expected_allocation_deleter_id = allocation_deleter_t::default_id + 1;
    auto const expected_allocation_value = std::data(page_allocation);
    EXPECT_CALL(mock_allocator, allocate(page_size, std::align_val_t{os_page_size}))
        .WillOnce(Return(
            ByMove(allocation_t{expected_allocation_value, allocation_deleter_t{expected_allocation_deleter_id}})
        ));

    auto const paged_node = sut();

    ASSERT_EQ(expected_allocation_value, static_cast<void*>(paged_node.get()));
    ASSERT_EQ(expected_allocation_deleter_id, paged_node.get_deleter().allocation_deleter.id);
    ASSERT_EQ(nullptr, paged_node->next);

    auto const expected_page_begin = expected_allocation_value + sizeof(node_t);
    auto const expected_page_size = page_size - sizeof(node_t);
    ASSERT_EQ(expected_page_begin, paged_node->page.begin);
    ASSERT_EQ(expected_page_size, paged_node->page.size);
    ASSERT_EQ(max_allocation_size, paged_node->page.max_allocation_size);
}

TEST_F(paged_node_factory_test_t, construction_fails_when_allocation_throws)
{
    EXPECT_CALL(mock_allocator, allocate(page_size, std::align_val_t{os_page_size})).WillOnce(Throw(std::bad_alloc{}));
    EXPECT_THROW((void)sut(), std::bad_alloc);
}

TEST_F(paged_node_factory_test_t, construction_fails_when_ctor_throws)
{
    auto const expected_allocation_deleter_id = allocation_deleter_t::default_id + 1;
    auto const expected_allocation_value = static_cast<void*>(std::data(page_allocation));
    EXPECT_CALL(mock_allocator, allocate(page_size, std::align_val_t{os_page_size}))
        .WillOnce(Return(
            ByMove(allocation_t{expected_allocation_value, allocation_deleter_t{expected_allocation_deleter_id}})
        ));

    page_t::throw_exception = true;
    EXPECT_THROW((void)sut(), page_ctor_x);
}

} // namespace
} // namespace dink::paged
