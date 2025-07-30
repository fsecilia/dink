/*!
    \file
    Copyright (C) 2025 Frank Secilia
*/

#include <dink/test.hpp>
#include <dink/resolver.hpp>

namespace dink::resolvers {
namespace {

struct resolver_test_t : Test
{
    struct resolved_t
    {
        static int_t const default_id = 3;
        static int_t const expected_id = default_id + 1;
        int_t id = default_id;

        auto operator<=>(resolved_t const& src) const noexcept -> auto = default;
    };

    struct composer_t
    {
        resolved_t resolved;

        template <typename resolved_t>
        auto resolve() const -> resolved_t
        {
            return resolved;
        }
    };

    resolved_t const expected_resolved = resolved_t{.id = resolved_t::expected_id};
    composer_t composer{.resolved = expected_resolved};
};

// ---------------------------------------------------------------------------------------------------------------------

struct transient_resolver_test_t : resolver_test_t
{
    template <typename, typename, int_t>
    struct arg_t
    {};

    template <typename, typename composer_t, typename, template <typename, typename, int_t> class, typename...>
    struct dispatcher_t
    {
        auto operator()(composer_t& composer) noexcept -> resolved_t& { return composer.resolved; }
    };

    template <typename>
    struct factory_t
    {};

    template <typename resolved_t>
    struct binding_t
    {
        std::optional<resolved_t> resolved;

        constexpr auto bind(resolved_t bound) -> void { resolved = bound; }

        constexpr auto unbind() noexcept -> void { resolved.reset(); }
        constexpr auto is_bound() const noexcept -> bool { return resolved.has_value(); }
        constexpr auto bound() const noexcept -> resolved_t const& { return *resolved; }
        constexpr auto bound() noexcept -> resolved_t& { return *resolved; }
    };

    using sut_t = transient_t<dispatcher_t, factory_t, binding_t, arg_t>;
    sut_t sut{};
};

TEST_F(transient_resolver_test_t, expected_result)
{
    ASSERT_EQ(expected_resolved, sut.template resolve<resolved_t>(composer));
}

TEST_F(transient_resolver_test_t, resolve_with_binding)
{
    auto const bound = resolved_t{expected_resolved.id + 1};
    sut.bind(bound);
    ASSERT_EQ(bound, sut.template resolve<resolved_t>(composer));
}

TEST_F(transient_resolver_test_t, resolve_after_unbinding)
{
    auto const bound = resolved_t{expected_resolved.id + 1};
    sut.bind(bound);
    sut.template unbind<resolved_t>();
    ASSERT_EQ(expected_resolved, sut.template resolve<resolved_t>(composer));
}

// ---------------------------------------------------------------------------------------------------------------------

struct shared_resolver_test_t : resolver_test_t
{
    template <typename resolved_t>
    struct binding_t
    {
        static_assert(!std::is_reference_v<resolved_t>);

        resolved_t* resolved = nullptr;

        constexpr auto bind(resolved_t& bound) -> void { resolved = &bound; }

        constexpr auto unbind() noexcept -> void { resolved = nullptr; }
        constexpr auto is_bound() const noexcept -> bool { return resolved; }

        template <typename immediate_resolved_t>
        constexpr auto bound() const noexcept -> immediate_resolved_t&
        {
            return *resolved;
        }
    };

    using sut_t = shared_t<binding_t>;

    sut_t sut{};
};

TEST_F(shared_resolver_test_t, expected_result)
{
    ASSERT_EQ(expected_resolved, sut.template resolve<resolved_t>(composer));
}

TEST_F(shared_resolver_test_t, result_is_stored_locally)
{
    ASSERT_NE(&expected_resolved, &sut.template resolve<resolved_t>(composer));
}

TEST_F(shared_resolver_test_t, result_is_repeatable)
{
    ASSERT_EQ(&sut.template resolve<resolved_t>(composer), &sut.template resolve<resolved_t>(composer));
}

TEST_F(shared_resolver_test_t, resolve_with_binding)
{
    auto bound = resolved_t{expected_resolved.id + 1};
    sut.bind(bound);
    ASSERT_EQ(&bound, &sut.template resolve<resolved_t>(composer));
}

TEST_F(shared_resolver_test_t, resolve_after_unbinding)
{
    auto bound = resolved_t{expected_resolved.id + 1};
    sut.bind(bound);
    sut.template unbind<resolved_t>();
    ASSERT_EQ(expected_resolved, sut.template resolve<resolved_t>(composer));
}

TEST_F(shared_resolver_test_t, bound_instances_are_mutable_references)
{
    // bind initial
    auto mutable_resolved = expected_resolved;
    sut.bind(mutable_resolved);

    // returns initial
    ASSERT_EQ(mutable_resolved, sut.template resolve<resolved_t>(composer));

    // set new
    auto const expected_id = mutable_resolved.id + 1;
    mutable_resolved.id = expected_id;

    // new is reflected in result
    ASSERT_EQ(expected_id, sut.template resolve<resolved_t>(composer).id);
}

} // namespace
} // namespace dink::resolvers
