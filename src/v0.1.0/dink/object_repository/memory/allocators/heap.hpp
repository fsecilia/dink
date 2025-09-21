/*!
    \file
    Copyright (C) 2025 Frank Secilia
*/

#pragma once

#include <dink/lib.hpp>
#include <dink/object_repository/memory/alignment.hpp>
#include <cassert>
#include <cstddef>
#include <memory>
#include <new>

namespace dink::heap {

//! aligned heap allocator; returns unique_ptrs
template <typename allocation_deleter_t, typename api_t>
class allocator_t
{
public:
    //! unique_ptr with custom deleter
    using allocation_t = std::unique_ptr<void, allocation_deleter_t>;

    /*!
        allocates from heap using std::malloc

        \returns unique_ptr with stateless deleter
        \throws std::bad_alloc if allocation fails
    */
    [[nodiscard]] auto allocate(std::size_t size, allocation_deleter_t allocation_deleter = {}) const -> allocation_t
    {
        auto result = allocation_t{api_.malloc(size), std::move(allocation_deleter)};
        if (!result) throw std::bad_alloc{};
        return result;
    }

    /*!
        allocates from heap using std::aligned_alloc

        \pre align_val is nonzero power of two
        \pre size is multiple of align_val
        \returns unique_ptr with stateless deleter
        \throws std::bad_alloc if allocation fails
    */
    [[nodiscard]] auto allocate(
        std::size_t size, std::align_val_t align_val, allocation_deleter_t allocation_deleter = {}
    ) const -> allocation_t
    {
        assert(is_valid_aligned_request(size, align_val));

        auto result = allocation_t{
            api_.aligned_alloc(static_cast<std::size_t>(align_val), size), std::move(allocation_deleter)
        };
        if (!result) throw std::bad_alloc{};
        return result;
    }

    explicit allocator_t(api_t api) noexcept : api_{std::move(api)} {}

private:
    [[no_unique_address]] api_t api_{};
};

//! deletes a heap allocation using free; stateless
struct allocation_deleter_t
{
    auto operator()(void* allocation) const noexcept { std::free(allocation); }
};

//! default api for heap_allocator_t
struct allocator_api_t
{
    [[nodiscard]] auto malloc(std::size_t size) const noexcept -> void* { return std::malloc(size); }
    [[nodiscard]] auto aligned_alloc(std::size_t alignment, std::size_t size) const noexcept -> void*
    {
        return std::aligned_alloc(alignment, size);
    }
};

} // namespace dink::heap
