/*!
    \file
    Copyright (C) 2025 Frank Secilia
*/

#include <dink/test.hpp>
#include <dink/tuple.hpp>

namespace dink::tuple {
namespace {

// shorthand for tests
template <typename... elements_t>
using t = std::tuple<elements_t...>;

template <int id>
struct unique_value_t
{};

using v0 = unique_value_t<0>;
using v1 = unique_value_t<1>;
using v2 = unique_value_t<2>;

// clang-format off

// exhaustive test of all combinations of 0, 1, 2 of lengths up to 3

static_assert(!contains_v<t<>, v0>);
static_assert(!contains_v<t<>, v1>);
static_assert(!contains_v<t<>, v2>);
static_assert(contains_v<t<v0>, v0>);
static_assert(!contains_v<t<v0>, v1>);
static_assert(!contains_v<t<v0>, v2>);
static_assert(!contains_v<t<v1>, v0>);
static_assert(contains_v<t<v1>, v1>);
static_assert(!contains_v<t<v1>, v2>);
static_assert(!contains_v<t<v2>, v0>);
static_assert(!contains_v<t<v2>, v1>);
static_assert(contains_v<t<v2>, v2>);
static_assert(contains_v<t<v0, v1>, v0>);
static_assert(contains_v<t<v0, v1>, v1>);
static_assert(!contains_v<t<v0, v1>, v2>);
static_assert(contains_v<t<v0, v2>, v0>);
static_assert(!contains_v<t<v0, v2>, v1>);
static_assert(contains_v<t<v0, v2>, v2>);
static_assert(contains_v<t<v1, v0>, v0>);
static_assert(contains_v<t<v1, v0>, v1>);
static_assert(!contains_v<t<v1, v0>, v2>);
static_assert(!contains_v<t<v1, v2>, v0>);
static_assert(contains_v<t<v1, v2>, v1>);
static_assert(contains_v<t<v1, v2>, v2>);
static_assert(contains_v<t<v2, v0>, v0>);
static_assert(!contains_v<t<v2, v0>, v1>);
static_assert(contains_v<t<v2, v0>, v2>);
static_assert(!contains_v<t<v2, v1>, v0>);
static_assert(contains_v<t<v2, v1>, v1>);
static_assert(contains_v<t<v2, v1>, v2>);
static_assert(contains_v<t<v0, v1, v2>, v0>);
static_assert(contains_v<t<v0, v1, v2>, v1>);
static_assert(contains_v<t<v0, v1, v2>, v2>);
static_assert(contains_v<t<v0, v2, v1>, v0>);
static_assert(contains_v<t<v0, v2, v1>, v1>);
static_assert(contains_v<t<v0, v2, v1>, v2>);
static_assert(contains_v<t<v1, v0, v2>, v0>);
static_assert(contains_v<t<v1, v0, v2>, v1>);
static_assert(contains_v<t<v1, v0, v2>, v2>);
static_assert(contains_v<t<v1, v2, v0>, v0>);
static_assert(contains_v<t<v1, v2, v0>, v1>);
static_assert(contains_v<t<v1, v2, v0>, v2>);
static_assert(contains_v<t<v2, v0, v1>, v0>);
static_assert(contains_v<t<v2, v0, v1>, v1>);
static_assert(contains_v<t<v2, v0, v1>, v2>);
static_assert(contains_v<t<v2, v1, v0>, v0>);
static_assert(contains_v<t<v2, v1, v0>, v1>);
static_assert(contains_v<t<v2, v1, v0>, v2>);

// ---------------------------------------------------------------------------------------------------------------------

static_assert(std::is_same_v<append_t<t<>, v0>, t<v0>>);
static_assert(std::is_same_v<append_t<t<>, v1>, t<v1>>);
static_assert(std::is_same_v<append_t<t<>, v2>, t<v2>>);
static_assert(std::is_same_v<append_t<t<v0>, v0>, t<v0, v0>>);
static_assert(std::is_same_v<append_t<t<v0>, v1>, t<v0, v1>>);
static_assert(std::is_same_v<append_t<t<v0>, v2>, t<v0, v2>>);
static_assert(std::is_same_v<append_t<t<v1>, v0>, t<v1, v0>>);
static_assert(std::is_same_v<append_t<t<v1>, v1>, t<v1, v1>>);
static_assert(std::is_same_v<append_t<t<v1>, v2>, t<v1, v2>>);
static_assert(std::is_same_v<append_t<t<v2>, v0>, t<v2, v0>>);
static_assert(std::is_same_v<append_t<t<v2>, v1>, t<v2, v1>>);
static_assert(std::is_same_v<append_t<t<v2>, v2>, t<v2, v2>>);
static_assert(std::is_same_v<append_t<t<v0, v0>, v0>, t<v0, v0, v0>>);
static_assert(std::is_same_v<append_t<t<v0, v0>, v1>, t<v0, v0, v1>>);
static_assert(std::is_same_v<append_t<t<v0, v0>, v2>, t<v0, v0, v2>>);
static_assert(std::is_same_v<append_t<t<v0, v1>, v0>, t<v0, v1, v0>>);
static_assert(std::is_same_v<append_t<t<v0, v1>, v1>, t<v0, v1, v1>>);
static_assert(std::is_same_v<append_t<t<v0, v1>, v2>, t<v0, v1, v2>>);
static_assert(std::is_same_v<append_t<t<v0, v2>, v0>, t<v0, v2, v0>>);
static_assert(std::is_same_v<append_t<t<v0, v2>, v1>, t<v0, v2, v1>>);
static_assert(std::is_same_v<append_t<t<v0, v2>, v2>, t<v0, v2, v2>>);
static_assert(std::is_same_v<append_t<t<v1, v0>, v0>, t<v1, v0, v0>>);
static_assert(std::is_same_v<append_t<t<v1, v0>, v1>, t<v1, v0, v1>>);
static_assert(std::is_same_v<append_t<t<v1, v0>, v2>, t<v1, v0, v2>>);
static_assert(std::is_same_v<append_t<t<v1, v1>, v0>, t<v1, v1, v0>>);
static_assert(std::is_same_v<append_t<t<v1, v1>, v1>, t<v1, v1, v1>>);
static_assert(std::is_same_v<append_t<t<v1, v1>, v2>, t<v1, v1, v2>>);
static_assert(std::is_same_v<append_t<t<v1, v2>, v0>, t<v1, v2, v0>>);
static_assert(std::is_same_v<append_t<t<v1, v2>, v1>, t<v1, v2, v1>>);
static_assert(std::is_same_v<append_t<t<v1, v2>, v2>, t<v1, v2, v2>>);
static_assert(std::is_same_v<append_t<t<v2, v0>, v0>, t<v2, v0, v0>>);
static_assert(std::is_same_v<append_t<t<v2, v0>, v1>, t<v2, v0, v1>>);
static_assert(std::is_same_v<append_t<t<v2, v0>, v2>, t<v2, v0, v2>>);
static_assert(std::is_same_v<append_t<t<v2, v1>, v0>, t<v2, v1, v0>>);
static_assert(std::is_same_v<append_t<t<v2, v1>, v1>, t<v2, v1, v1>>);
static_assert(std::is_same_v<append_t<t<v2, v1>, v2>, t<v2, v1, v2>>);
static_assert(std::is_same_v<append_t<t<v2, v2>, v0>, t<v2, v2, v0>>);
static_assert(std::is_same_v<append_t<t<v2, v2>, v1>, t<v2, v2, v1>>);
static_assert(std::is_same_v<append_t<t<v2, v2>, v2>, t<v2, v2, v2>>);
static_assert(std::is_same_v<append_t<t<v0, v0, v0>, v0>, t<v0, v0, v0, v0>>);
static_assert(std::is_same_v<append_t<t<v0, v0, v1>, v0>, t<v0, v0, v1, v0>>);
static_assert(std::is_same_v<append_t<t<v0, v0, v2>, v0>, t<v0, v0, v2, v0>>);
static_assert(std::is_same_v<append_t<t<v0, v1, v0>, v0>, t<v0, v1, v0, v0>>);
static_assert(std::is_same_v<append_t<t<v0, v1, v1>, v0>, t<v0, v1, v1, v0>>);
static_assert(std::is_same_v<append_t<t<v0, v1, v2>, v0>, t<v0, v1, v2, v0>>);
static_assert(std::is_same_v<append_t<t<v0, v2, v0>, v0>, t<v0, v2, v0, v0>>);
static_assert(std::is_same_v<append_t<t<v0, v2, v1>, v0>, t<v0, v2, v1, v0>>);
static_assert(std::is_same_v<append_t<t<v0, v2, v2>, v0>, t<v0, v2, v2, v0>>);
static_assert(std::is_same_v<append_t<t<v1, v0, v0>, v0>, t<v1, v0, v0, v0>>);
static_assert(std::is_same_v<append_t<t<v1, v0, v1>, v0>, t<v1, v0, v1, v0>>);
static_assert(std::is_same_v<append_t<t<v1, v0, v2>, v0>, t<v1, v0, v2, v0>>);
static_assert(std::is_same_v<append_t<t<v1, v1, v0>, v0>, t<v1, v1, v0, v0>>);
static_assert(std::is_same_v<append_t<t<v1, v1, v1>, v0>, t<v1, v1, v1, v0>>);
static_assert(std::is_same_v<append_t<t<v1, v1, v2>, v0>, t<v1, v1, v2, v0>>);
static_assert(std::is_same_v<append_t<t<v1, v2, v0>, v0>, t<v1, v2, v0, v0>>);
static_assert(std::is_same_v<append_t<t<v1, v2, v1>, v0>, t<v1, v2, v1, v0>>);
static_assert(std::is_same_v<append_t<t<v1, v2, v2>, v0>, t<v1, v2, v2, v0>>);
static_assert(std::is_same_v<append_t<t<v2, v0, v0>, v0>, t<v2, v0, v0, v0>>);
static_assert(std::is_same_v<append_t<t<v2, v0, v1>, v0>, t<v2, v0, v1, v0>>);
static_assert(std::is_same_v<append_t<t<v2, v0, v2>, v0>, t<v2, v0, v2, v0>>);
static_assert(std::is_same_v<append_t<t<v2, v1, v0>, v0>, t<v2, v1, v0, v0>>);
static_assert(std::is_same_v<append_t<t<v2, v1, v1>, v0>, t<v2, v1, v1, v0>>);
static_assert(std::is_same_v<append_t<t<v2, v1, v2>, v0>, t<v2, v1, v2, v0>>);
static_assert(std::is_same_v<append_t<t<v2, v2, v0>, v0>, t<v2, v2, v0, v0>>);
static_assert(std::is_same_v<append_t<t<v2, v2, v1>, v0>, t<v2, v2, v1, v0>>);
static_assert(std::is_same_v<append_t<t<v2, v2, v2>, v0>, t<v2, v2, v2, v0>>);
static_assert(std::is_same_v<append_t<t<v0, v0, v0>, v1>, t<v0, v0, v0, v1>>);
static_assert(std::is_same_v<append_t<t<v0, v0, v1>, v1>, t<v0, v0, v1, v1>>);
static_assert(std::is_same_v<append_t<t<v0, v0, v2>, v1>, t<v0, v0, v2, v1>>);
static_assert(std::is_same_v<append_t<t<v0, v1, v0>, v1>, t<v0, v1, v0, v1>>);
static_assert(std::is_same_v<append_t<t<v0, v1, v1>, v1>, t<v0, v1, v1, v1>>);
static_assert(std::is_same_v<append_t<t<v0, v1, v2>, v1>, t<v0, v1, v2, v1>>);
static_assert(std::is_same_v<append_t<t<v0, v2, v0>, v1>, t<v0, v2, v0, v1>>);
static_assert(std::is_same_v<append_t<t<v0, v2, v1>, v1>, t<v0, v2, v1, v1>>);
static_assert(std::is_same_v<append_t<t<v0, v2, v2>, v1>, t<v0, v2, v2, v1>>);
static_assert(std::is_same_v<append_t<t<v1, v0, v0>, v1>, t<v1, v0, v0, v1>>);
static_assert(std::is_same_v<append_t<t<v1, v0, v1>, v1>, t<v1, v0, v1, v1>>);
static_assert(std::is_same_v<append_t<t<v1, v0, v2>, v1>, t<v1, v0, v2, v1>>);
static_assert(std::is_same_v<append_t<t<v1, v1, v0>, v1>, t<v1, v1, v0, v1>>);
static_assert(std::is_same_v<append_t<t<v1, v1, v1>, v1>, t<v1, v1, v1, v1>>);
static_assert(std::is_same_v<append_t<t<v1, v1, v2>, v1>, t<v1, v1, v2, v1>>);
static_assert(std::is_same_v<append_t<t<v1, v2, v0>, v1>, t<v1, v2, v0, v1>>);
static_assert(std::is_same_v<append_t<t<v1, v2, v1>, v1>, t<v1, v2, v1, v1>>);
static_assert(std::is_same_v<append_t<t<v1, v2, v2>, v1>, t<v1, v2, v2, v1>>);
static_assert(std::is_same_v<append_t<t<v2, v0, v0>, v1>, t<v2, v0, v0, v1>>);
static_assert(std::is_same_v<append_t<t<v2, v0, v1>, v1>, t<v2, v0, v1, v1>>);
static_assert(std::is_same_v<append_t<t<v2, v0, v2>, v1>, t<v2, v0, v2, v1>>);
static_assert(std::is_same_v<append_t<t<v2, v1, v0>, v1>, t<v2, v1, v0, v1>>);
static_assert(std::is_same_v<append_t<t<v2, v1, v1>, v1>, t<v2, v1, v1, v1>>);
static_assert(std::is_same_v<append_t<t<v2, v1, v2>, v1>, t<v2, v1, v2, v1>>);
static_assert(std::is_same_v<append_t<t<v2, v2, v0>, v1>, t<v2, v2, v0, v1>>);
static_assert(std::is_same_v<append_t<t<v2, v2, v1>, v1>, t<v2, v2, v1, v1>>);
static_assert(std::is_same_v<append_t<t<v2, v2, v2>, v1>, t<v2, v2, v2, v1>>);
static_assert(std::is_same_v<append_t<t<v0, v0, v0>, v2>, t<v0, v0, v0, v2>>);
static_assert(std::is_same_v<append_t<t<v0, v0, v1>, v2>, t<v0, v0, v1, v2>>);
static_assert(std::is_same_v<append_t<t<v0, v0, v2>, v2>, t<v0, v0, v2, v2>>);
static_assert(std::is_same_v<append_t<t<v0, v1, v0>, v2>, t<v0, v1, v0, v2>>);
static_assert(std::is_same_v<append_t<t<v0, v1, v1>, v2>, t<v0, v1, v1, v2>>);
static_assert(std::is_same_v<append_t<t<v0, v1, v2>, v2>, t<v0, v1, v2, v2>>);
static_assert(std::is_same_v<append_t<t<v0, v2, v0>, v2>, t<v0, v2, v0, v2>>);
static_assert(std::is_same_v<append_t<t<v0, v2, v1>, v2>, t<v0, v2, v1, v2>>);
static_assert(std::is_same_v<append_t<t<v0, v2, v2>, v2>, t<v0, v2, v2, v2>>);
static_assert(std::is_same_v<append_t<t<v1, v0, v0>, v2>, t<v1, v0, v0, v2>>);
static_assert(std::is_same_v<append_t<t<v1, v0, v1>, v2>, t<v1, v0, v1, v2>>);
static_assert(std::is_same_v<append_t<t<v1, v0, v2>, v2>, t<v1, v0, v2, v2>>);
static_assert(std::is_same_v<append_t<t<v1, v1, v0>, v2>, t<v1, v1, v0, v2>>);
static_assert(std::is_same_v<append_t<t<v1, v1, v1>, v2>, t<v1, v1, v1, v2>>);
static_assert(std::is_same_v<append_t<t<v1, v1, v2>, v2>, t<v1, v1, v2, v2>>);
static_assert(std::is_same_v<append_t<t<v1, v2, v0>, v2>, t<v1, v2, v0, v2>>);
static_assert(std::is_same_v<append_t<t<v1, v2, v1>, v2>, t<v1, v2, v1, v2>>);
static_assert(std::is_same_v<append_t<t<v1, v2, v2>, v2>, t<v1, v2, v2, v2>>);
static_assert(std::is_same_v<append_t<t<v2, v0, v0>, v2>, t<v2, v0, v0, v2>>);
static_assert(std::is_same_v<append_t<t<v2, v0, v1>, v2>, t<v2, v0, v1, v2>>);
static_assert(std::is_same_v<append_t<t<v2, v0, v2>, v2>, t<v2, v0, v2, v2>>);
static_assert(std::is_same_v<append_t<t<v2, v1, v0>, v2>, t<v2, v1, v0, v2>>);
static_assert(std::is_same_v<append_t<t<v2, v1, v1>, v2>, t<v2, v1, v1, v2>>);
static_assert(std::is_same_v<append_t<t<v2, v1, v2>, v2>, t<v2, v1, v2, v2>>);
static_assert(std::is_same_v<append_t<t<v2, v2, v0>, v2>, t<v2, v2, v0, v2>>);
static_assert(std::is_same_v<append_t<t<v2, v2, v1>, v2>, t<v2, v2, v1, v2>>);
static_assert(std::is_same_v<append_t<t<v2, v2, v2>, v2>, t<v2, v2, v2, v2>>);

// clang-format on

} // namespace
} // namespace dink::tuple
