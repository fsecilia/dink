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

    struct mock_node_allocator_t
    {
        MOCK_METHOD(allocation_t, allocate, (std::size_t size, std::align_val_t align_val));
        virtual ~mock_node_allocator_t() = default;
    };
    StrictMock<mock_node_allocator_t> mock_node_allocator;

    struct node_allocator_t
    {
        auto allocate(std::size_t size, std::align_val_t align_val) -> allocation_t
        {
            return mock_node_allocator->allocate(size, align_val);
        }

        mock_node_allocator_t* mock_node_allocator = nullptr;
    };

    struct page_ctor_x
    {};

    struct page_t
    {
        page_t(void* begin, std::size_t size) : begin{begin}, size{size}
        {
            if (throw_exception) throw page_ctor_x{};
        }

        static inline auto throw_exception = false;

        void* begin;
        std::size_t size;
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

    static inline constexpr auto const page_size = std::size_t{2048};
    static inline constexpr auto const page_align_val = std::align_val_t{512};
    alignas(static_cast<std::size_t>(page_align_val)) std::array<std::byte, page_size> page_allocation{};

    struct policy_t
    {
        using node_allocator_t = node_allocator_t;
        using node_deleter_t = node_deleter_t;
        using node_t = node_t;
        using page_t = page_t;
    };

    using sut_t = node_factory_t<policy_t>;
    sut_t sut{node_allocator_t{&mock_node_allocator}};
};

TEST_F(paged_node_factory_test_t, construction_succeeds)
{
    auto const expected_allocation_deleter_id = allocation_deleter_t::default_id + 1;
    auto const expected_allocation_value = std::data(page_allocation);
    EXPECT_CALL(mock_node_allocator, allocate(page_size, page_align_val))
        .WillOnce(Return(
            ByMove(allocation_t{expected_allocation_value, allocation_deleter_t{expected_allocation_deleter_id}})
        ));

    auto const allocated_node = sut(page_size, page_align_val);

    ASSERT_EQ(expected_allocation_value, static_cast<void*>(allocated_node.get()));
    ASSERT_EQ(expected_allocation_deleter_id, allocated_node.get_deleter().allocation_deleter.id);
    ASSERT_EQ(nullptr, allocated_node->next);

    auto const expected_page_begin = expected_allocation_value + sizeof(node_t);
    auto const expected_page_size = page_size - sizeof(node_t);
    ASSERT_EQ(expected_page_begin, allocated_node->page.begin);
    ASSERT_EQ(expected_page_size, allocated_node->page.size);
}

TEST_F(paged_node_factory_test_t, construction_fails_when_allocation_throws)
{
    EXPECT_CALL(mock_node_allocator, allocate(page_size, page_align_val)).WillOnce(Throw(std::bad_alloc{}));
    EXPECT_THROW((void)sut(page_size, page_align_val), std::bad_alloc);
}

TEST_F(paged_node_factory_test_t, construction_fails_when_ctor_throws)
{
    EXPECT_CALL(mock_node_allocator, allocate(page_size, page_align_val))
        .WillOnce(Return(ByMove(allocation_t{std::data(page_allocation), allocation_deleter_t{}})));

    page_t::throw_exception = true;
    EXPECT_THROW((void)sut(page_size, page_align_val), page_ctor_x);
    page_t::throw_exception = false;
}

// ---------------------------------------------------------------------------------------------------------------------

struct paged_allocator_reservation_test_t : Test
{
    using allocated_node_t = int_t;
    static inline allocated_node_t expected_allocated_node = 3;

    struct mock_allocator_t
    {
        MOCK_METHOD(void, commit, (allocated_node_t&&), (noexcept));
        virtual ~mock_allocator_t() = default;
    };
    StrictMock<mock_allocator_t> mock_allocator;

    struct allocator_t
    {
        auto commit(allocated_node_t&& allocated_node) noexcept -> void { mock->commit(std::move(allocated_node)); }
        mock_allocator_t* mock = nullptr;
    };
    allocator_t allocator{&mock_allocator};

