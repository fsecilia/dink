/*!
    \file
    Copyright (C) 2025 Frank Secilia
*/

#include <dink/lib.hpp>
#include <dink/object_repository/memory/deleters.hpp>
#include <algorithm>
#include <cassert>
#include <memory>
#include <utility>

namespace dink {

//! applies deleter to each node in an intrusive list, from tail to head
template <typename node_t, typename deleter_t>
struct chained_node_deleter_t
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

//! append-only, intrusive list of owned nodes
template <typename node_t, typename node_deleter_t>
class intrusive_list_t
{
public:
    using owned_node_t = std::unique_ptr<node_t, node_deleter_t>;

    auto push(owned_node_t&& node) noexcept -> void
    {
        assert(node != nullptr);

        // wrap deleter
        auto chained_node = chained_node_t{node.get(), chained_node_deleter_t{std::move(node).get_deleter()}};

        // link
        node->prev = tail_.release();
        node.release();
        tail_ = std::move(chained_node);
    }

    auto back() const noexcept -> node_t const&
    {
        assert(tail_ != nullptr);
        return *tail_;
    }

    auto back() noexcept -> node_t& { return const_cast<node_t&>(static_cast<intrusive_list_t const&>(*this).back()); }

    explicit intrusive_list_t(node_deleter_t node_deleter = {}) noexcept
        : tail_{nullptr, chained_node_deleter_t{std::move(node_deleter)}}
    {}

private:
    using chained_node_deleter_t = chained_node_deleter_t<node_t, node_deleter_t>;
    using chained_node_t = std::unique_ptr<node_t, chained_node_deleter_t>;
    chained_node_t tail_{nullptr, chained_node_deleter_t{}};
};

} // namespace dink
