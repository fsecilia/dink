/*!
    \file
    Copyright (C) 2025 Frank Secilia
*/

#include <dink/test.hpp>
#include <dink/object_repository/memory/alignment.hpp>
#include <atomic>
#include <bit>
#include <cassert>
#include <cstddef>
#include <optional>
#include <unordered_map>
#include <utility>

namespace dink {
// namespace {

//! ensures container capacity is available for at least one call to push or emplace; maintains amortized O(1) growth
template <typename container_t>
auto ensure_capacity_for_push(container_t& container) -> void
{
    auto capacity = container.capacity();
    if (container.size() < capacity) return;
    container.reserve(capacity * 2);
}

//! ensures container size is large enough so that at least given index is valid; maintains amortized O(1) growth
template <typename container_t>
auto ensure_size_for_index(container_t& container, std::size_t index) -> void
{
    size_t const required_size = index + 1;

    if (required_size > container.capacity())
    {
        size_t new_capacity = std::bit_ceil(required_size);
        if (new_capacity == required_size) new_capacity *= 2;
        container.reserve(new_capacity);
    }

    if (required_size > container.size()) container.resize(required_size, nullptr);
}

struct os_page_size_provider_t
{
    auto operator()() const noexcept -> std::size_t { return 4096; }
};

template <typename allocation_t>
class region_t
{
public:
    auto begin() const noexcept -> void* { return std::to_address(allocation_); }
    auto end() const noexcept -> void* { return end_; }
    auto size() const noexcept -> std::size_t { return end_ - begin(); }

    region_t(allocation_t&& allocation, std::size_t size) noexcept
        : end_{reinterpret_cast<std::byte*>(std::to_address(allocation)) + size}, allocation_{std::move(allocation)}
    {}

private:
    std::byte* end_;
    allocation_t allocation_;
};

struct heap_allocation_deleter_t
{
    auto operator()(void* allocation) const noexcept -> void { std::free(allocation); }
};

struct heap_allocator_api_t
{
    [[nodiscard]] auto malloc(std::size_t size) const noexcept -> void* { return std::malloc(size); }
    [[nodiscard]] auto aligned_alloc(std::size_t alignment, std::size_t size) const noexcept -> void*
    {
        return std::aligned_alloc(alignment, size);
    }
};

template <typename deleter_t, typename api_t>
class heap_allocator_t
{
public:
    using allocation_t = std::unique_ptr<void, deleter_t>;

    [[nodiscard]] auto allocate(std::size_t size, deleter_t deleter = {}) const -> allocation_t
    {
        auto result = allocation_t{api_.malloc(size), std::move(deleter)};
        if (!result) throw std::bad_alloc{};
        return result;
    }

    [[nodiscard]] auto allocate(std::size_t size, std::align_val_t align_val, deleter_t deleter = {}) const
        -> allocation_t
    {
        assert(is_valid_alignment(align_val));

        // aligned_alloc requires allocation size be a multiple of the alignment, so round up
        size = align(size, align_val);

        auto result = allocation_t{api_.aligned_alloc(static_cast<std::size_t>(align_val), size), std::move(deleter)};
        if (!result) throw std::bad_alloc{};
        return result;
    }

    explicit heap_allocator_t(api_t api = {}) noexcept : api_{std::move(api)} {}

private:
    [[no_unique_address]] api_t api_{};
};

template <typename allocator_t>
class page_transaction_t
{
public:
    explicit operator bool() const noexcept { return allocation(); }

    auto allocation() const noexcept -> void* { return allocation_begin_; }
    auto commit() noexcept -> void { allocator_->commit(allocation_end_); }

    page_transaction_t(allocator_t& allocator, void* allocation_begin, void* allocation_end) noexcept
        : allocator_{&allocator}, allocation_begin_{std::move(allocation_begin)}, allocation_end_{allocation_end}
    {}

private:
    allocator_t* allocator_;
    void* allocation_begin_;
    void* allocation_end_;
};

template <template <typename> class page_transaction_p, typename region_t>
class page_t
{
public:
    using transaction_t = page_transaction_p<page_t>;

