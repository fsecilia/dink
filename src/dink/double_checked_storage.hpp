/*!
    \file
    Copyright (C) 2025 Frank Secilia
*/

#pragma once

#include <dink/lib.hpp>
#include <atomic>
#include <memory>
#include <mutex>
#include <utility>

namespace dink {

//! initializes an instance using the double-checked locking pattern
template <typename instance_t>
struct double_checked_storage_t {
public:
    //! returns cached instance or creates it using factory
    template <typename factory_t>
    auto get_or_create(factory_t&& factory) -> instance_t& {
        // try loading outside of lock
        auto* result = atomic.load(std::memory_order_acquire);
        if (result != nullptr) return *result;

        std::lock_guard lock(creation_mutex);

        // try loading from inside of lock
        result = atomic.load(std::memory_order_relaxed);
        if (result != nullptr) return *result;

        // couldn't get, so create
        return create(std::forward<factory_t>(factory));
    }

    //! returns cached instance or nullptr if not yet initialized
    auto get() const noexcept -> instance_t* { return atomic.load(std::memory_order_acquire); }

private:
    //! instance is stored in a union so we can control when ctor and dtor are called without using heap
    union union_t {
        union_t() {}
        ~union_t() {}
        instance_t instance;
    };
    union_t storage;

    //! instance is tracked in a unique_ptr with a custom deleter for union
    std::unique_ptr<instance_t, void (*)(instance_t*)> instance{nullptr, [](instance_t* p) { p->~instance_t(); }};

    //! instance is replicated into an atomic when valid
    std::atomic<instance_t*> atomic{nullptr};

    //! the double-checked locking pattern requires a mutex when creating the instance
    std::mutex creation_mutex;

    //! creates instance under lock, replicates to atomic
    template <typename factory_t>
    auto create(factory_t&& factory) -> instance_t& {
        // find where to put instance
        auto* storage_ptr = &storage.instance;

        // call ctor
        new (storage_ptr) instance_t(std::forward<factory_t>(factory)());

        // capture instance
        instance.reset(storage_ptr);

        // replicate into atomic
        atomic.store(storage_ptr, std::memory_order_release);

        return *instance;
    }
};

}  // namespace dink
