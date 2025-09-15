/*!
    \file
    Copyright (C) 2025 Frank Secilia
*/

#include <dink/test.hpp>
#include <new>
#include <utility>
#include <vector>

namespace dink {
namespace {

struct heap_allocator_t
{
    struct deleter_t
    {
        auto operator()(void* allocation) const noexcept { std::free(allocation); }
    };
    using allocation_t = std::unique_ptr<void, deleter_t>;

    auto allocate(std::size_t size, std::align_val_t align_val) const -> allocation_t
    {
        return allocation_t{std::aligned_alloc(size, static_cast<std::size_t>(align_val))};
    }
};

template <typename allocation_t, typename rollback_command_t>
class pending_allocation_t
{
public:
    auto result() const noexcept -> allocation_t { return result_; }

    explicit operator bool() const noexcept { return succeeded(); }
    auto succeeded() const noexcept -> bool { return result_ != nullptr; }

    auto commit() noexcept -> void { release(); }

    auto roll_back() noexcept -> void
    {
        rollback_command_();
        release();
    }

    pending_allocation_t(allocation_t result, rollback_command_t rollback_command) noexcept
        : result_{result}, rollback_command_{std::move(rollback_command)}
    {}

    ~pending_allocation_t() { try_roll_back(); }

    pending_allocation_t(pending_allocation_t const&) = delete;
    auto operator=(pending_allocation_t const&) -> allocation_t& = delete;

    pending_allocation_t(pending_allocation_t&& src) noexcept
        : result_{std::move(src).result_}, rollback_command_{std::move(src).rollback_command_}
    {
        src.release();
    }

    auto operator=(pending_allocation_t&& src) noexcept -> pending_allocation_t&
    {
        if (this == &src) return *this;

        try_roll_back();

        result_ = std::move(src).result_;
        rollback_command_ = std::move(src.rollback_command_);

        src.release();

        return *this;
    }

private:
    allocation_t result_{}; // cleared after commit, rollback, or move
    rollback_command_t rollback_command_{};

    auto try_roll_back() noexcept -> void
    {
        if (result_) roll_back();
    }

    auto release() noexcept -> void { result_ = nullptr; }
};

template <typename arena_allocator_t, typename arena_factory_t>
class arena_allocator_factory_t
{
public:
    auto operator()() -> arena_allocator_t { return arena_allocator_t{arena_factory_()}; }

    explicit arena_allocator_factory_t(arena_factory_t arena_factory) noexcept
        : arena_factory_{std::move(arena_factory)}
    {}

private:
    arena_factory_t arena_factory_;
};

struct page_size_t
{
    auto operator()() const noexcept { return 4096; }
};

template <typename allocation_t>
struct arena_t
{
    allocation_t allocation;
    std::size_t size;
};

template <typename arena_t, typename allocator_t, typename page_size_t>
class arena_factory_t
{
public:
    auto operator()() -> arena_t
    {
        return arena_t{allocator_.allocate(arena_size_, std::align_val_t{page_size_}), arena_size_};
    }

    arena_factory_t(allocator_t allocator, page_size_t page_size) noexcept
        : allocator_{std::move(allocator)}, page_size_{page_size()}
    {}

private:
    allocator_t allocator_;
    std::size_t page_size_;
    std::size_t arena_size_{page_size_ * 16};
};

template <typename arena_allocator_p, typename arena_allocator_factory_p>
struct pooled_arena_config_t
{
    using arena_allocator_t = arena_allocator_p;
    using arena_allocator_factory_t = arena_allocator_factory_p;

    using arena_allocators_t = std::vector<arena_allocator_t>;

    arena_allocator_factory_t arena_allocator_factory;
    arena_allocators_t arena_allocators{arena_allocator_factory()};
};

template <typename pooled_arena_config_t>
class pooled_arena_allocator_t
{
public:
    using arena_allocator_t = pooled_arena_config_t::arena_allocator_t;
    using arena_allocator_factory_t = pooled_arena_config_t::arena_allocator_factory_t;
    using arena_allocators_t = pooled_arena_config_t::arena_allocators_t;

    using allocation_t = arena_allocator_t::allocation_t;

    struct rollback_command_t
    {
        pooled_arena_allocator_t* allocator;
        arena_allocator_t::rollback_command arena_rollback_command;
        auto operator()() const noexcept -> void { allocator->roll_back(rollback_token); }
    };
    using pending_allocation_t = pending_allocation_t<allocation_t, rollback_command_t>;