    [[nodiscard]] auto prepare(std::size_t size, std::align_val_t align_val) noexcept -> transaction_t
    {
        assert(is_valid_alignment(align_val));

        size = std::max(size, std::size_t{1});

        auto const allocation_begin = align(cur_, align_val);

        auto const size_remaining
            = static_cast<std::size_t>(reinterpret_cast<std::byte*>(region_.end()) - allocation_begin);
        auto const end_exceeds_buffer = size_remaining < size;
        if (end_exceeds_buffer) return transaction_t{*this, nullptr, nullptr};

        auto const allocation_end = allocation_begin + size;
        return transaction_t{*this, allocation_begin, allocation_end};
    }

    auto commit(void* allocation_end) noexcept -> void
    {
        assert(cur_ < allocation_end);
        assert(allocation_end <= region_.end());

        cur_ = static_cast<std::byte*>(allocation_end);
    }

    explicit page_t(region_t&& region) noexcept
        : cur_{static_cast<std::byte*>(region.begin())}, region_{std::move(region)}
    {}

private:
    std::byte* cur_;
    region_t region_;
};

template <typename page_t, typename allocator_t>
class page_factory_t
{
public:
    [[nodiscard]] auto operator()(std::size_t size, std::align_val_t align_val) -> page_t
    {
        return page_t{region_t{allocator_.allocate(size, align_val), size}};
    }

    explicit page_factory_t(allocator_t allocator) noexcept : allocator_{std::move(allocator)} {}

private:
    allocator_t allocator_;
};

template <typename os_page_size_provider_t>
struct page_size_config_t
{
    static inline auto constexpr const os_pages_per_logical_page = std::size_t{16};
    using max_allocation_size_scale = std::ratio<1, 8>;

    std::size_t os_page_size;
    std::size_t page_size = os_page_size * os_pages_per_logical_page;
    std::size_t max_allocation_size = (page_size / max_allocation_size_scale::den) * max_allocation_size_scale::num;

    explicit page_size_config_t(os_page_size_provider_t os_page_size_provider) noexcept
        : os_page_size{os_page_size_provider()}
    {}
};

template <typename paged_allocator_t, typename page_t, typename page_transaction_t>
class paged_allocator_transaction_t
{
public:
    explicit operator bool() const noexcept { return allocation(); }

    auto allocation() const noexcept -> void* { return page_transaction_.allocation(); }
    auto commit() noexcept -> void
    {
        paged_allocator_->commit(std::move(new_page_));
        page_transaction_.commit();
    }

    paged_allocator_transaction_t(
        paged_allocator_t& paged_allocator, page_transaction_t&& page_transaction, std::optional<page_t> new_page
    ) noexcept
        : paged_allocator_{&paged_allocator}, page_transaction_{std::move(page_transaction)},
          new_page_{std::move(new_page)}
    {}

private:
    paged_allocator_t* paged_allocator_;
    page_transaction_t page_transaction_;
    std::optional<page_t> new_page_;
};

template <
    template <typename, typename, typename> class transaction_p, typename page_t, typename page_factory_t,
    typename page_transaction_t, typename page_size_config_t>
class paged_allocator_t
{
public:
    using transaction_t = transaction_p<paged_allocator_t, page_t, page_transaction_t>;

    auto max_allocation_size() const noexcept -> std::size_t { return page_size_config_.max_allocation_size; }

    [[nodiscard]] auto prepare(std::size_t size, std::align_val_t align_val) -> transaction_t
    {
        assert(is_valid_alignment(align_val));
        assert(size + static_cast<std::size_t>(align_val) - 1 <= max_allocation_size());

        auto page_transaction = pages_.back().prepare(size, align_val);
        if (page_transaction.allocation()) { return transaction_t{*this, std::move(page_transaction), std::nullopt}; }

        ensure_capacity_for_push(pages_);

        auto new_page = create_page();
        page_transaction = new_page.prepare(size, align_val);
        assert(page_transaction.allocation());
        return transaction_t{*this, std::move(page_transaction), std::optional{std::move(new_page)}};
    }

    auto commit(std::optional<page_t>&& new_page) noexcept -> void
    {
        assert(pages_.size() < pages_.capacity());
        if (new_page) pages_.emplace_back(*std::move(new_page));
    }