    struct mock_page_reservation_t
    {
        MOCK_METHOD(void*, allocation, (), (const, noexcept));
        MOCK_METHOD(void, commit, (), (noexcept));
        virtual ~mock_page_reservation_t() = default;
    };
    StrictMock<mock_page_reservation_t> mock_page_reservation;

    struct page_reservation_t
    {
        auto allocation() const noexcept -> void* { return mock->allocation(); }
        auto commit() noexcept -> void { mock->commit(); }
        mock_page_reservation_t* mock = nullptr;
    };

    struct policy_t
    {
        using allocated_node_t = allocated_node_t;
        using allocator_t = allocator_t;
        using page_reservation_t = page_reservation_t;
    };
    using sut_t = reservation_t<policy_t>;
    sut_t sut{allocator, page_reservation_t{&mock_page_reservation}, expected_allocated_node};
};

TEST_F(paged_allocator_reservation_test_t, allocation_returns_page_reservation_allocation)
{
    auto const expected_allocation = this;
    EXPECT_CALL(mock_page_reservation, allocation()).WillOnce(Return(expected_allocation));

    auto actual_allocation = sut.allocation();

    ASSERT_EQ(expected_allocation, actual_allocation);
}

TEST_F(paged_allocator_reservation_test_t, commit_forwards_to_allocation_and_page_reservation)
{
    EXPECT_CALL(mock_allocator, commit(Eq(expected_allocated_node)));
    EXPECT_CALL(mock_page_reservation, commit());
    sut.commit();
}

// ---------------------------------------------------------------------------------------------------------------------

struct paged_allocator_test_t : Test
{
    struct page_reservation_t
    {
        void* expected_allocation;
        auto allocation() const noexcept -> void* { return expected_allocation; }
    };

    struct mock_page_t
    {
        MOCK_METHOD(page_reservation_t, reserve, (std::size_t size, std::align_val_t alignment), (noexcept));
        MOCK_METHOD(void, commit, (), (noexcept));

        virtual ~mock_page_t() = default;
    };
    StrictMock<mock_page_t> initial_mock_page;
    StrictMock<mock_page_t> new_mock_page;

    struct page_t
    {
        auto reserve(std::size_t size, std::align_val_t alignment) noexcept -> page_reservation_t
        {
            return mock->reserve(size, alignment);
        }

        auto commit() noexcept -> void { mock->commit(); }
        mock_page_t* mock = nullptr;
    };

    struct node_t
    {
        page_t page;
    };
    node_t initial_node = node_t{page_t{&initial_mock_page}};
    node_t new_node = node_t{page_t{&new_mock_page}};

    using allocated_node_t = node_t*;

    struct allocation_list_t
    {
        allocated_node_t tail;
        auto back() const noexcept -> node_t& { return pushed ? *pushed : *tail; }

        allocated_node_t pushed = nullptr;
        auto push(allocated_node_t new_node) noexcept -> void { pushed = new_node; }
    };

    struct mock_node_factory_t
    {
        MOCK_METHOD(allocated_node_t, call, (std::size_t page_size, std::align_val_t page_alignment));

        virtual ~mock_node_factory_t() = default;
    };
    StrictMock<mock_node_factory_t> mock_node_factory;

    struct node_factory_t
    {
        auto operator()(std::size_t page_size, std::align_val_t page_alignment) -> allocated_node_t
        {
            return mock->call(page_size, page_alignment);
        }

        mock_node_factory_t* mock = nullptr;
    };

    static inline constexpr auto const os_page_size = std::size_t{512};
    static inline constexpr auto const page_size = std::size_t{1024};
    static inline constexpr auto const max_allocation_size = std::size_t{256};
    struct page_size_config_t
    {
        std::size_t os_page_size = paged_allocator_test_t::os_page_size;
        std::size_t page_size = paged_allocator_test_t::page_size;
        std::size_t max_allocation_size = paged_allocator_test_t::max_allocation_size;
    };

