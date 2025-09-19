/*!
    \file
    Copyright (C) 2025 Frank Secilia
*/

#include "allocation_list.hpp"
#include <dink/test.hpp>

namespace dink {
namespace {

struct node_deleter_test_t : Test
{
    struct node_t
    {
        node_t* prev = nullptr;
    };
    node_t nodes[3] = {nullptr, &nodes[0], &nodes[1]};

    struct mock_allocation_deleter_t
    {
        MOCK_METHOD(void, call, (node_t * node), (const, noexcept));
        virtual ~mock_allocation_deleter_t() = default;
    };
    StrictMock<mock_allocation_deleter_t> mock_allocation_deleter{};

    struct allocation_deleter_t
    {
        auto operator()(node_t* node) const noexcept -> void { mock->call(node); }
        mock_allocation_deleter_t* mock = nullptr;
    };

    using sut_t = node_deleter_t<node_t, allocation_deleter_t>;
    sut_t sut{allocation_deleter_t{&mock_allocation_deleter}};

    InSequence seq; // all tests here are ordered
};

TEST_F(node_deleter_test_t, zero_nodes)
{
    sut(nullptr);
}

TEST_F(node_deleter_test_t, one_node)
{
    EXPECT_CALL(mock_allocation_deleter, call(&nodes[0]));
    sut(&nodes[0]);
}

TEST_F(node_deleter_test_t, two_nodes)
{
    EXPECT_CALL(mock_allocation_deleter, call(&nodes[1]));
    EXPECT_CALL(mock_allocation_deleter, call(&nodes[0]));
    sut(&nodes[1]);
}

TEST_F(node_deleter_test_t, three_nodes)
{
    EXPECT_CALL(mock_allocation_deleter, call(&nodes[2]));
    EXPECT_CALL(mock_allocation_deleter, call(&nodes[1]));
    EXPECT_CALL(mock_allocation_deleter, call(&nodes[0]));
    sut(&nodes[2]);
}

} // namespace
} // namespace dink
