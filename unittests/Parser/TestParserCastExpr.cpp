//===----------------------------------------------------------------------===//
//
// Part of the Eter Project, under the Apache License v2.0 with LLVM Exceptions.
// See /LICENSE for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "eter/Parser/NodePool.h"
#include "eter/Parser/Parser.h"
#include "eter/Parser/Regime.h"

#include "TestParserHelper.h"
#include "gtest/gtest.h"

using namespace eter;
using namespace eter::parser;
using namespace eter::lexer;
using namespace ParserTestHelper;

TEST(ParserTestCastExpr, IdentAsType) {
  parseSource("fn f() { let x = 5 as f32; }");

  EXPECT_TRUE(PR.ok());
  const NodeIndex Body =
      PR.Pool.childrenOf(PR.Pool.childrenOf(PR.Root)[0]).back();
  const NodeIndex Let = PR.Pool.childrenOf(Body)[0];
  const auto Children = PR.Pool.childrenOf(Let);
  const NodeIndex Init = Children.back();
  EXPECT_EQ(PR.Pool.kindOf(Init), NodeKind::CastExpr);
}

TEST(ParserTestCastExpr, CastChildren) {
  parseSource("fn f() { let y = x as f32; }");

  EXPECT_TRUE(PR.ok());
  const NodeIndex Body =
      PR.Pool.childrenOf(PR.Pool.childrenOf(PR.Root)[0]).back();
  const NodeIndex Let = PR.Pool.childrenOf(Body)[0];
  const auto Children = PR.Pool.childrenOf(Let);
  const NodeIndex Cast = Children.back();
  const auto CastChildren = PR.Pool.childrenOf(Cast);
  ASSERT_EQ(CastChildren.size(), 2u);
  EXPECT_EQ(PR.Pool.kindOf(CastChildren[0]), NodeKind::IdentExpr);
  EXPECT_EQ(PR.Pool.kindOf(CastChildren[1]), NodeKind::NamedType);
}

TEST(ParserTestCastExpr, CastLiteral) {
  parseSource("fn f() { let x = 42 as f64; }");

  EXPECT_TRUE(PR.ok());
  const NodeIndex Body =
      PR.Pool.childrenOf(PR.Pool.childrenOf(PR.Root)[0]).back();
  const NodeIndex Let = PR.Pool.childrenOf(Body)[0];
  const auto Children = PR.Pool.childrenOf(Let);
  const NodeIndex Cast = Children.back();
  EXPECT_EQ(PR.Pool.kindOf(Cast), NodeKind::CastExpr);
  const auto CastChildren = PR.Pool.childrenOf(Cast);
  EXPECT_EQ(PR.Pool.kindOf(CastChildren[0]), NodeKind::LitExpr);
  EXPECT_EQ(PR.Pool.kindOf(CastChildren[1]), NodeKind::NamedType);
}

TEST(ParserTestCastExpr, CastParenthesized) {
  parseSource("fn f() { let x = (a + b) as u32; }");

  EXPECT_TRUE(PR.ok());
  const NodeIndex Body =
      PR.Pool.childrenOf(PR.Pool.childrenOf(PR.Root)[0]).back();
  const NodeIndex Let = PR.Pool.childrenOf(Body)[0];
  const auto Children = PR.Pool.childrenOf(Let);
  const NodeIndex Cast = Children.back();
  EXPECT_EQ(PR.Pool.kindOf(Cast), NodeKind::CastExpr);
  const auto CastChildren = PR.Pool.childrenOf(Cast);
  EXPECT_EQ(PR.Pool.kindOf(CastChildren[0]), NodeKind::BinaryExpr);
  EXPECT_EQ(PR.Pool.kindOf(CastChildren[1]), NodeKind::NamedType);
}

TEST(ParserTestCastExpr, ChainedCasts) {
  parseSource("fn f() { let x = 5 as i32 as f64; }");

  EXPECT_TRUE(PR.ok());
  const NodeIndex Body =
      PR.Pool.childrenOf(PR.Pool.childrenOf(PR.Root)[0]).back();
  const NodeIndex Let = PR.Pool.childrenOf(Body)[0];
  const auto Children = PR.Pool.childrenOf(Let);
  const NodeIndex Cast = Children.back();
  EXPECT_EQ(PR.Pool.kindOf(Cast), NodeKind::CastExpr);
  const auto CastChildren = PR.Pool.childrenOf(Cast);
  EXPECT_EQ(PR.Pool.kindOf(CastChildren[0]), NodeKind::CastExpr);
  EXPECT_EQ(PR.Pool.kindOf(CastChildren[1]), NodeKind::NamedType);
}

TEST(ParserTestCastExpr, CastToArrayType) {
  parseSource("fn f() { let x = ptr as [u8; 16]; }");

  EXPECT_TRUE(PR.ok());
  const NodeIndex Body =
      PR.Pool.childrenOf(PR.Pool.childrenOf(PR.Root)[0]).back();
  const NodeIndex Let = PR.Pool.childrenOf(Body)[0];
  const auto Children = PR.Pool.childrenOf(Let);
  const NodeIndex Cast = Children.back();
  EXPECT_EQ(PR.Pool.kindOf(Cast), NodeKind::CastExpr);
  const auto CastChildren = PR.Pool.childrenOf(Cast);
  EXPECT_EQ(PR.Pool.kindOf(CastChildren[1]), NodeKind::ArrayType);
}

TEST(ParserTestCastExpr, CastInExprStmt) {
  parseSource("fn f() { a as f32; }");

  EXPECT_TRUE(PR.ok());
  const NodeIndex Body =
      PR.Pool.childrenOf(PR.Pool.childrenOf(PR.Root)[0]).back();
  const NodeIndex S = PR.Pool.childrenOf(Body)[0];
  EXPECT_EQ(PR.Pool.kindOf(S), NodeKind::CastExpr);
}

TEST(ParserTestCastExpr, MissingTypeAfterAs) {
  parseSource("fn f() { a as; }");

  EXPECT_FALSE(PR.ok());
  expectDiag(DiagID::ExpectedTypeName);
}
