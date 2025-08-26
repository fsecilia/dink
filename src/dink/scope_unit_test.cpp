/*!
    \file
    Copyright (C) 2025 Frank Secilia
*/

#include <dink/test.hpp>
#include <dink/scope.hpp>

namespace dink::scope {
namespace {

struct resolvers_scope_test_t : Test
{
    struct resolved_t
    {
        static int_t const default_id = 3;
        static int_t const expected_id = default_id + 1;
        int_t id = default_id;
        auto operator<=>(resolved_t const& src) const noexcept -> auto = default;
        auto operator==(resolved_t const& src) const noexcept -> bool = default;
    };

    struct composer_t
    {
        resolved_t resolved;

        template <typename>
        auto resolve() const noexcept -> resolved_t
        {
            return resolved;
        }
    };
    composer_t composer{resolved_t{.id = resolved_t::expected_id}};
};

// ---------------------------------------------------------------------------------------------------------------------

struct resolvers_scope_test_global_t : resolvers_scope_test_t
{
    using sut_t = global_t;
    sut_t sut{};
};

TEST_F(resolvers_scope_test_global_t, initially_unresolved)
{
    ASSERT_EQ(nullptr, sut.template resolved<resolved_t>());
}

// vvvvvv -- from here and below, the resolved instance stays resolved until program end, not test teardown -- vvvvvv
TEST_F(resolvers_scope_test_global_t, resolve_uses_composer)
{
    ASSERT_EQ(composer.resolved, sut.template resolve<resolved_t>(composer));
}

TEST_F(resolvers_scope_test_global_t, resolve_is_idempotent)
{
    ASSERT_EQ(&sut.template resolve<resolved_t>(composer), &sut.template resolve<resolved_t>(composer));
}

TEST_F(resolvers_scope_test_global_t, resolved_matches_resolve)
{
    ASSERT_EQ(sut.template resolved<resolved_t>(), &sut.template resolve<resolved_t>(composer));
}

TEST_F(resolvers_scope_test_global_t, resolved_is_idempotent)
{
    ASSERT_EQ(sut.template resolved<resolved_t>(), sut.template resolved<resolved_t>());
}

// ---------------------------------------------------------------------------------------------------------------------

struct resolvers_scope_test_local_t : resolvers_scope_test_t
{
    struct parent_t
    {
        resolved_t* resolved_result = nullptr;

        template <typename>
        auto resolved() const noexcept -> resolved_t*
        {
            return resolved_result;
        }
    };
    parent_t parent;

    using sut_t = local_t<parent_t>;
    sut_t sut{parent};
};

TEST_F(resolvers_scope_test_local_t, initially_unresolved)
{
    ASSERT_EQ(nullptr, sut.resolved<resolved_t>());
}

TEST_F(resolvers_scope_test_local_t, resolve_uses_composer)
{
    ASSERT_EQ(composer.resolved, sut.template resolve<resolved_t>(composer));
}

TEST_F(resolvers_scope_test_local_t, resolve_is_idempotent)
{
    ASSERT_EQ(&sut.template resolve<resolved_t>(composer), &sut.template resolve<resolved_t>(composer));
}

// ---------------------------------------------------------------------------------------------------------------------

struct resolvers_scope_test_local_resolved_t : resolvers_scope_test_local_t
{
    resolvers_scope_test_local_resolved_t() { sut.template resolve<resolved_t>(composer); }
};

TEST_F(resolvers_scope_test_local_resolved_t, resolved_matches_resolve)
{
    ASSERT_EQ(sut.template resolved<resolved_t>(), &sut.template resolve<resolved_t>(composer));
}

TEST_F(resolvers_scope_test_local_resolved_t, resolved_is_idempotent)
{
    ASSERT_EQ(sut.template resolved<resolved_t>(), sut.template resolved<resolved_t>());
}

// ---------------------------------------------------------------------------------------------------------------------

struct resolvers_scope_test_local_resolved_by_parent_t : resolvers_scope_test_local_t
{
    resolved_t parent_resolved_result;

    resolvers_scope_test_local_resolved_by_parent_t() noexcept { parent.resolved_result = &parent_resolved_result; }
};

TEST_F(resolvers_scope_test_local_resolved_by_parent_t, resolve_returns_from_parent)
{
    ASSERT_EQ(&parent_resolved_result, &sut.resolve<resolved_t>(composer));
}

TEST_F(resolvers_scope_test_local_resolved_by_parent_t, resolve_is_idempotent)
{
    ASSERT_EQ(&sut.resolve<resolved_t>(composer), &sut.resolve<resolved_t>(composer));
}

TEST_F(resolvers_scope_test_local_resolved_by_parent_t, resolved_returns_from_parent)
{
    ASSERT_EQ(&parent_resolved_result, sut.resolved<resolved_t>());
}

TEST_F(resolvers_scope_test_local_resolved_by_parent_t, resolved_is_idempotent)
{
    ASSERT_EQ(sut.resolved<resolved_t>(), sut.resolved<resolved_t>());
}

} // namespace
} // namespace dink::scope
