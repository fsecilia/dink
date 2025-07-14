/*!
    \file
    Copyright (C) 2025 Frank Secilia
*/

#include <dink/test.hpp>
#include <dink/dink.hpp>

namespace dink {
namespace {

inline static int_t next_id = 1;

template <typename... args_t>
struct constructible_t
{
    int_t id = next_id++;
    constructible_t(args_t...) noexcept {}
};

// ---------------------------------------------------------------------------------------------------------------------

struct integration_test_t : Test
{
    using sut_t = dink_t;
    sut_t sut{};

    template <typename resolved_t>
    struct expected_t
    {
        using result_t = resolved_t;
    };

    template <typename resolved_t>
    struct expected_t<resolved_t&&>
    {
        using result_t = resolved_t;
    };

    template <typename resolved_t>
    auto resolve() -> decltype(auto)
    {
        return static_cast<typename expected_t<resolved_t>::result_t>(sut.template resolve<resolved_t>());
    }

    template <typename value_t>
    auto test_resolve()
    {
        auto const& value = resolve<value_t>();
        auto const& ref = resolve<value_t&>();
        ASSERT_NE(&value, &ref);
    }
};

TEST_F(integration_test_t, vals_are_transient)
{
    ASSERT_NE(resolve<constructible_t<>>().id, resolve<constructible_t<>&&>().id);
}

TEST_F(integration_test_t, refs_are_shared)
{
    ASSERT_EQ(resolve<constructible_t<>&>().id, resolve<constructible_t<> const&>().id);
}

TEST_F(integration_test_t, rrefs_are_transient)
{
    ASSERT_NE(resolve<constructible_t<>&&>().id, resolve<constructible_t<> const&&>().id);
}

TEST_F(integration_test_t, default_constructible)
{
    test_resolve<constructible_t<>>();
}

TEST_F(integration_test_t, constructible_from_val)
{
    test_resolve<constructible_t<constructible_t<>>>();
}

TEST_F(integration_test_t, constructible_from_lref)
{
    test_resolve<constructible_t<constructible_t<>&>>();
}

TEST_F(integration_test_t, constructible_from_rref)
{
    test_resolve<constructible_t<constructible_t<>&&>>();
}

TEST_F(integration_test_t, constructible_from_cref)
{
    test_resolve<constructible_t<constructible_t<> const&>>();
}

TEST_F(integration_test_t, constructible_from_rcref)
{
    test_resolve<constructible_t<constructible_t<> const&&>>();
}

TEST_F(integration_test_t, constructible_from_val_val)
{
    test_resolve<constructible_t<constructible_t<>, constructible_t<>>>();
}

TEST_F(integration_test_t, constructible_from_val_lref)
{
    test_resolve<constructible_t<constructible_t<>, constructible_t<>&>>();
}

TEST_F(integration_test_t, constructible_from_val_rref)
{
    test_resolve<constructible_t<constructible_t<>, constructible_t<>&&>>();
}

TEST_F(integration_test_t, constructible_from_val_cref)
{
    test_resolve<constructible_t<constructible_t<>, constructible_t<> const&>>();
}

TEST_F(integration_test_t, constructible_from_val_rcref)
{
    test_resolve<constructible_t<constructible_t<>, constructible_t<> const&&>>();
}

TEST_F(integration_test_t, constructible_from_lref_val)
{
    test_resolve<constructible_t<constructible_t<>&, constructible_t<>>>();
}

TEST_F(integration_test_t, constructible_from_lref_lref)
{
    test_resolve<constructible_t<constructible_t<>&, constructible_t<>&>>();
}

TEST_F(integration_test_t, constructible_from_lref_rref)
{
    test_resolve<constructible_t<constructible_t<>&, constructible_t<>&&>>();
}

TEST_F(integration_test_t, constructible_from_lref_cref)
{
    test_resolve<constructible_t<constructible_t<>&, constructible_t<> const&>>();
}

TEST_F(integration_test_t, constructible_from_lref_rcref)
{
    test_resolve<constructible_t<constructible_t<>&, constructible_t<> const&&>>();
}

TEST_F(integration_test_t, constructible_from_rref_val)
{
    test_resolve<constructible_t<constructible_t<>&&, constructible_t<>>>();
}

TEST_F(integration_test_t, constructible_from_rref_lref)
{
    test_resolve<constructible_t<constructible_t<>&&, constructible_t<>&>>();
}

TEST_F(integration_test_t, constructible_from_rref_rref)
{
    test_resolve<constructible_t<constructible_t<>&&, constructible_t<>&&>>();
}

TEST_F(integration_test_t, constructible_from_rref_cref)
{
    test_resolve<constructible_t<constructible_t<>&&, constructible_t<> const&>>();
}

TEST_F(integration_test_t, constructible_from_rref_rcref)
{
    test_resolve<constructible_t<constructible_t<>&&, constructible_t<> const&&>>();
}

TEST_F(integration_test_t, constructible_from_cref_val)
{
    test_resolve<constructible_t<constructible_t<> const&, constructible_t<>>>();
}

TEST_F(integration_test_t, constructible_from_cref_lref)
{
    test_resolve<constructible_t<constructible_t<> const&, constructible_t<>&>>();
}

TEST_F(integration_test_t, constructible_from_cref_rref)
{
    test_resolve<constructible_t<constructible_t<> const&, constructible_t<>&&>>();
}

TEST_F(integration_test_t, constructible_from_cref_cref)
{
    test_resolve<constructible_t<constructible_t<> const&, constructible_t<> const&>>();
}

TEST_F(integration_test_t, constructible_from_cref_rcref)
{
    test_resolve<constructible_t<constructible_t<> const&, constructible_t<> const&&>>();
}

TEST_F(integration_test_t, constructible_from_rcref_val)
{
    test_resolve<constructible_t<constructible_t<> const&&, constructible_t<>>>();
}

TEST_F(integration_test_t, constructible_from_rcref_lref)
{
    test_resolve<constructible_t<constructible_t<> const&&, constructible_t<>&>>();
}

TEST_F(integration_test_t, constructible_from_rcref_rref)
{
    test_resolve<constructible_t<constructible_t<> const&&, constructible_t<>&&>>();
}

TEST_F(integration_test_t, constructible_from_rcref_cref)
{
    test_resolve<constructible_t<constructible_t<> const&&, constructible_t<> const&>>();
}

TEST_F(integration_test_t, constructible_from_rcref_rcref)
{
    test_resolve<constructible_t<constructible_t<> const&&, constructible_t<> const&&>>();
}

// ---------------------------------------------------------------------------------------------------------------------

struct integration_test_deep_graph_t : integration_test_t
{
    template <typename value_t>
    using nested_t = constructible_t<value_t, value_t&, value_t&&, value_t const, value_t const&, value_t const&&>;

    using val_t = constructible_t<>;
    using obj_t = nested_t<val_t>;
    using graph_t = nested_t<obj_t>;
};

TEST_F(integration_test_deep_graph_t, resolve)
{
    test_resolve<graph_t>();
}

} // namespace
} // namespace dink
