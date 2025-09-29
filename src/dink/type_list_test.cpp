/*!
    \file
    Copyright (C) 2025 Frank Secilia
*/

#include "type_list.hpp"
#include <dink/test.hpp>

namespace dink::type_list {
namespace {

// arbitrary, unique types
struct v0;
struct v1;
struct v2;
struct v3;

// shorthand for type list
template <typename... elements_t>
using t = type_list_t<elements_t...>;

// ---------------------------------------------------------------------------------------------------------------------

// append to empty list
static_assert(std::is_same_v<append_t<t<>, v0>, t<v0>>);

// append to list with one element
static_assert(std::is_same_v<append_t<t<v0>, v1>, t<v0, v1>>);

// append to list with multiple elements
static_assert(std::is_same_v<append_t<t<v0, v1, v2>, v3>, t<v0, v1, v2, v3>>);

// ---------------------------------------------------------------------------------------------------------------------

// empty list
static_assert(!contains_v<t<>, v0>); // always contains nothing

// single element
static_assert(contains_v<t<v0>, v0>); // contained
static_assert(!contains_v<t<v0>, v1>); // not contained

// multiple elements
static_assert(contains_v<t<v0, v1, v2>, v0>); // begin contained
static_assert(contains_v<t<v0, v1, v2>, v2>); // end contained
static_assert(!contains_v<t<v0, v1, v2>, v3>); // not contained

// ---------------------------------------------------------------------------------------------------------------------

// append unique to empty list
static_assert(std::is_same_v<append_unique_t<t<>, v0>, t<v0>>); // append always works

// append unique to list with one element
static_assert(std::is_same_v<append_unique_t<t<v0>, v0>, t<v0>>); // already contained
static_assert(std::is_same_v<append_unique_t<t<v0>, v1>, t<v0, v1>>); // not already contained

// append unique to list with multiple elements
static_assert(std::is_same_v<append_unique_t<t<v0, v1, v2>, v0>, t<v0, v1, v2>>); // already contained
static_assert(std::is_same_v<append_unique_t<t<v0, v1, v2>, v3>, t<v0, v1, v2, v3>>); // not already contained

// ---------------------------------------------------------------------------------------------------------------------

// empty list is always unique
static_assert(std::is_same_v<unique_t<t<>>, t<>>);

// single element list is always unique
static_assert(std::is_same_v<unique_t<t<v0>>, t<v0>>);

// smallest multiple-element list
static_assert(std::is_same_v<unique_t<t<v0, v0>>, t<v0>>); // not unique
static_assert(std::is_same_v<unique_t<t<v0, v1>>, t<v0, v1>>); // already unique

// larger multiple-element list
static_assert(std::is_same_v<unique_t<t<v0, v0, v0>>, t<v0>>); // not unique multiple times
static_assert(std::is_same_v<unique_t<t<v0, v0, v1>>, t<v0, v1>>); // not unique once
static_assert(std::is_same_v<unique_t<t<v0, v1, v2>>, t<v0, v1, v2>>); // unique
static_assert(std::is_same_v<unique_t<t<v0, v1, v0>>, t<v0, v1>>); // non-consecutive duplicates
static_assert(std::is_same_v<unique_t<t<v0, v1, v0, v2, v1>>, t<v0, v1, v2>>); // more complex mix of duplicates

} // namespace
} // namespace dink::type_list
