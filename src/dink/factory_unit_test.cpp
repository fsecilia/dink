/*!
    \file
    Copyright (C) 2025 Frank Secilia
*/

#include <dink/factory.hpp>
#include <dink/test.hpp>

namespace dink {
namespace factories {
namespace {

template <int_t position>
struct param_t
{
    int_t index = position;
};

enum class source_t
{
    direct_ctor,
    static_construct_method,
    external
};

template <int_t num_params>
struct resolved_t;

template <>
struct resolved_t<0>
{
    source_t source = source_t::direct_ctor;

    static constexpr auto const expected_default_value = int_t{233};
    int_t actual_default_value = expected_default_value;

    resolved_t() noexcept = default;
    resolved_t(param_t<0> param) noexcept;
};

template <>
struct resolved_t<1>
{
    source_t source = source_t::direct_ctor;
    param_t<0> param;

    resolved_t(param_t<0> param) noexcept : param{param} {}
    resolved_t(param_t<0>, param_t<1>) noexcept;
};

template <>
struct resolved_t<2>
{
    source_t source = source_t::direct_ctor;
    param_t<0> param_0;
    param_t<1> param_1;

    resolved_t(param_t<0> param_0, param_t<1> param_1) noexcept : param_0{param_0}, param_1{param_1} {}
    resolved_t(param_t<0>, param_t<1>, param_t<2>) noexcept;
};

// ------------------------------------------------------------------------------------------------------------------ //

template <int_t num_params>
struct direct_ctor_resolved_t;

template <>
struct direct_ctor_resolved_t<0> : private resolved_t<0>
{
    using resolved_t<0>::resolved_t;
    using resolved_t<0>::actual_default_value;
    using resolved_t<0>::expected_default_value;
    using resolved_t<0>::source;
};

template <>
struct direct_ctor_resolved_t<1> : private resolved_t<1>
{
    using resolved_t<1>::resolved_t;
    using resolved_t<1>::source;
    using resolved_t<1>::param;
};

template <>
struct direct_ctor_resolved_t<2> : private resolved_t<2>
{
    using resolved_t<2>::resolved_t;
    using resolved_t<2>::source;
    using resolved_t<2>::param_0;
    using resolved_t<2>::param_1;
};

TEST(factory_test_t, direct_ctor_0)
{
    auto const actual = factory_t<direct_ctor_resolved_t<0>>{}();
    ASSERT_EQ(source_t::direct_ctor, actual.source);
    ASSERT_EQ(direct_ctor_resolved_t<0>::expected_default_value, actual.actual_default_value);
}

TEST(factory_test_t, direct_ctor_1)
{
    auto const expected = param_t<0>(3);
    auto const actual = factory_t<direct_ctor_resolved_t<1>>{}(expected);
    ASSERT_EQ(source_t::direct_ctor, actual.source);
    ASSERT_EQ(expected.index, actual.param.index);
}

TEST(factory_test_t, direct_ctor_2)
{
    auto const expected_0 = param_t<0>(3);
    auto const expected_1 = param_t<1>(5);
    auto const actual = factory_t<direct_ctor_resolved_t<2>>{}(expected_0, expected_1);
    ASSERT_EQ(source_t::direct_ctor, actual.source);
    ASSERT_EQ(expected_0.index, actual.param_0.index);
    ASSERT_EQ(expected_1.index, actual.param_1.index);
}

// ------------------------------------------------------------------------------------------------------------------ //

template <int_t num_params>
struct static_construct_method_resolved_t;

template <>
struct static_construct_method_resolved_t<0> : private direct_ctor_resolved_t<0>
{
    using direct_ctor_resolved_t<0>::source;
    using direct_ctor_resolved_t<0>::actual_default_value;
    using direct_ctor_resolved_t<0>::expected_default_value;

    static auto construct() noexcept -> static_construct_method_resolved_t
    {
        auto result = static_construct_method_resolved_t{};
        result.source = source_t::static_construct_method;
        return result;
    }

private:
    using direct_ctor_resolved_t<0>::direct_ctor_resolved_t;
};
static_assert(has_static_construct_method<static_construct_method_resolved_t<0>>);

template <>
struct static_construct_method_resolved_t<1> : private direct_ctor_resolved_t<1>
{
    using direct_ctor_resolved_t<1>::source;
    using direct_ctor_resolved_t<1>::param;

    static auto construct(param_t<0> param) noexcept -> static_construct_method_resolved_t
    {
        auto result = static_construct_method_resolved_t{param};
        result.source = source_t::static_construct_method;
        return result;
    }

private:
    using direct_ctor_resolved_t<1>::direct_ctor_resolved_t;
};

template <>
struct static_construct_method_resolved_t<2> : private direct_ctor_resolved_t<2>
{
    using direct_ctor_resolved_t<2>::source;
    using direct_ctor_resolved_t<2>::param_0;
    using direct_ctor_resolved_t<2>::param_1;

