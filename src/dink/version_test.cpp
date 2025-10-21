// \file
// Copyright (c) 2025 Frank Secilia
// SPDX-License-Identifier: MIT

#include "version.hpp"
#include <dink/test.hpp>

namespace dink {
namespace {

struct VersionTest : Test {
  using Sut = Version;
  const Sut sut =
      Version{dink_version_major, dink_version_minor, dink_version_patch};
};

TEST_F(VersionTest, version_matches_preprocessor) {
  const auto expected = version();

  const auto actual = sut;

  ASSERT_EQ(expected, actual);
}

TEST_F(VersionTest, version_string_matches_preprocessor) {
  const auto expected = dink_version;

  const auto actual = sut.to_string();

  ASSERT_EQ(expected, actual);
}

}  // namespace
}  // namespace dink
