/*!
    \file
    Copyright (C) 2025 Frank Secilia
*/

#include <dink/test.hpp>
#include <dink/composer.hpp>

namespace dink {
namespace {

struct composer_test_t : Test
{
    struct resolved_t
    {
        int_t result;
    };

    struct transient_resolver_t
    {
        static constexpr resolved_t const expected_result{.result = 3};

        template <typename resolved_t, typename composer_t>
        auto resolve(composer_t& composer) const -> resolved_t
        {
            return expected_result;
        }
    };

    struct shared_resolver_t
    {
        inline static resolved_t expected_result{.result = 5};

        template <typename resolved_t, typename composer_t>
        auto resolve(composer_t& composer) const -> resolved_t&
        {
            return expected_result;
        }
    };

    using sut_t = composer_t<transient_resolver_t, shared_resolver_t>;

    sut_t sut{};
};

TEST_F(composer_test_t, resolve_transient)
{
    ASSERT_EQ(transient_resolver_t::expected_result.result, sut.template resolve<resolved_t>().result);
}

TEST_F(composer_test_t, resolve_shared)
{
    ASSERT_EQ(&shared_resolver_t::expected_result, &sut.template resolve<resolved_t&>());
}

} // namespace
} // namespace dink
