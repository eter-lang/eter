//===----------------------------------------------------------------------===//
//
// Part of the Eter Project, under the Apache License v2.0 with LLVM Exceptions.
// See /LICENSE for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "eter/Driver/Version.h"

#include "gtest/gtest.h"

using namespace eter;

TEST(VersionTest, VersionString) {
  auto Ver = version();
  EXPECT_FALSE(Ver.empty());
  EXPECT_EQ(Ver, "0.1.0");
}

TEST(VersionTest, VersionFormat) {
  auto Ver = version();
  EXPECT_EQ(Ver, "0.1.0");
}
