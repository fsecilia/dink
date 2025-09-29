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

} // namespace
} // namespace dink::type_list