    static auto construct(param_t<0> param_0, param_t<1> param_1) noexcept -> static_construct_method_resolved_t
    {
        auto result = static_construct_method_resolved_t{param_0, param_1};
        result.source = source_t::static_construct_method;
        return result;
    }

private:
    using direct_ctor_resolved_t<2>::direct_ctor_resolved_t;
};

TEST(factory_test_t, static_construct_method_0)
{
    auto const actual = factory_t<static_construct_method_resolved_t<0>>{}();
    ASSERT_EQ(source_t::static_construct_method, actual.source);
    ASSERT_EQ(static_construct_method_resolved_t<0>::expected_default_value, actual.actual_default_value);
}

TEST(factory_test_t, static_construct_method_1)
{
    auto const expected = param_t<0>(3);
    auto const actual = factory_t<static_construct_method_resolved_t<1>>{}(expected);
    ASSERT_EQ(source_t::static_construct_method, actual.source);
    ASSERT_EQ(expected.index, actual.param.index);
}

TEST(factory_test_t, static_construct_method_2)
{
    auto const expected_0 = param_t<0>(3);
    auto const expected_1 = param_t<1>(5);
    auto const actual = factory_t<static_construct_method_resolved_t<2>>{}(expected_0, expected_1);
    ASSERT_EQ(source_t::static_construct_method, actual.source);
    ASSERT_EQ(expected_0.index, actual.param_0.index);
    ASSERT_EQ(expected_1.index, actual.param_1.index);
}

// ------------------------------------------------------------------------------------------------------------------ //

template <int_t num_params>
struct external_resolved_t;

template <int_t num_params>
struct external_resolved_factory_t
{
    template <typename... args_t>
    auto operator()(args_t&&... args) const noexcept -> external_resolved_t<num_params>
    {
        auto result = external_resolved_t<num_params>{std::forward<args_t>(args)...};
        result.source = source_t::external;
        return result;
    }
};

template <>
struct external_resolved_t<0> : private static_construct_method_resolved_t<0>
{
    using static_construct_method_resolved_t<0>::source;
    using static_construct_method_resolved_t<0>::actual_default_value;
    using static_construct_method_resolved_t<0>::expected_default_value;

private:
    friend external_resolved_factory_t<0>;
    using static_construct_method_resolved_t<0>::static_construct_method_resolved_t;
};

template <>
struct external_resolved_t<1> : private static_construct_method_resolved_t<1>
{
    using static_construct_method_resolved_t<1>::source;
    using static_construct_method_resolved_t<1>::param;

private:
    friend external_resolved_factory_t<1>;
    using static_construct_method_resolved_t<1>::static_construct_method_resolved_t;
};

template <>
struct external_resolved_t<2> : private static_construct_method_resolved_t<2>
{
    using static_construct_method_resolved_t<2>::source;
    using static_construct_method_resolved_t<2>::param_0;
    using static_construct_method_resolved_t<2>::param_1;

private:
    friend external_resolved_factory_t<2>;
    using static_construct_method_resolved_t<2>::static_construct_method_resolved_t;
};

} // namespace
} // namespace factories

template <int_t num_params>
struct factory_t<factories::external_resolved_t<num_params>>
    : factories::external_t<
          factories::external_resolved_t<num_params>, factories::external_resolved_factory_t<num_params>>
{
    factory_t()
        : factories::external_t<
              factories::external_resolved_t<num_params>,
              factories::external_resolved_factory_t<num_params>>{factories::external_resolved_factory_t<num_params>{}}
    {}
};

namespace factories {
namespace {

TEST(factory_test_t, external_0)
{
    auto const actual = factory_t<external_resolved_t<0>>{}();
    ASSERT_EQ(source_t::external, actual.source);
    ASSERT_EQ(external_resolved_t<0>::expected_default_value, actual.actual_default_value);
}

TEST(factory_test_t, external_1)
{
    auto const expected = param_t<0>(3);
    auto const actual = factory_t<external_resolved_t<1>>{}(expected);
    ASSERT_EQ(source_t::external, actual.source);
    ASSERT_EQ(expected.index, actual.param.index);
}

TEST(factory_test_t, external_2)
{
    auto const expected_0 = param_t<0>(3);
    auto const expected_1 = param_t<1>(5);
    auto const actual = factory_t<external_resolved_t<2>>{}(expected_0, expected_1);
    ASSERT_EQ(source_t::external, actual.source);
    ASSERT_EQ(expected_0.index, actual.param_0.index);
    ASSERT_EQ(expected_1.index, actual.param_1.index);
}

} // namespace
} // namespace factories
} // namespace dink