    struct policy_t;
    using sut_t = allocator_t<policy_t>;

    struct reservation_t
    {
        sut_t* allocator;
        page_reservation_t page_reservation;
        allocated_node_t new_node;
    };

    struct policy_t
    {
        using allocated_node_t = allocated_node_t;
        using allocation_list_t = allocation_list_t;
        using node_t = node_t;
        using node_factory_t = node_factory_t;
        using page_t = page_t;
        using page_size_config_t = page_size_config_t;
        using reservation_t = reservation_t;
    };

    sut_t sut = [this]() {
        EXPECT_CALL(mock_node_factory, call(page_size, std::align_val_t{os_page_size})).WillOnce(Return(&initial_node));
        return sut_t{node_factory_t{&mock_node_factory}, page_size_config_t{}};
    }();

    void* const expected_allocation = this;
    std::size_t const expected_reserve_size{53};
    std::align_val_t const expected_reserve_alignment{16};
};

TEST_F(paged_allocator_test_t, max_allocation_size)
{
    ASSERT_EQ(max_allocation_size, sut.max_allocation_size());
}

TEST_F(paged_allocator_test_t, reserve_from_current_page_succeeds)
{
    EXPECT_CALL(initial_mock_page, reserve(expected_reserve_size, expected_reserve_alignment))
        .WillOnce(Return(page_reservation_t{expected_allocation}));

    auto const result = sut.reserve(expected_reserve_size, expected_reserve_alignment);

    ASSERT_EQ(&sut, result.allocator);
    ASSERT_EQ(expected_allocation, result.page_reservation.allocation());
    ASSERT_EQ(nullptr, result.new_node);
}

TEST_F(paged_allocator_test_t, reserve_from_current_page_fails_then_from_new_page_succeeds)
{
    EXPECT_CALL(initial_mock_page, reserve(expected_reserve_size, expected_reserve_alignment))
        .WillOnce(Return(page_reservation_t{nullptr}));
    EXPECT_CALL(mock_node_factory, call(page_size, std::align_val_t{os_page_size})).WillOnce(Return(&new_node));
    EXPECT_CALL(new_mock_page, reserve(expected_reserve_size, expected_reserve_alignment))
        .WillOnce(Return(page_reservation_t{expected_allocation}));

    auto const result = sut.reserve(expected_reserve_size, expected_reserve_alignment);

    ASSERT_EQ(&sut, result.allocator);
    ASSERT_EQ(expected_allocation, result.page_reservation.allocation());
    ASSERT_EQ(&new_node, result.new_node);
}

TEST_F(paged_allocator_test_t, reserve_from_current_page_fails_then_create_node_throws_exception)
{
    EXPECT_CALL(initial_mock_page, reserve(expected_reserve_size, expected_reserve_alignment))
        .WillOnce(Return(page_reservation_t{nullptr}));
    EXPECT_CALL(mock_node_factory, call(page_size, std::align_val_t{os_page_size})).WillOnce(Throw(std::bad_alloc{}));

    EXPECT_THROW((void)sut.reserve(expected_reserve_size, expected_reserve_alignment), std::bad_alloc);
}

TEST_F(paged_allocator_test_t, commit_pushes_new_page_onto_allocation_list)
{
    sut.commit(&new_node);

    // the only way to observe the result of commit is to try and allocate and see that it comes from the new node
    EXPECT_CALL(new_mock_page, reserve(expected_reserve_size, expected_reserve_alignment))
        .WillOnce(Return(page_reservation_t{expected_allocation}));

    (void)sut.reserve(expected_reserve_size, expected_reserve_alignment);
}

TEST_F(paged_allocator_test_t, commit_empty_page_is_no_op)
{
    sut.commit(allocated_node_t{nullptr});
}

} // namespace
} // namespace dink::paged