    auto max_allocation_size() const noexcept -> std::size_t { return arena_allocator().max_allocation_size(); }

    auto allocate(std::size_t size, std::align_val_t align_val) -> pending_allocation_t
    {
        auto arena_result = arena_allocator().allocate(size, align_val);
        if (!arena_result)
        {
            arena_allocators_.push_back(arena_allocator_factory_());
            arena_result = arena_allocator().allocate(size, align_val);
        }
        return pending_allocation_t{arena_result.allocation, rollback_command_t{this, arena_result.rollback_command}};
    }

    auto roll_back(rollback_token_t const& rollback_token) noexcept -> void
    {
        rollback_token();
        if (arena_allocator().empty() && arena_allocators_.size() > 1) arena_allocators_.pop_back();
    }

    pooled_arena_allocator_t(pooled_arena_config_t pooled_arena_config)
        : arena_allocator_factory_{std::move(pooled_arena_config).arena_allocator_factory},
          arena_allocators_{std::move(pooled_arena_config).arena_allocators}
    {
        assert(!arena_allocators_.empty());
    }

private:
    arena_allocator_factory_t arena_allocator_factory_;
    arena_allocators_t arena_allocators_;

    auto arena_allocator() noexcept -> arena_allocator_t& { return arena_allocators_.back(); }
};

template <typename arena_t>
class arena_allocator_t
{
public:
    using address_t = uintptr_t;
    using rollback_token_t = address_t;
    using allocation_t = void*;

    struct rollback_command_t
    {
        arena_allocator_t* allocator;
        address_t prev;
        auto operator()() const noexcept -> void { allocator->roll_back(prev); }
    };

    struct pending_allocation_t
    {
        allocation_t allocation;
        rollback_command_t rollback_command;
    };

    auto max_allocation_size() const noexcept { return 256; }

    auto empty() const noexcept -> bool { return begin() == cur_; }

    auto try_allocate(std::size_t size, std::align_val_t align_val) noexcept -> pending_allocation_t
    {
        auto const alignment = static_cast<std::size_t>(align_val);
        size = std::max(size, std::size_t{1});

        auto const aligned_begin = (cur_ + alignment - 1) & -alignment;
        auto const aligned_end = aligned_begin + size;

        auto const arena_end = begin() + arena_.size;
        auto const fits = aligned_end <= arena_end;
        if (!fits) return nullptr;

        auto const prev = cur_;
        cur_ = aligned_end;

        return pending_allocation_t{reinterpret_cast<void*>(aligned_begin), rollback_command_t{this, prev}};
    }

    auto roll_back(rollback_token_t rollback_token) noexcept -> void { cur_ = rollback_token; }

    arena_allocator_t(arena_t arena) : arena_{std::move(arena)} {}

    arena_allocator_t(arena_t&&) = default;
    arena_allocator_t& operator=(arena_t&&) = default;
    arena_allocator_t(arena_t const&) = delete;
    arena_allocator_t& operator=(arena_t const&) = delete;

private:
    auto begin() const noexcept -> address_t { return reinterpret_cast<address_t>(std::get(arena_)); }

    arena_t arena_;
    address_t cur_{begin()};
};

template <typename allocator_t, typename allocations_t = std::vector<typename allocator_t::allocation_t>>
class scoped_allocator_t
{
public:
    using allocation_t = void*;

    auto allocate(std::size_t size, std::align_val_t align_val) -> allocation_t
    {
        return std::to_address(allocations_.emplace_back(allocator_.allocate(size, align_val)));
    }

    auto roll_back() noexcept -> void
    {
        if (!allocations_.empty()) allocations_.pop_back();
    }

    explicit scoped_allocator_t(allocator_t allocator, allocations_t allocations = {}) noexcept
        : allocator_{std::move(allocator)}, allocations_{std::move(allocations)}
    {}

    scoped_allocator_t(scoped_allocator_t const&) = delete;
    auto operator=(scoped_allocator_t const&) -> scoped_allocator_t& = delete;

    scoped_allocator_t(scoped_allocator_t&&) = default;
    auto operator=(scoped_allocator_t&&) -> scoped_allocator_t& = default;

private:
    allocator_t allocator_{};
    allocations_t allocations_{};
};

} // namespace
} // namespace dink