    explicit paged_allocator_t(page_factory_t create_page, page_size_config_t page_size_config)
        : create_page_{std::move(create_page)}, page_size_config_{std::move(page_size_config)}
    {
        pages_.emplace_back(this->create_page());
    }

private:
    page_factory_t create_page_;
    page_size_config_t page_size_config_;
    std::vector<page_t> pages_;

    auto create_page() -> page_t
    {
        return create_page_(page_size_config_.page_size, std::align_val_t{page_size_config_.os_page_size});
    }
};

template <typename scoped_allocator_t, typename allocation_t>
class scoped_allocator_transaction_t
{
public:
    explicit operator bool() const noexcept { return allocation(); }

    auto allocation() const noexcept -> void* { return std::to_address(allocation_); }
    auto commit() noexcept -> void { scoped_allocator_->commit(std::move(allocation_)); }

    scoped_allocator_transaction_t(scoped_allocator_t& scoped_allocator, allocation_t&& allocation) noexcept
        : scoped_allocator_{&scoped_allocator}, allocation_{std::move(allocation)}
    {}

private:
    scoped_allocator_t* scoped_allocator_;
    allocation_t allocation_;
};

template <template <typename, typename> class transaction_p, typename allocator_t, typename allocation_t>
class scoped_allocator_t
{
public:
    using transaction_t = transaction_p<scoped_allocator_t, allocation_t>;

    auto prepare(std::size_t size, std::align_val_t align_val) -> transaction_t
    {
        ensure_capacity_for_push(allocations_);
        return transaction_t{*this, allocator_.allocate(size, align_val)};
    }

    auto commit(allocation_t&& allocation) noexcept -> void
    {
        assert(allocations_.size() < allocations_.capacity());
        allocations_.emplace_back(std::move(allocation));
    }

    explicit scoped_allocator_t(allocator_t allocator) : allocator_{std::move(allocator)} {}

private:
    allocator_t allocator_;
    std::vector<allocation_t> allocations_;
};

template <typename small_object_transaction_t, typename large_object_transaction_t>
class thresholding_allocator_transaction_t
{
public:
    explicit operator bool() const noexcept { return allocation(); }

    auto allocation() const noexcept -> void*
    {
        return std::visit([](auto& strat) noexcept { return strat.allocation(); }, strat_);
    }

    auto commit() noexcept -> void
    {
        std::visit([](auto& strat) noexcept { strat.commit(); }, strat_);
    }

    thresholding_allocator_transaction_t(small_object_transaction_t&& small_object_transaction) noexcept
        : strat_{std::move(small_object_transaction)}
    {}

    thresholding_allocator_transaction_t(large_object_transaction_t&& large_object_transaction) noexcept
        : strat_{std::move(large_object_transaction)}
    {}

private:
    std::variant<small_object_transaction_t, large_object_transaction_t> strat_;
};

template <typename transaction_t, typename small_object_allocator_t, typename large_object_allocator_t>
class thresholding_allocator_t
{
public:
    auto threshold() const noexcept -> std::size_t { return small_object_allocator_.max_allocation_size(); }

    auto prepare(std::size_t size, std::align_val_t align_val) -> transaction_t
    {
        auto const total_allocation_size = size + static_cast<std::size_t>(align_val) - 1;
        if (total_allocation_size <= small_object_allocator_.max_allocation_size())
        {
            return transaction_t{small_object_allocator_.prepare(size, align_val)};
        }

        return transaction_t{large_object_allocator_.prepare(size, align_val)};
    }

    thresholding_allocator_t(
        small_object_allocator_t small_object_allocator, large_object_allocator_t large_object_allocator
    ) noexcept
        : small_object_allocator_{std::move(small_object_allocator)},
          large_object_allocator_{std::move(large_object_allocator)}
    {}

private:
    small_object_allocator_t small_object_allocator_;
    large_object_allocator_t large_object_allocator_;
};

class destruction_list_element_t
{
public:
    template <typename instance_t>
    explicit destruction_list_element_t(instance_t* instance) noexcept
        : instance_{instance, &instance_dtor<instance_t>()}
    {}

private:
    using deleter_t = void (*)(void*) noexcept;
    std::unique_ptr<void, deleter_t> instance_;

