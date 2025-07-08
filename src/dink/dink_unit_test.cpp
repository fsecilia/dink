/*!
    \file
    Copyright (C) 2025 Frank Secilia
*/

#include <dink/test.hpp>
#include <dink/dink.hpp>

namespace dink {
namespace {

TEST(dink_test_t, version)
{
    ASSERT_EQ((version_t{dink_version_major, dink_version_minor, dink_version_patch}), dink_t{}.version());
}

} // namespace
} // namespace dink
