/*!
    \file
    Copyright (C) 2025 Frank Secilia
*/

#include <dink/lib.hpp>
#include <algorithm>
#include <cassert>
#include <memory>
#include <utility>

namespace dink {

//! deletes a list of nodes, tail to head, destroying each and freeing its underlying allocation
template <typename node_t, typename allocation_deleter_t>
struct node_deleter_t
{
    auto operator()(node_t* tail) const noexcept -> void
    {
        while (tail)
        {
            auto prev = tail->prev;

            // destroy instance
            tail->~node_t();

            // free allocation
            allocation_deleter(tail);

            tail = prev;
        }
    }

    [[no_unique_address]] allocation_deleter_t allocation_deleter;
};

//! append-only, node-based, intrusive list of owned allocations
template <typename node_t, typename node_deleter_t>
class allocation_list_t
{
public:
    using allocated_node_t = std::unique_ptr<node_t, node_deleter_t>;

    auto push(allocated_node_t&& node) noexcept -> void
    {
        assert(node != nullptr);
        node->prev = tail_.release();
        tail_ = std::move(node);
    }

    auto back() const noexcept -> node_t const&
    {
        assert(tail_ != nullptr);
        return *tail_;
    }

    auto back() noexcept -> node_t& { return const_cast<node_t&>(static_cast<allocation_list_t const&>(*this).back()); }

    explicit allocation_list_t(allocated_node_t&& tail) noexcept : tail_{std::move(tail)} {}

    allocation_list_t() = default;

private:
    allocated_node_t tail_{nullptr, node_deleter_t{}};
};

} // namespace dink
