/*!
    \file
    Copyright (C) 2025 Frank Secilia
*/

#include "intrusive_list.hpp"
#include <dink/test.hpp>

namespace dink {
namespace {

struct chained_node_deleter_test_t : Test
{
    struct node_t
    {
        node_t* prev = nullptr;
    };
    node_t nodes[3] = {nullptr, &nodes[0], &nodes[1]};

    struct mock_deleter_t
    {
        MOCK_METHOD(void, call, (node_t * node), (const, noexcept));
        virtual ~mock_deleter_t() = default;
    };

    struct deleter_t
    {
        auto operator()(node_t* node) const noexcept -> void { mock->call(node); }
        mock_deleter_t* mock = nullptr;
    };

    StrictMock<mock_deleter_t> mock_deleter{};

    using sut_t = chained_node_deleter_t<node_t, deleter_t>;
    sut_t sut{deleter_t{&mock_deleter}};

    InSequence seq; // all tests here are ordered
};

TEST_F(chained_node_deleter_test_t, call_operator_with_zero_nodes_is_no_op)
{
    sut(nullptr);
}

TEST_F(chained_node_deleter_test_t, call_operator_with_one_node_deletes_node)
{
    EXPECT_CALL(mock_deleter, call(&nodes[0]));
    sut(&nodes[0]);
}

TEST_F(chained_node_deleter_test_t, call_operator_with_two_nodes_deletes_in_reverse_order)
{
    EXPECT_CALL(mock_deleter, call(&nodes[1]));
    EXPECT_CALL(mock_deleter, call(&nodes[0]));
    sut(&nodes[1]);
}

TEST_F(chained_node_deleter_test_t, call_operator_with_three_nodes_deletes_in_reverse_order)
{
    EXPECT_CALL(mock_deleter, call(&nodes[2]));
    EXPECT_CALL(mock_deleter, call(&nodes[1]));
    EXPECT_CALL(mock_deleter, call(&nodes[0]));
    sut(&nodes[2]);
}

// ---------------------------------------------------------------------------------------------------------------------

struct intrusive_list_test_t : Test
{
    struct node_t
    {
        node_t* prev = nullptr;
    };
    node_t nodes[2] = {};

    struct mock_node_deleter_t
    {
        MOCK_METHOD(void, call, (node_t * node), (const, noexcept));
        virtual ~mock_node_deleter_t() = default;
    };
    StrictMock<mock_node_deleter_t> mock_node_deleter{};

    struct node_deleter_t
    {
        auto operator()(node_t* node) const noexcept -> void
        {
            if (!mock) return;
            mock->call(node);
        }

        mock_node_deleter_t* mock = nullptr;
    };

    using sut_t = intrusive_list_t<node_t, node_deleter_t>;
    using allocated_node_t = sut_t::owned_node_t;
};

// ---------------------------------------------------------------------------------------------------------------------

struct intrusive_list_test_empty_t : intrusive_list_test_t
{
    sut_t sut{};
};

TEST_F(intrusive_list_test_empty_t, push_adds_initial_node_which_becomes_new_back)
{
    sut.push(allocated_node_t{&nodes[0], node_deleter_t{&mock_node_deleter}});

    ASSERT_EQ(&nodes[0], &sut.back());

    EXPECT_CALL(mock_node_deleter, call(&nodes[0]));
}

// ---------------------------------------------------------------------------------------------------------------------

struct intrusive_list_test_populated_t : intrusive_list_test_t
{
    sut_t sut{};

    intrusive_list_test_populated_t() noexcept
    {
        sut.push({allocated_node_t{&nodes[0], node_deleter_t{&mock_node_deleter}}});
    }
};

TEST_F(intrusive_list_test_populated_t, back_returns_initial_node)
{
    ASSERT_EQ(&nodes[0], &sut.back());
    ASSERT_EQ(&nodes[0], &static_cast<sut_t const&>(sut).back());

    EXPECT_CALL(mock_node_deleter, call(&nodes[0]));
}

TEST_F(intrusive_list_test_populated_t, push_adds_second_node_which_becomes_new_back_and_links_prev)
{
    sut.push(allocated_node_t{&nodes[1], node_deleter_t{&mock_node_deleter}});

    ASSERT_EQ(&nodes[1], &sut.back());
    ASSERT_EQ(&nodes[1], &static_cast<sut_t const&>(sut).back());

    ASSERT_EQ(&nodes[0], nodes[1].prev);

    EXPECT_CALL(mock_node_deleter, call(&nodes[1]));
    EXPECT_CALL(mock_node_deleter, call(&nodes[0]));
}

} // namespace
} // namespace dink