    template <typename instance_t>
    static auto instance_dtor(void* instance) noexcept -> void
    {
        assert(instance);
        std::destroy_at(reinterpret_cast<instance_t*>(instance));
    }
};

template <typename element_t = destruction_list_element_t, typename elements_t = std::vector<element_t>>
class destruction_list_t
{
public:
    auto prepare() -> void { ensure_capacity_for_push(elements_); }

    template <typename instance_t>
    auto commit(instance_t* instance)
    {
        elements_.emplace_back(element_t{instance});
    }

private:
    elements_t elements_;
};

/*!
    generates non-deterministic, global runtime type indices stable for the lifetime of a single program execution

    This type generates type ids that are globally stable per type per run. It is not a formal singleton, but it is
    idempotent and two instances will produce the same values for the same types.
*/
class type_index_t
{
public:
    using index_t = std::size_t;

    template <typename type_t>
    auto get() const noexcept -> index_t
    {
        static auto const index = generate();
        return index;
    }

private:
    static auto generate() noexcept -> index_t
    {
        static std::atomic<index_t> next{1};
        return next.fetch_add(1, std::memory_order_relaxed);
    }
};

template <typename id_map_t, typename map_iterator_t>
class id_map_transaction_t
{
public:
    explicit operator bool() const noexcept { return true; }

    template <typename instance_t>
    auto commit(instance_t* instance) const noexcept -> void
    {
        id_map_->commit(map_iterator_, instance);
    }

private:
    id_map_t* id_map_;
    map_iterator_t map_iterator_;
};

template <typename type_index_t>
class id_map_t
{
public:
    using map_t = std::unordered_map<std::size_t, void*>;
    using map_iterator_t = map_t::iterator;

    template <typename instance_t>
    auto find() -> instance_t*
    {
        auto const type_index = type_index_.template get<instance_t>();
        if (type_index >= instances_.size()) return nullptr;
        return static_cast<instance_t*>(instances_[type_index]);
    }

    template <typename instance_t>
    auto prepare() -> void
    {
        auto result = instances_.emplace(type_index_.template get<instance_t>(), nullptr);
    }

    template <typename instance_t>
    auto commit(map_iterator_t map_iterator, instance_t* instance) noexcept -> void
    {
        map_iterator->second = instance;
    }

private:
    map_t instances_;
    [[no_unique_address]] type_index_t type_index_;
};

TEST(object_repo_poc, run)
{
    using allocation_t = std::unique_ptr<void, heap_allocation_deleter_t>;
    using heap_allocator_t = heap_allocator_t<heap_allocation_deleter_t, heap_allocator_api_t>;
    using region_t = region_t<allocation_t>;

    using page_t = page_t<page_transaction_t, region_t>;
    using page_transaction_t = page_transaction_t<page_t>;
    using page_factory_t = page_factory_t<page_t, heap_allocator_t>;
    using page_size_config_t = page_size_config_t<os_page_size_provider_t>;

    using small_object_allocator_t = paged_allocator_t<
        paged_allocator_transaction_t, page_t, page_factory_t, page_transaction_t, page_size_config_t>;
    using small_object_allocator_transaction_t
        = paged_allocator_transaction_t<small_object_allocator_t, page_t, page_transaction_t>;

    using large_object_allocator_t = scoped_allocator_t<scoped_allocator_transaction_t, heap_allocator_t, allocation_t>;
    using large_object_allocator_transaction_t = scoped_allocator_transaction_t<large_object_allocator_t, allocation_t>;

    using thresholding_allocator_transaction_t = thresholding_allocator_transaction_t<
        small_object_allocator_transaction_t, large_object_allocator_transaction_t>;
    using thresholding_allocator_t = thresholding_allocator_t<
        thresholding_allocator_transaction_t, small_object_allocator_t, large_object_allocator_t>;

    auto thresholding_allocator = thresholding_allocator_t{
        small_object_allocator_t{page_factory_t{heap_allocator_t{}}, page_size_config_t{os_page_size_provider_t{}}},
        large_object_allocator_t{heap_allocator_t{}}
    };

    std::cout << thresholding_allocator.prepare(5, std::align_val_t{16}).allocation() << std::endl;
    std::cout << thresholding_allocator.prepare(50001, std::align_val_t{16}).allocation() << std::endl;
}

// } // namespace
} // namespace dink
