/*!
    \file
    Copyright (C) 2025 Frank Secilia
*/

#include "unqualified.hpp"
#include <dink/test.hpp>

namespace dink {
namespace {

// arbitrary unique type
struct type_t
{};

// unparameterized deleter
struct simple_deleter_t
{};

// param for parameterized deleter
struct deleter_param_t
{};

template <typename value_t, typename param_t>
struct parameterized_deleter_t
{};

// base case: these should all resolve to the canonical type
static_assert(std::is_same_v<unqualified_t<type_t>, type_t>);
static_assert(std::is_same_v<unqualified_t<type_t&>, type_t>);
static_assert(std::is_same_v<unqualified_t<type_t const*>, type_t>);
static_assert(std::is_same_v<unqualified_t<std::reference_wrapper<type_t>>, type_t>);

// unique_ptrs resolve to a unique_ptr to the canonical type
static_assert(std::is_same_v<unqualified_t<std::unique_ptr<type_t>>, std::unique_ptr<type_t>>);
static_assert(std::is_same_v<unqualified_t<std::unique_ptr<type_t> const&>, std::unique_ptr<type_t>>);
static_assert(std::is_same_v<unqualified_t<std::unique_ptr<type_t* const>>, std::unique_ptr<type_t>>);

// unique_pts with deleters must also resolve their deleters to match
static_assert(
    std::is_same_v<
        unqualified_t<std::unique_ptr<type_t* const, simple_deleter_t>>, std::unique_ptr<type_t, simple_deleter_t>>
);
static_assert(
    std::is_same_v<
        unqualified_t<std::unique_ptr<type_t* const, parameterized_deleter_t<type_t* const, deleter_param_t>>>,
        std::unique_ptr<type_t, parameterized_deleter_t<type_t, deleter_param_t>>>
);

// shared_pointers remain shared_ptrs
static_assert(std::is_same_v<unqualified_t<std::shared_ptr<type_t>>, std::shared_ptr<type_t>>);
static_assert(std::is_same_v<unqualified_t<std::shared_ptr<type_t> const&>, std::shared_ptr<type_t>>);
static_assert(std::is_same_v<unqualified_t<std::shared_ptr<type_t* const>>, std::shared_ptr<type_t>>);

// weak_ptrs become shared_ptrs
static_assert(std::is_same_v<unqualified_t<std::weak_ptr<type_t>>, std::shared_ptr<type_t>>);
static_assert(std::is_same_v<unqualified_t<std::weak_ptr<type_t> const&>, std::shared_ptr<type_t>>);
static_assert(std::is_same_v<unqualified_t<std::weak_ptr<type_t* const>>, std::shared_ptr<type_t>>);

} // namespace
} // namespace dink
