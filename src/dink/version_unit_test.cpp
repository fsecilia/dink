/*!
    \file
    Copyright (C) 2025 Frank Secilia
*/

#include <dink/test.hpp>
#include <dink/version.hpp>

namespace dink {
namespace {

struct version_test_t : Test
{
    static constexpr int_t const major = 3;
    static constexpr int_t const minor = 5;
    static constexpr int_t const patch = 11;
    static constexpr int_t const other = 13;

    static constexpr auto version_string = "3.5.11";

    using sut_t = version_t;

    sut_t sut{major, minor, patch};
};

TEST_F(version_test_t, major)
{
    ASSERT_EQ(major, sut.major());
}

TEST_F(version_test_t, set_major)
{
    sut.major(other);
    ASSERT_EQ(other, sut.major());
}

TEST_F(version_test_t, minor)
{
    ASSERT_EQ(minor, sut.minor());
}

TEST_F(version_test_t, set_minor)
{
    sut.minor(other);
    ASSERT_EQ(other, sut.minor());
}

TEST_F(version_test_t, patch)
{
    ASSERT_EQ(patch, sut.patch());
}

TEST_F(version_test_t, set_patch)
{
    sut.patch(other);
    ASSERT_EQ(other, sut.patch());
}

TEST_F(version_test_t, to_string)
{
    ASSERT_EQ(version_string, sut.to_string());
}

} // namespace
} // namespace dink
