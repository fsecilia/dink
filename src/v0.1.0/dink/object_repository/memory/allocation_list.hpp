/*!
    \file
    Copyright (C) 2025 Frank Secilia
*/

#include <algorithm>
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

} // namespace dink
