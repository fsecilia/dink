/*!
    \file
    Copyright (C) 2025 Frank Secilia
*/

#pragma once

#include <dink/lib.hpp>
#include <cassert>
#include <memory>
#include <new>
#include <ranges>
#include <utility>
#include <vector>

namespace dink {

//! append-only, heterogeneous container built using type-erasure
template <typename allocator_t>
class type_erased_storage_t
{
public:
    template <typename instance_t, typename... ctor_args_t>
    auto create(std::align_val_t alignment, ctor_args_t&&... ctor_args) -> instance_t&
    {
        // add empty entry to hold instance and its dtor - the vector may reallocate and throw, so do this first
        auto& stored_instance = stored_instances_.emplace_back(
            stored_instance_t{.instance = nullptr, .untyped_dtor = &stored_instance_t::template typed_dtor<instance_t>}
        );

        try
        {
            // create actual instance in stored_instance
            return create_instance<instance_t>(stored_instance, alignment, std::forward<ctor_args_t>(ctor_args)...);
        }
        catch (...)
        {
            stored_instances_.pop_back();
            throw;
        }
    }

    template <typename instance_t, typename... ctor_args_t>
    auto create(ctor_args_t&&... ctor_args) -> instance_t&
    {
        return create<instance_t>(std::align_val_t{alignof(instance_t)}, std::forward<ctor_args_t>(ctor_args)...);
    }

    explicit type_erased_storage_t(allocator_t allocator) noexcept : allocator_{std::move(allocator)} {}

    ~type_erased_storage_t()
    {
        for (auto& stored_instance : stored_instances_ | std::views::reverse) stored_instance.destroy_instance();
    }

private:
    allocator_t allocator_{};

    struct stored_instance_t
    {
        void* instance;

        template <typename instance_t>
        static auto typed_dtor(void* instance) noexcept -> void
        {
            assert(instance);
            std::destroy_at(reinterpret_cast<instance_t*>(instance));
        }

        using untyped_dtor_t = void (*)(void*) noexcept;
        untyped_dtor_t untyped_dtor;

        auto destroy_instance() noexcept -> void { untyped_dtor(instance); }
    };
    std::vector<stored_instance_t> stored_instances_{};

    template <typename instance_t, typename... ctor_args_t>
    auto create_instance(stored_instance_t& stored_instance, std::align_val_t alignment, ctor_args_t&&... ctor_args)
        -> instance_t&
    {
        // allocate bare instance memory
        stored_instance.instance = allocator_.allocate(sizeof(instance_t), alignment);

        try
        {
            // construct instance in allocation
            return *std::construct_at(
                reinterpret_cast<instance_t*>(stored_instance.instance), std::forward<ctor_args_t>(ctor_args)...
            );
        }
        catch (...)
        {
            allocator_.roll_back();
            throw;
        }
    }
};

} // namespace dink
