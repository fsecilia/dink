/*!
    \file
    Copyright (C) 2025 Frank Secilia
*/

#include <dink/test.hpp>
#include <dink/lifetime_registry.hpp>

namespace dink {
namespace {

struct lifetime_registry_test_t : Test
{
    struct resolved_t
    {};

    using sut_t = lifetime_registry_t;
    sut_t sut;
};

TEST_F(lifetime_registry_test_t, first_ensure_always_succeeds_singleton)
{
    sut.ensure<resolved_t>(lifetime_t::singleton);
}

TEST_F(lifetime_registry_test_t, first_ensure_always_succeeds_transient)
{
    sut.ensure<resolved_t>(lifetime_t::transient);
}

TEST_F(lifetime_registry_test_t, second_ensure_succeeds_if_match_singleton_singleton)
{
    sut.ensure<resolved_t>(lifetime_t::singleton);
    sut.ensure<resolved_t>(lifetime_t::singleton);
}

TEST_F(lifetime_registry_test_t, second_ensure_succeeds_if_match_transient_transient)
{
    sut.ensure<resolved_t>(lifetime_t::transient);
    sut.ensure<resolved_t>(lifetime_t::transient);
}

TEST_F(lifetime_registry_test_t, second_ensure_fails_if_mismatch_singleton_transient)
{
    sut.ensure<resolved_t>(lifetime_t::transient);
    EXPECT_THROW(sut.ensure<resolved_t>(lifetime_t::singleton), lifetime_mismatch_x);
}

TEST_F(lifetime_registry_test_t, second_ensure_fails_if_mismatch_transient_singleton)
{
    sut.ensure<resolved_t>(lifetime_t::singleton);
    EXPECT_THROW(sut.ensure<resolved_t>(lifetime_t::transient), lifetime_mismatch_x);
}

} // namespace
} // namespace dink
