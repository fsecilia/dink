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
    struct composer_t
    {
        static int_t const default_id = 3;
        static int_t const expected_id = default_id + 1;
        int_t id = default_id;

        template <typename resolved_t>
        auto resolve() const -> resolved_t
        {
            return *this;
        }
    };
    composer_t composer{.id = composer_t::expected_id};

    using resolved_t = composer_t;
    using requested_t = resolved_t;
};

// ------------------------------------------------------------------------------------------------------------------ //

struct transient_resolver_test_t : resolver_test_t
{
    template <typename, typename, int_t>
    struct arg_t
    {};

    template <typename, typename composer_t, typename, template <typename, typename, int_t> class, typename...>
    struct dispatcher_t
    {
        auto operator()(composer_t& composer) noexcept -> composer_t& { return composer; }
    };

    template <typename>
    struct factory_t
    {};

    template <typename resolved_t>
    struct binding_t
    {
        std::optional<resolved_t> resolved;

        template <typename forwarded_resolved_t>
        constexpr auto bind(forwarded_resolved_t&& resolved) -> void
        {
            binding_t::resolved.emplace(std::forward<forwarded_resolved_t>(resolved));
        }

        constexpr auto unbind() noexcept -> void { resolved.reset(); }
        constexpr auto is_bound() const noexcept -> bool { return resolved.has_value(); }
        constexpr auto bound() const noexcept -> resolved_t const& { return *resolved; }
        constexpr auto bound() noexcept -> resolved_t& { return *resolved; }
    };

    using sut_t = transient_t<dispatcher_t, factory_t, binding_t, arg_t>;
    sut_t sut{};
};

TEST_F(transient_resolver_test_t, resolve)
{
    ASSERT_EQ(composer_t::expected_id, (sut.template resolve<requested_t, resolved_t>(composer).id));
}

TEST_F(transient_resolver_test_t, resolve_with_binding)
{
    auto const bound_id = composer_t::expected_id + 1;
    sut.bind(composer_t{bound_id});
    ASSERT_EQ(bound_id, (sut.template resolve<requested_t, resolved_t>(composer).id));
}

TEST_F(transient_resolver_test_t, resolve_after_unbinding)
{
    auto const bound_id = composer_t::expected_id + 1;
    sut.bind(composer_t{bound_id});
    sut.template unbind<composer_t>();
    ASSERT_EQ(composer_t::expected_id, (sut.template resolve<requested_t, resolved_t>(composer).id));
}

// ------------------------------------------------------------------------------------------------------------------ //

struct shared_resolver_test_t : resolver_test_t
{
    template <typename resolved_t>
    struct binding_t
    {
        static_assert(!std::is_reference_v<resolved_t>);

        resolved_t* resolved;

        template <typename forwarded_resolved_t>
        constexpr auto bind(forwarded_resolved_t&& forwarded_resolved) -> void
        {
            resolved = &forwarded_resolved;
        }

        constexpr auto unbind() noexcept -> void { resolved = nullptr; }
        constexpr auto is_bound() const noexcept -> bool { return resolved; }
        constexpr auto bound() const noexcept -> resolved_t const& { return *resolved; }
        constexpr auto bound() noexcept -> resolved_t& { return *resolved; }
    };

    using sut_t = shared_t<binding_t>;

    sut_t sut{};
};

TEST_F(shared_resolver_test_t, expected_result)
{
    ASSERT_EQ(composer_t::expected_id, (sut.template resolve<requested_t, resolved_t>(composer).id));
}

TEST_F(shared_resolver_test_t, result_is_stored_locally)
{
    ASSERT_NE(&composer, (&sut.template resolve<requested_t, resolved_t>(composer)));
}

TEST_F(shared_resolver_test_t, result_is_repeatable)
{
    ASSERT_EQ(
        (&sut.template resolve<requested_t, resolved_t>(composer)),
        (&sut.template resolve<requested_t, resolved_t>(composer))
    );
}

TEST_F(shared_resolver_test_t, resolve_with_binding)
{
    auto const bound_id = composer_t::expected_id + 1;
    auto bound_composer = composer_t{bound_id};
    sut.bind(bound_composer);
    ASSERT_EQ(bound_id, (sut.template resolve<requested_t, resolved_t>(composer).id));
}

TEST_F(shared_resolver_test_t, bound_instances_are_mutable_references)
{
    // bind initial
    auto bound_composer = composer_t{composer_t::expected_id};
    sut.bind(bound_composer);

    // still returns initial
    ASSERT_EQ(composer_t::expected_id, (sut.template resolve<requested_t, resolved_t>(composer).id));

    // set new id in bound composer
    auto const new_id = bound_composer.id + 1;
    bound_composer.id = new_id;

    // new id is reflected in result
    ASSERT_EQ(new_id, (sut.template resolve<requested_t, resolved_t>(composer).id));
}

TEST_F(shared_resolver_test_t, resolve_after_unbinding)
{
    auto const bound_id = composer_t::expected_id + 1;
    sut.bind(composer_t{bound_id});
    sut.template unbind<composer_t>();
    ASSERT_EQ(composer_t::expected_id, (sut.template resolve<requested_t, resolved_t>(composer).id));
}

} // namespace
} // namespace dink::resolvers
