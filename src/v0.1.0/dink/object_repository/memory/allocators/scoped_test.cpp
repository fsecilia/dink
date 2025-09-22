/*!
    \file
    Copyright (C) 2025 Frank Secilia
*/

#include "scoped.hpp"
#include <dink/test.hpp>

namespace dink::scoped {
namespace {

struct scoped_node_factory_test_t : Test
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
        MOCK_METHOD(allocation_t, allocate, (std::size_t size));
        virtual ~mock_node_allocator_t() = default;
    };
    StrictMock<mock_node_allocator_t> mock_node_allocator;

    struct node_allocator_t
    {
        auto allocate(std::size_t size) -> allocation_t { return mock_node_allocator->allocate(size); }

        mock_node_allocator_t* mock_node_allocator = nullptr;
    };

    struct node_ctor_x
    {};

    struct node_t
    {
        node_t(node_t* next, void* allocation) : next{next}, allocation{allocation}
        {
            if (throw_exception) throw node_ctor_x{};
        }

        static inline auto throw_exception = false;

        node_t* next;
        void* allocation;
    };

    struct node_deleter_t
    {
        allocation_deleter_t allocation_deleter;

        auto operator()(node_t* node) noexcept -> void { node->~node_t(); }

        node_deleter_t(allocation_deleter_t&& allocation_deleter) : allocation_deleter{std::move(allocation_deleter)} {}
        node_deleter_t() = default;
    };

    static inline constexpr auto const requested_allocation_size = std::size_t{2048};
    static inline constexpr auto const requested_allocation_align_val = std::align_val_t{512};
    static inline constexpr auto const allocation_size
        = requested_allocation_size + sizeof(node_t) + static_cast<std::size_t>(requested_allocation_align_val) - 1;
    std::array<std::byte, allocation_size> allocation{};

    struct policy_t
    {
        using node_allocator_t = node_allocator_t;
        using node_deleter_t = node_deleter_t;
        using node_t = node_t;
    };

    using sut_t = node_factory_t<policy_t>;
    sut_t sut{node_allocator_t{&mock_node_allocator}};
};

TEST_F(scoped_node_factory_test_t, construction_succeeds)
{
    auto const expected_allocation_deleter_id = allocation_deleter_t::default_id + 1;
    auto const expected_allocation_value = std::data(allocation);
    EXPECT_CALL(mock_node_allocator, allocate(allocation_size))
        .WillOnce(Return(
            ByMove(allocation_t{expected_allocation_value, allocation_deleter_t{expected_allocation_deleter_id}})
        ));

    auto const allocated_node = sut(requested_allocation_size, requested_allocation_align_val);

    ASSERT_EQ(expected_allocation_value, static_cast<void*>(allocated_node.get()));
    ASSERT_EQ(expected_allocation_deleter_id, allocated_node.get_deleter().allocation_deleter.id);
    ASSERT_EQ(nullptr, allocated_node->next);

    auto const expected_allocation_begin
        = align(expected_allocation_value + sizeof(node_t), requested_allocation_align_val);
    ASSERT_EQ(expected_allocation_begin, allocated_node->allocation);
}

TEST_F(scoped_node_factory_test_t, construction_fails_when_allocation_throws)
{
    EXPECT_CALL(mock_node_allocator, allocate(allocation_size)).WillOnce(Throw(std::bad_alloc{}));
    EXPECT_THROW((void)sut(requested_allocation_size, requested_allocation_align_val), std::bad_alloc);
}

TEST_F(scoped_node_factory_test_t, construction_fails_when_ctor_throws)
{
    EXPECT_CALL(mock_node_allocator, allocate(allocation_size))
        .WillOnce(Return(ByMove(allocation_t{std::data(allocation), allocation_deleter_t{}})));

    node_t::throw_exception = true;
    EXPECT_THROW((void)sut(requested_allocation_size, requested_allocation_align_val), node_ctor_x);
    node_t::throw_exception = false;
}

// ---------------------------------------------------------------------------------------------------------------------

struct scoped_allocator_reservation_test_t : Test
{
    void* const expected_allocation = this;

    struct node_t
    {
        void* allocation;
    };
    node_t expected_node{expected_allocation};

    using allocated_node_t = node_t*;
    allocated_node_t expected_allocated_node = &expected_node;

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

    struct policy_t
    {
        using allocated_node_t = node_t*;
        using allocator_t = allocator_t;
    };
    using sut_t = reservation_t<policy_t>;
    sut_t sut{allocator, expected_allocated_node};
};

TEST_F(scoped_allocator_reservation_test_t, allocation_returns_allocation_from_allocated_node)
{
    auto actual_allocation = sut.allocation();

    ASSERT_EQ(expected_allocation, actual_allocation);
}

TEST_F(scoped_allocator_reservation_test_t, commit_forwards_to_allocattor)
{
    EXPECT_CALL(mock_allocator, commit(Eq(expected_allocated_node)));
    sut.commit();
}

} // namespace
} // namespace dink::scoped
