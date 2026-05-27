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
  const Span S;
  EXPECT_EQ(S.Start, 0u);
  EXPECT_EQ(S.End, 0u);
}

TEST(SpanTest, ParameterizedConstruction) {
  const Span S(10, 20);
  EXPECT_EQ(S.Start, 10u);
  EXPECT_EQ(S.End, 20u);
}

TEST(SpanTest, ZeroLengthSpan) {
  const Span S(5, 5);
  EXPECT_EQ(S.Start, 5u);
  EXPECT_EQ(S.End, 5u);
}

TEST(SpanTest, LargeValues) {
  const Span S(UINT32_MAX - 10, UINT32_MAX);
  EXPECT_EQ(S.Start, UINT32_MAX - 10);
  EXPECT_EQ(S.End, UINT32_MAX);
}
