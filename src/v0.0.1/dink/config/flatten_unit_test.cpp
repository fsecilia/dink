/*!
    \file
    Copyright (C) 2025 Frank Secilia
*/

#include "flatten.hpp"
#include <dink/test.hpp>

namespace dink {
namespace {

template <int id_t>
struct unique_value_t
{
    constexpr auto operator==(unique_value_t const&) const noexcept -> bool = default;
};

// ---------------------------------------------------------------------------------------------------------------------

using t0 = unique_value_t<0>;
using t1 = unique_value_t<1>;
using t2 = unique_value_t<2>;
using t3 = unique_value_t<3>;

static_assert(std::is_same_v<flatten_tuple_t<std::tuple<>>, std::tuple<>>);
static_assert(std::is_same_v<flatten_tuple_t<std::tuple<std::tuple<>>>, std::tuple<>>);

static_assert(std::is_same_v<flatten_tuple_t<std::tuple<t0>>, std::tuple<t0>>);
static_assert(std::is_same_v<flatten_tuple_t<std::tuple<t0, t1>>, std::tuple<t0, t1>>);
static_assert(std::is_same_v<flatten_tuple_t<std::tuple<t0, t1, t2>>, std::tuple<t0, t1, t2>>);
static_assert(std::is_same_v<flatten_tuple_t<std::tuple<t0, t1, t2, t3>>, std::tuple<t0, t1, t2, t3>>);

static_assert(std::is_same_v<flatten_tuple_t<std::tuple<std::tuple<>>>, std::tuple<>>);
static_assert(std::is_same_v<flatten_tuple_t<std::tuple<std::tuple<t0>>>, std::tuple<t0>>);
static_assert(std::is_same_v<flatten_tuple_t<std::tuple<std::tuple<t0, t1>>>, std::tuple<t0, t1>>);
static_assert(std::is_same_v<flatten_tuple_t<std::tuple<std::tuple<t0, t1, t2>>>, std::tuple<t0, t1, t2>>);
static_assert(std::is_same_v<flatten_tuple_t<std::tuple<std::tuple<t0, t1, t2, t3>>>, std::tuple<t0, t1, t2, t3>>);

static_assert(
    std::is_same_v<flatten_tuple_t<std::tuple<std::tuple<>, std::tuple<t0, t1, t2, t3>>>, std::tuple<t0, t1, t2, t3>>
);
static_assert(
    std::is_same_v<flatten_tuple_t<std::tuple<std::tuple<t0>, std::tuple<t1, t2, t3>>>, std::tuple<t0, t1, t2, t3>>
);
static_assert(
    std::is_same_v<flatten_tuple_t<std::tuple<std::tuple<t0, t1>, std::tuple<t2, t3>>>, std::tuple<t0, t1, t2, t3>>
);
static_assert(
    std::is_same_v<flatten_tuple_t<std::tuple<std::tuple<t0, t1, t2>, std::tuple<t3>>>, std::tuple<t0, t1, t2, t3>>
);
static_assert(
    std::is_same_v<flatten_tuple_t<std::tuple<std::tuple<t0, t1, t2, t3>, std::tuple<>>>, std::tuple<t0, t1, t2, t3>>
);

static_assert(
    std::is_same_v<
        flatten_tuple_t<std::tuple<std::tuple<std::tuple<>>, std::tuple<std::tuple<t0, t1, t2, t3>>>>,
        std::tuple<t0, t1, t2, t3>>
);
static_assert(
    std::is_same_v<
        flatten_tuple_t<std::tuple<std::tuple<std::tuple<t0>>, std::tuple<std::tuple<t1, t2, t3>>>>,
        std::tuple<t0, t1, t2, t3>>
);
static_assert(
    std::is_same_v<
        flatten_tuple_t<std::tuple<std::tuple<std::tuple<t0, t1>>, std::tuple<std::tuple<t2, t3>>>>,
        std::tuple<t0, t1, t2, t3>>
);
static_assert(
    std::is_same_v<
        flatten_tuple_t<std::tuple<std::tuple<std::tuple<t0, t1, t2>>, std::tuple<std::tuple<t3>>>>,
        std::tuple<t0, t1, t2, t3>>
);
static_assert(
    std::is_same_v<
        flatten_tuple_t<std::tuple<std::tuple<std::tuple<t0, t1, t2, t3>>, std::tuple<std::tuple<>>>>,
        std::tuple<t0, t1, t2, t3>>
);

// ---------------------------------------------------------------------------------------------------------------------

constexpr auto const v0 = t0{};
constexpr auto const v1 = t1{};
constexpr auto const v2 = t2{};
constexpr auto const v3 = t3{};

static_assert(flatten_tuple(std::make_tuple()) == std::make_tuple());
static_assert(flatten_tuple(std::make_tuple(std::make_tuple())) == std::make_tuple());

static_assert(flatten_tuple(std::make_tuple(v0)) == std::make_tuple(v0));
static_assert(flatten_tuple(std::make_tuple(v0, v1)) == std::make_tuple(v0, v1));
static_assert(flatten_tuple(std::make_tuple(v0, v1, v2)) == std::make_tuple(v0, v1, v2));
static_assert(flatten_tuple(std::make_tuple(v0, v1, v2, v3)) == std::make_tuple(v0, v1, v2, v3));

static_assert(flatten_tuple(std::make_tuple(std::make_tuple())) == std::make_tuple());
static_assert(flatten_tuple(std::make_tuple(std::make_tuple(v0))) == std::make_tuple(v0));
static_assert(flatten_tuple(std::make_tuple(std::make_tuple(v0, v1))) == std::make_tuple(v0, v1));
static_assert(flatten_tuple(std::make_tuple(std::make_tuple(v0, v1, v2))) == std::make_tuple(v0, v1, v2));
static_assert(flatten_tuple(std::make_tuple(std::make_tuple(v0, v1, v2, v3))) == std::make_tuple(v0, v1, v2, v3));

static_assert(
    flatten_tuple(std::make_tuple(std::make_tuple(), std::make_tuple(v0, v1, v2, v3)))
    == std::make_tuple(v0, v1, v2, v3)
);
static_assert(
    flatten_tuple(std::make_tuple(std::make_tuple(v0), std::make_tuple(v1, v2, v3))) == std::make_tuple(v0, v1, v2, v3)
);
static_assert(
    flatten_tuple(std::make_tuple(std::make_tuple(v0, v1), std::make_tuple(v2, v3))) == std::make_tuple(v0, v1, v2, v3)
);
static_assert(
    flatten_tuple(std::make_tuple(std::make_tuple(v0, v1, v2), std::make_tuple(v3))) == std::make_tuple(v0, v1, v2, v3)
);
static_assert(
    flatten_tuple(std::make_tuple(std::make_tuple(v0, v1, v2, v3), std::make_tuple()))
    == std::make_tuple(v0, v1, v2, v3)
);

static_assert(
    flatten_tuple(std::make_tuple(std::make_tuple(std::make_tuple()), std::make_tuple(std::make_tuple(v0, v1, v2, v3))))
    == std::make_tuple(v0, v1, v2, v3)
);
static_assert(
    flatten_tuple(std::make_tuple(std::make_tuple(std::make_tuple(v0)), std::make_tuple(std::make_tuple(v1, v2, v3))))
    == std::make_tuple(v0, v1, v2, v3)
);
static_assert(
    flatten_tuple(std::make_tuple(std::make_tuple(std::make_tuple(v0, v1)), std::make_tuple(std::make_tuple(v2, v3))))
    == std::make_tuple(v0, v1, v2, v3)
);
static_assert(
    flatten_tuple(std::make_tuple(std::make_tuple(std::make_tuple(v0, v1, v2)), std::make_tuple(std::make_tuple(v3))))
    == std::make_tuple(v0, v1, v2, v3)
);
static_assert(
    flatten_tuple(std::make_tuple(std::make_tuple(std::make_tuple(v0, v1, v2, v3)), std::make_tuple(std::make_tuple())))
    == std::make_tuple(v0, v1, v2, v3)
);

// ---------------------------------------------------------------------------------------------------------------------

static_assert(std::same_as<
              flatten_t<std::tuple<>, t0, std::tuple<t1>, std::tuple<std::tuple<t2, t3>>>, std::tuple<t0, t1, t2, t3>>);

static_assert(
    flatten(std::make_tuple(), v0, std::make_tuple(v1), std::make_tuple(std::make_tuple(v2, v3)))
    == std::make_tuple(v0, v1, v2, v3)
);

} // namespace
} // namespace dink
