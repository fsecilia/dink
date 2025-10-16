/*!
    \file
    Copyright (C) 2025 Frank Secilia
*/

#include "arity.hpp"
#include <dink/test.hpp>

namespace dink {
namespace {

template <typename... args_t>
struct constructed_by_t {
    constructed_by_t(args_t...) noexcept;
};

static_assert(arity_v<constructed_by_t<>> == 0);
static_assert(arity_v<constructed_by_t<bool>> == 1);
static_assert(arity_v<constructed_by_t<int, bool>> == 2);
static_assert(arity_v<constructed_by_t<void*, int, bool>> == 3);
static_assert(arity_v<constructed_by_t<float&, void*, int, bool>> == 4);
static_assert(arity_v<constructed_by_t<constructed_by_t<>&&, float&, void*, int, bool>> == 5);
static_assert(arity_v<constructed_by_t<bool, int, void const*, float const&, constructed_by_t<>&&>> == 5);
static_assert(arity_v<constructed_by_t<int, int, int, int, int, int, int, int, int, int>> == 10);

struct multi_ctor_t {
    multi_ctor_t();
    multi_ctor_t(int);
    multi_ctor_t(int, float, double);
};

// makes sure search is greedy
static_assert(arity_v<multi_ctor_t> == 3);

template <typename... args_t>
struct factory_t {
    // matching call operator
    auto operator()(args_t...) -> constructed_by_t<args_t...>;

    // confounding overloaded call operator
    auto operator()(bool, args_t...) -> bool;
};

static_assert(arity_v<constructed_by_t<>, factory_t<>> == 0);
static_assert(arity_v<constructed_by_t<bool>, factory_t<bool>> == 1);
static_assert(arity_v<constructed_by_t<int, bool>, factory_t<int, bool>> == 2);
static_assert(arity_v<constructed_by_t<void*, int, bool>, factory_t<void*, int, bool>> == 3);
static_assert(arity_v<constructed_by_t<float&, void*, int, bool>, factory_t<float&, void*, int, bool>> == 4);
static_assert(arity_v<constructed_by_t<constructed_by_t<>&&, float&, void*, int, bool>,
                      factory_t<constructed_by_t<>&&, float&, void*, int, bool>> == 5);
static_assert(arity_v<constructed_by_t<bool, int, void const*, float const&, constructed_by_t<>&&>,
                      factory_t<bool, int, void const*, float const&, constructed_by_t<>&&>> == 5);
static_assert(arity_v<constructed_by_t<int, int, int, int, int, int, int, int, int, int>,
                      factory_t<int, int, int, int, int, int, int, int, int, int>> == 10);

}  // namespace
}  // namespace dink
