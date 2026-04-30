//===----------------------------------------------------------------------===//
//
// Part of the Eter Project, under the Apache License v2.0 with LLVM Exceptions.
// See /LICENSE for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "eter/Base/Span.h"

#include "gtest/gtest.h"

using namespace eter;

TEST(SpanTest, DefaultConstruction) {
  Span s;
  EXPECT_EQ(s.Start, 0u);
  EXPECT_EQ(s.End, 0u);
}

TEST(SpanTest, ParameterizedConstruction) {
  Span s(10, 20);
  EXPECT_EQ(s.Start, 10u);
  EXPECT_EQ(s.End, 20u);
}

TEST(SpanTest, ZeroLengthSpan) {
  Span s(5, 5);
  EXPECT_EQ(s.Start, 5u);
  EXPECT_EQ(s.End, 5u);
}

TEST(SpanTest, LargeValues) {
  Span s(UINT32_MAX - 10, UINT32_MAX);
  EXPECT_EQ(s.Start, UINT32_MAX - 10);
  EXPECT_EQ(s.End, UINT32_MAX);
}
