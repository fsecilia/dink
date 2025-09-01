/*!
    \file
    Copyright (C) 2025 Frank Secilia
*/

#pragma once

#include <dink/lib.hpp>
#include <dink/memory/page.hpp>
#include <dink/memory/page_factory.hpp>
#include <bit>
#include <cstddef>
#include <new>
#include <vector>

namespace dink {

template <typename params_t, typename page_t, typename page_factory_t>
concept paged_sub_allocator_ctor_params = requires(params_t params) {
    { params.page_factory } -> std::convertible_to<page_factory_t>;
    { params.initial_page } -> std::convertible_to<page_t>;
};

/*!
    Parameter type used to construct paged_sub_allocator_t.
    
    This parameter type contains both a page factory, and the initial page from that factory.
*/
template <page page_t, page_factory<page_t> page_factory_t>
struct paged_sub_allocator_ctor_params_t
{
    page_factory_t page_factory;
    page_t initial_page = page_factory();
};

/*!
    manages a collection of pages for small object allocations
    
    The paged sub-allocator is a subcomponent of a larger allocator. It satisfies allocation requests by returning
    memory views into a set of managed pages. The pages themselves own the allocations, making this type append-only.
    
    \note This is not a general-purpose allocator. It operates under a narrow contract, only asserting its
    preconditions, expecting its owner to enforce them.
    
    \invariant An instance always contains at least one page. This is a contract enforced by the constructor, which
    allows the allocation path to be simplified by safely accessing the tail page without an empty check.
    
    \tparam page_t The type of page object to manage.
    \tparam page_factory_t The type of factory used to create new pages.
*/
template <
    page page_t, page_factory<page_t> page_factory_t,
    paged_sub_allocator_ctor_params<page_t, page_factory_t> ctor_params_t>
class paged_sub_allocator_t
{
public:
    //! maximum effective allocation size supported (`size + alignment - 1`)
    auto max_allocation_size() const noexcept -> std::size_t
    {
        auto const page_size = page_factory_.size();
        return page_size / 8;
    }

    /*!
        verifies if a given size and alignment satisfy preconditions for allocation
        
        This function encapsulates the logic for the two primary rules that an allocation request must follow:        
            1. The alignment must be a non-zero power of two.            
            2. The worst-case effective size (`size + alignment - 1`) must not exceed the maximum supported by this
            allocator.
        
        \note This method is public primarily for testing purposes, allowing the precondition logic to be verified
        independently of whether assertions are enabled. It is used internally by the `allocate()` method's assert.
        
        \param size The number of bytes for the proposed allocation.
        \param alignment The required alignment for the proposed allocation.        
        \returns True if the preconditions are met and a call to `allocate()` is safe, false otherwise.
    */
    auto allocate_preconditions_met(std::size_t size, std::align_val_t alignment) const noexcept -> bool
    {
        auto const alignment_value = static_cast<std::size_t>(alignment);
        auto const alignment_is_nonzero_power_of_two = std::has_single_bit(alignment_value);

        auto const worst_case_alignment = alignment_value - 1;
        auto const effective_allocation_size = size + worst_case_alignment;

        return alignment_is_nonzero_power_of_two && effective_allocation_size <= max_allocation_size();
    }

    /*!
        allocates a memory view from its managed pages
        
        Attempts to allocate from the most recently created page. If that page is full, a new page is created and the
        memory view is allocated from it.
        
        \pre Alignment must be a non-zero power of two.
        \pre The worst-case effective size of the allocation (`size + alignment - 1`) must be less than or equal to the
        value returned by `max_allocation_size()`.
        
        \param size The number of bytes to allocate.
        \param alignment The required alignment for the allocation.
        
        \throws std::bad_alloc if a new page is needed and the page factory fails to allocate its memory, or if 
        appending to the vector of pages fails.
                
        \returns A non-null pointer to the allocated memory.
    */
    //! tries allocating from leaf page, allocates from new page if full
    auto allocate(std::size_t size, std::align_val_t alignment) -> void*
    {
        assert(allocate_preconditions_met(size, alignment));

        // try allocating from most recent page
        auto result = pages_.back().try_allocate(size, alignment);
        if (result) return result;

        // allocate from new page
        auto new_page = pages_.emplace_back(page_factory_());
        result = new_page.try_allocate(size, alignment);

        // allocating from a new page always succeeds when preconditions are met
        assert(result);

        return result;
    }

    //! constructs with factory and initial page
    paged_sub_allocator_t(ctor_params_t ctor_params) noexcept
        : page_factory_{std::move(ctor_params).page_factory}, pages_{std::move(ctor_params).initial_page}
    {
    }

private:
    page_factory_t page_factory_{};
    std::vector<page_t> pages_{};
};

} // namespace dink
