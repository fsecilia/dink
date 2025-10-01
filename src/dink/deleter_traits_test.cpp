/*!
    \file
    Copyright (C) 2025 Frank Secilia
*/

#include "deleter_traits.hpp"
#include <dink/test.hpp>

namespace dink {
namespace {

// type being pointed to
struct old_element_t
{};

// type we want to rebind the deleter to
struct new_element_t
{};

// simple, unparameterized, custom deleter to test fallback case
struct simple_deleter_t
{};

// arbitrary argument for parameterized deleter
struct deleter_arg_t
{};

// custom deleter that follows rebindable convention, element type is first template param
template <typename value_t, typename arg_t>
struct parameterized_deleter_t
{};

// ---------------------------------------------------------------------------------------------------------------------

// fallback case for simple, unparameterized, custom deleter; deleter should be unchanged
static_assert(std::is_same_v<rebind_deleter_t<simple_deleter_t, new_element_t>, simple_deleter_t>);

// explicit test for std::default_delete; deleter should be rebound to new element type
static_assert(std::is_same_v<
              rebind_deleter_t<std::default_delete<old_element_t>, new_element_t>, std::default_delete<new_element_t>>);

// template template specialization; should correctly substitute new element type while preserving other arguments
static_assert(
    std::is_same_v<
        rebind_deleter_t<parameterized_deleter_t<old_element_t, deleter_arg_t>, new_element_t>,
        parameterized_deleter_t<new_element_t, deleter_arg_t>>
);

} // namespace
} // namespace dink
