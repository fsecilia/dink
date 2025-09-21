/*!
    \file
    Copyright (C) 2025 Frank Secilia
*/

#include <dink/lib.hpp>
#include <dink/object_repository/memory/alignment.hpp>
#include <algorithm>
#include <cassert>
#include <new>

namespace dink::page {

/*!
    command to commit allocation from a page after reserving it

    This type is the \ref pending_allocation for the page allocator.
*/
template <typename allocator_t>
class pending_allocation_t
{
public:
    auto allocation() const noexcept -> void* { return allocation_begin_; }
    auto commit() noexcept -> void { allocator_->commit(allocation_end_); }

    pending_allocation_t(allocator_t& allocator, void* allocation_begin, void* allocation_end) noexcept
        : allocator_{&allocator}, allocation_begin_{std::move(allocation_begin)}, allocation_end_{allocation_end}
    {}

private:
    allocator_t* allocator_;
    void* allocation_begin_;
    void* allocation_end_;
};

/*!
    allocates from within a region of memory

    This type uses a \ref pending_allocation.
*/
template <template <typename> class pending_allocation_p>
class allocator_t
{
public:
    using pending_allocation_t = pending_allocation_p<allocator_t>;

    //! maximum allocation size to balance amortization against internal fragmentation
    auto max_allocation_size() const noexcept -> std::size_t { return max_allocation_size_; }

    //! \pre align_val is nonzero power of two
    [[nodiscard]] auto reserve(std::size_t size, std::align_val_t align_val) noexcept -> pending_allocation_t
    {
        assert(is_valid_alignment(align_val));

        // round empty requests up to 1-byte requests so they still get unique addresses
        size = std::max(size, std::size_t{1});

        // make sure worst-case alignment is smaller than limit
        auto const alignment = static_cast<size_t>(align_val);
        auto const total_size_exceeds_limit = size + alignment - 1 > max_allocation_size_;
        if (total_size_exceeds_limit) return pending_allocation_t{*this, nullptr, nullptr};

        // find next aligned location
        auto const allocation_begin = align(cur_, align_val);

        // make sure the aligned allocation fits in remaining space
        auto const size_remaining = static_cast<std::size_t>(end_ - allocation_begin);
        auto const end_exceeds_buffer = size_remaining < size;
        if (end_exceeds_buffer) return pending_allocation_t{*this, nullptr, nullptr};

        auto const allocation_end = allocation_begin + size;
        return pending_allocation_t{*this, allocation_begin, allocation_end};
    }

    auto commit(void* allocation_end) noexcept -> void { cur_ = static_cast<std::byte*>(allocation_end); }

    allocator_t(void* begin, std::size_t size, std::size_t max_allocation_size) noexcept
        : cur_{static_cast<std::byte*>(begin)}, end_{cur_ + size}, max_allocation_size_{max_allocation_size}
    {}

private:
    std::byte* cur_;
    std::byte* end_;
    std::size_t max_allocation_size_;
};

} // namespace dink::page
