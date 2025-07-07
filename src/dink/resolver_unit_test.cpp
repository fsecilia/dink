/*!
    \file
    Copyright (C) 2025 Frank Secilia
*/

#include <dink/unit_test.hpp>
#include <dink/resolver.hpp>

namespace dink::resolvers {
namespace {

struct transient_resolver_test_t : Test
{
    template <typename, typename, int_t>
    struct arg_t
    {};

    struct composer_t
    {};

    template <typename, typename composer_t, typename, template <typename, typename, int_t> class, typename...>
    struct dispatcher_t;

    template <typename>
    struct factory_t
    {};

    using resolved_t = composer_t;

    using sut_t = transient_t<dispatcher_t, factory_t, arg_t>;

    sut_t sut{};
    composer_t composer{};
};

// define only the dispatchers we expect
template <typename... args_t>
struct transient_resolver_test_t::dispatcher_t<
    transient_resolver_test_t::resolved_t, transient_resolver_test_t::composer_t,
    transient_resolver_test_t::factory_t<transient_resolver_test_t::resolved_t>, transient_resolver_test_t::arg_t,
    args_t...>
{
    auto operator()(composer_t& composer) noexcept -> composer_t& { return composer; }
};

TEST_F(transient_resolver_test_t, resolve_invokes_correct_dispatcher)
{
    ASSERT_EQ(&composer, &sut.template resolve<resolved_t>(composer));
}

// ------------------------------------------------------------------------------------------------------------------ //

struct shared_resolver_test_t : Test
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

    using resolved_t = composer_t;
    using sut_t = shared_t;

    sut_t sut{};
    composer_t composer{.id = composer_t::expected_id};
};

TEST_F(shared_resolver_test_t, expected_result)
{
    ASSERT_EQ(composer_t::expected_id, sut.template resolve<resolved_t>(composer).id);
}

TEST_F(shared_resolver_test_t, result_is_stored_locally)
{
    ASSERT_NE(&composer, &sut.template resolve<resolved_t>(composer));
}

TEST_F(shared_resolver_test_t, result_is_repeatable)
{
    ASSERT_EQ(&sut.template resolve<resolved_t>(composer), &sut.template resolve<resolved_t>(composer));
}

} // namespace
} // namespace dink::resolvers
