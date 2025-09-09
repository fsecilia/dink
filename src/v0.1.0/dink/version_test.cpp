/*!
    \file
    Copyright (C) 2025 Frank Secilia
*/

#include <dink/test.hpp>
#include <dink/version.hpp>

namespace dink {
namespace {

TEST(version_test, version_matches_preprocessor)
{
    ASSERT_EQ((groundwork::version_t{dink_version_major, dink_version_minor, dink_version_patch}), version());
}

} // namespace
} // namespace dink
