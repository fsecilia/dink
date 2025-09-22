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

        // lay out node followed by manually aligned request
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

/*!
    command to commit scoped allocation after reserving it

    This type is the \ref reservation for the scoped allocator.
*/
template <typename policy_t>
class reservation_t
{
public:
    using allocated_node_t = policy_t::allocated_node_t;
    using allocator_t = policy_t::allocator_t;

    auto allocation() const noexcept -> void* { return allocated_node_->allocation; }
    auto commit() noexcept -> void { allocator_->commit(std::move(allocated_node_)); }

    reservation_t(allocator_t& allocator, allocated_node_t allocated_node) noexcept
        : allocator_{&allocator}, allocated_node_{std::move(allocated_node)}
    {}

private:
    allocator_t* allocator_;
    allocated_node_t allocated_node_;
};

//! tracks allocations internally, freeing them on destruction
template <typename policy_t>
class allocator_t
{
public:
    using allocation_list_t = policy_t::allocation_list_t;
    using allocated_node_t = policy_t::allocated_node_t;
    using node_factory_t = policy_t::node_factory_t;
    using reservation_t = policy_t::reservation_t;

    auto reserve(std::size_t size, std::align_val_t align_val) -> reservation_t
    {
        return reservation_t{this, node_factory_(size, align_val)};
    }

    auto commit(allocated_node_t allocated_node) noexcept -> void { allocation_list_.push(std::move(allocated_node)); }

    explicit allocator_t(node_factory_t node_factory, allocation_list_t allocation_list = allocation_list_t{}) noexcept
        : node_factory_{std::move(node_factory)}, allocation_list_{std::move(allocation_list)}
    {}

private:
    node_factory_t node_factory_;
    allocation_list_t allocation_list_{};
};

} // namespace dink::scoped
