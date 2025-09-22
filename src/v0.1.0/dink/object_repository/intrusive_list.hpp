/*!
    \file
    Copyright (C) 2025 Frank Secilia
*/

#include <dink/lib.hpp>
#include <algorithm>
#include <cassert>
#include <memory>
#include <utility>

#include <tuple>

namespace dink {

//! applies deleter to each node in an intrusive list, from tail to head
template <typename node_t, typename deleter_t>
struct chained_deleter_t
{
    auto operator()(node_t* tail) const noexcept -> void
    {
        while (tail)
        {
            auto prev = tail->prev;
            deleter(tail);
            tail = prev;
        }
    }

    [[no_unique_address]] deleter_t deleter;
};

//! applies each deleter, in order, to a given node
template <typename node_t, typename... deleters_t>
struct composite_deleter_t
{
    auto operator()(node_t* node) const noexcept -> void
    {
        std::apply([node](auto const&... deleter) { (deleter(node), ...); }, deleters);
    }

    composite_deleter_t(deleters_t... deleters) : deleters{std::move(deleters)...} {}

    [[no_unique_address]] std::tuple<deleters_t...> deleters;
};

// destroys given instance
template <typename node_t>
struct destroy_node_deleter_t
{
    auto operator()(node_t* node) const noexcept -> void { node->~node_t(); }
};

template <typename node_t, typename allocation_deleter_t>
using allocated_node_deleter_t
    = chained_deleter_t<node_t, composite_deleter_t<node_t, destroy_node_deleter_t<node_t>, allocation_deleter_t>>;

//! append-only, intrusive list of owned nodes
template <typename node_t, typename node_deleter_t>
class intrusive_list_t
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

    auto back() noexcept -> node_t& { return const_cast<node_t&>(static_cast<intrusive_list_t const&>(*this).back()); }

    explicit intrusive_list_t(allocated_node_t&& tail) noexcept : tail_{std::move(tail)} {}
    explicit intrusive_list_t(node_deleter_t node_deleter = {}) noexcept : tail_{nullptr, std::move(node_deleter)} {}

private:
    allocated_node_t tail_{nullptr, node_deleter_t{}};
};

} // namespace dink
