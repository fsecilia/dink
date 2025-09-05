/*!
    \file
    Copyright (C) 2025 Frank Secilia
*/

#include "tuple.hpp"
#include <dink/test.hpp>

namespace dink::detail::tuple {
namespace {

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

template <typename key_p>
struct keyed_value_t
{
    using key_t = key_p;
};

using k0 = keyed_value_t<unique_value_t<0>>;
using k1 = keyed_value_t<unique_value_t<1>>;
using k2 = keyed_value_t<unique_value_t<2>>;

// exhaustive test of all combinations of elements containing keys with values 0, 1, 2 of lengths up to 3

static_assert(std::is_same_v<unique_by_key_t<t<>>, t<>>);
static_assert(std::is_same_v<unique_by_key_t<t<k0>>, t<k0>>);
static_assert(std::is_same_v<unique_by_key_t<t<k1>>, t<k1>>);
static_assert(std::is_same_v<unique_by_key_t<t<k2>>, t<k2>>);
static_assert(std::is_same_v<unique_by_key_t<t<k0, k0>>, t<k0>>);
static_assert(std::is_same_v<unique_by_key_t<t<k0, k1>>, t<k0, k1>>);
static_assert(std::is_same_v<unique_by_key_t<t<k0, k2>>, t<k0, k2>>);
static_assert(std::is_same_v<unique_by_key_t<t<k1, k0>>, t<k1, k0>>);
static_assert(std::is_same_v<unique_by_key_t<t<k1, k1>>, t<k1>>);
static_assert(std::is_same_v<unique_by_key_t<t<k1, k2>>, t<k1, k2>>);
static_assert(std::is_same_v<unique_by_key_t<t<k2, k0>>, t<k2, k0>>);
static_assert(std::is_same_v<unique_by_key_t<t<k2, k1>>, t<k2, k1>>);
static_assert(std::is_same_v<unique_by_key_t<t<k2, k2>>, t<k2>>);
static_assert(std::is_same_v<unique_by_key_t<t<k0, k0, k0>>, t<k0>>);
static_assert(std::is_same_v<unique_by_key_t<t<k0, k0, k1>>, t<k0, k1>>);
static_assert(std::is_same_v<unique_by_key_t<t<k0, k0, k2>>, t<k0, k2>>);
static_assert(std::is_same_v<unique_by_key_t<t<k0, k1, k0>>, t<k0, k1>>);
static_assert(std::is_same_v<unique_by_key_t<t<k0, k1, k1>>, t<k0, k1>>);
static_assert(std::is_same_v<unique_by_key_t<t<k0, k1, k2>>, t<k0, k1, k2>>);
static_assert(std::is_same_v<unique_by_key_t<t<k0, k2, k0>>, t<k0, k2>>);
static_assert(std::is_same_v<unique_by_key_t<t<k0, k2, k1>>, t<k0, k2, k1>>);
static_assert(std::is_same_v<unique_by_key_t<t<k0, k2, k2>>, t<k0, k2>>);
static_assert(std::is_same_v<unique_by_key_t<t<k1, k0, k0>>, t<k1, k0>>);
static_assert(std::is_same_v<unique_by_key_t<t<k1, k0, k1>>, t<k1, k0>>);
static_assert(std::is_same_v<unique_by_key_t<t<k1, k0, k2>>, t<k1, k0, k2>>);
static_assert(std::is_same_v<unique_by_key_t<t<k1, k1, k0>>, t<k1, k0>>);
static_assert(std::is_same_v<unique_by_key_t<t<k1, k1, k1>>, t<k1>>);
static_assert(std::is_same_v<unique_by_key_t<t<k1, k1, k2>>, t<k1, k2>>);
static_assert(std::is_same_v<unique_by_key_t<t<k1, k2, k0>>, t<k1, k2, k0>>);
static_assert(std::is_same_v<unique_by_key_t<t<k1, k2, k1>>, t<k1, k2>>);
static_assert(std::is_same_v<unique_by_key_t<t<k1, k2, k2>>, t<k1, k2>>);
static_assert(std::is_same_v<unique_by_key_t<t<k2, k0, k0>>, t<k2, k0>>);
static_assert(std::is_same_v<unique_by_key_t<t<k2, k0, k1>>, t<k2, k0, k1>>);
static_assert(std::is_same_v<unique_by_key_t<t<k2, k0, k2>>, t<k2, k0>>);
static_assert(std::is_same_v<unique_by_key_t<t<k2, k1, k0>>, t<k2, k1, k0>>);
static_assert(std::is_same_v<unique_by_key_t<t<k2, k1, k1>>, t<k2, k1>>);
static_assert(std::is_same_v<unique_by_key_t<t<k2, k1, k2>>, t<k2, k1>>);
static_assert(std::is_same_v<unique_by_key_t<t<k2, k2, k0>>, t<k2, k0>>);
static_assert(std::is_same_v<unique_by_key_t<t<k2, k2, k1>>, t<k2, k1>>);
static_assert(std::is_same_v<unique_by_key_t<t<k2, k2, k2>>, t<k2>>);

// clang-format on

} // namespace
} // namespace dink::detail::tuple
