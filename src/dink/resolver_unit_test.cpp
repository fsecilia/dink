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
        auto operator==(resolved_t const& src) const noexcept -> bool = default;
    };

    struct composer_t
    {
        mutable resolved_t resolved;

        template <typename resolved_t>
        auto resolve() const -> resolved_t&
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
        inline static constexpr auto const resolved = true;
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

    struct scope_t
    {
        resolved_t* resolved = nullptr;

        template <typename resolved_t, typename composer_t>
        auto resolve(composer_t& composer) const -> resolved_t&
        {
            return *resolved;
        }
    };

    template <typename parent_t>
    struct nested_scope_t : scope_t
    {
        parent_t* parent;
        nested_scope_t(parent_t& parent) noexcept : parent{&parent} {}
    };

    using sut_t = shared_t<binding_t, scope_t, nested_scope_t>;

    resolved_t mutable_resolved = expected_resolved;
    sut_t sut{scope_t{&mutable_resolved}};

    ~shared_resolver_test_t() override { sut.template unbind<resolved_t>(); }
};

TEST_F(shared_resolver_test_t, resolve)
{
    ASSERT_EQ(&mutable_resolved, &sut.template resolve<resolved_t>(composer));
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
    sut.bind(mutable_resolved);

    // returns initial
    ASSERT_EQ(mutable_resolved, sut.template resolve<resolved_t>(composer));

    // set new
    auto const expected_id = mutable_resolved.id + 1;
    mutable_resolved.id = expected_id;

    // new is reflected in result
    ASSERT_EQ(expected_id, sut.template resolve<resolved_t>(composer).id);
}

TEST_F(shared_resolver_test_t, scope)
{
    ASSERT_EQ(&sut.scope(), &const_cast<sut_t const&>(sut).scope());
}

// ---------------------------------------------------------------------------------------------------------------------

struct shared_resolver_nested_test_t : shared_resolver_test_t
{
    sut_t::nested_t nested = sut.create_nested();
};

TEST_F(shared_resolver_nested_test_t, scope_parent)
{
    ASSERT_EQ(&sut.scope(), nested.scope().parent);
}

TEST_F(shared_resolver_nested_test_t, resolve)
{
    auto nested_resolved = resolved_t{};
    nested.scope().resolved = &nested_resolved;

    auto& nested_result = nested.template resolve<resolved_t>(composer);
    ASSERT_EQ(&nested_resolved, &nested_result);
}

} // namespace
} // namespace dink::resolvers
