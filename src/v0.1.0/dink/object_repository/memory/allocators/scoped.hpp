/*!
    \file
    Copyright (C) 2025 Frank Secilia
*/

#pragma once

#include <dink/lib.hpp>
#include <dink/object_repository/memory/alignment.hpp>
#include <dink/object_repository/memory/allocators/allocators.hpp>
#include <dink/object_repository/memory/construct_in_allocation.hpp>
#include <cassert>
#include <cstddef>
#include <memory>
#include <utility>

namespace dink::scoped {

struct node_t
{
    node_t* next;
    void* allocation;
};

//! creates scoped nodes in an allocation large enough to hold the node out of band before the requested allocation
template <typename policy_t>
class node_factory_t
{
public:
    using node_allocator_t = policy_t::node_allocator_t;
    using node_deleter_t = policy_t::node_deleter_t;
    using node_t = policy_t::node_t;

    using allocated_node_t = std::unique_ptr<node_t, node_deleter_t>;

    auto operator()(std::size_t size, std::align_val_t align_val) -> allocated_node_t
    {
        // request oversized allocation
        auto const alignment = static_cast<std::size_t>(align_val);
        auto allocation = node_allocator_.allocate(size + sizeof(node_t) + alignment - 1);

        // lay out node and then manually aligned request
        auto const node_address = reinterpret_cast<std::byte*>(std::to_address(allocation));
        auto const aligned_allocation = reinterpret_cast<void*>(
            (reinterpret_cast<uintptr_t>(node_address) + sizeof(node_t) + alignment - 1) & -alignment
        );

        // construct node at beginning of allocation
        return construct_in_allocation<node_t, node_deleter_t>(
            std::move(allocation), node_t{nullptr, aligned_allocation}
        );
    }

    explicit node_factory_t(node_allocator_t node_allocator) noexcept : node_allocator_{std::move(node_allocator)} {}

private:
    node_allocator_t node_allocator_;
};

} // namespace dink::scoped
