/*!
    \file
    Copyright (C) 2025 Frank Secilia
*/

#include "arity.hpp"
#include <dink/test.hpp>

namespace dink {
namespace {

template <typename... args_t>
struct constructed_by_t
{
    constructed_by_t(args_t...) noexcept {}
};

static_assert(arity_v<constructed_by_t<>> == 0);
static_assert(arity_v<constructed_by_t<int>> == 1);
static_assert(arity_v<constructed_by_t<int*, float>> == 2);
static_assert(arity_v<constructed_by_t<void*, constructed_by_t<>, int>> == 3);
static_assert(arity_v<constructed_by_t<void*, constructed_by_t<>, int, float>> == 4);
static_assert(arity_v<constructed_by_t<void*, constructed_by_t<>, int, float, bool>> == 5);

struct multi_ctor_t
{
    multi_ctor_t();
    multi_ctor_t(int);
    multi_ctor_t(int, float, double);
};

// makes sure search is greedy
static_assert(arity_v<multi_ctor_t> == 3);

} // namespace
} // namespace dink
