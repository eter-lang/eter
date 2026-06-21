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

TEST(ParserTestProjAssign, SimpleProjAssign) {
  parseSource("fn f() { *x = 42; }");

  EXPECT_TRUE(PR.ok());
  const NodeIndex Body =
      PR.Pool.childrenOf(PR.Pool.childrenOf(PR.Root)[0]).back();
  const NodeIndex S = PR.Pool.childrenOf(Body)[0];
  EXPECT_EQ(PR.Pool.kindOf(S), NodeKind::ProjAssignExpr);
}

TEST(ParserTestProjAssign, ProjAssignChildren) {
  parseSource("fn f() { *x = 42; }");

  EXPECT_TRUE(PR.ok());
  const NodeIndex Body =
      PR.Pool.childrenOf(PR.Pool.childrenOf(PR.Root)[0]).back();
  const NodeIndex Proj = PR.Pool.childrenOf(Body)[0];
  const auto Children = PR.Pool.childrenOf(Proj);
  ASSERT_EQ(Children.size(), 2u);
  EXPECT_EQ(PR.Pool.kindOf(Children[0]), NodeKind::IdentExpr);
  EXPECT_EQ(PR.Pool.kindOf(Children[1]), NodeKind::LitExpr);
}

TEST(ParserTestProjAssign, ProjAssignField) {
  parseSource("fn f() { *obj.field = val; }");

  EXPECT_TRUE(PR.ok());
  const NodeIndex Body =
      PR.Pool.childrenOf(PR.Pool.childrenOf(PR.Root)[0]).back();
  const NodeIndex Proj = PR.Pool.childrenOf(Body)[0];
  EXPECT_EQ(PR.Pool.kindOf(Proj), NodeKind::ProjAssignExpr);
  const auto Children = PR.Pool.childrenOf(Proj);
  EXPECT_EQ(PR.Pool.kindOf(Children[0]), NodeKind::FieldExpr);
  EXPECT_EQ(PR.Pool.kindOf(Children[1]), NodeKind::IdentExpr);
}

TEST(ParserTestProjAssign, ProjAssignIndex) {
  parseSource("fn f() { *arr[i] = 42; }");

  EXPECT_TRUE(PR.ok());
  const NodeIndex Body =
      PR.Pool.childrenOf(PR.Pool.childrenOf(PR.Root)[0]).back();
  const NodeIndex Proj = PR.Pool.childrenOf(Body)[0];
  EXPECT_EQ(PR.Pool.kindOf(Proj), NodeKind::ProjAssignExpr);
  const auto Children = PR.Pool.childrenOf(Proj);
  EXPECT_EQ(PR.Pool.kindOf(Children[0]), NodeKind::IndexExpr);
}

TEST(ParserTestProjAssign, ProjAssignInLetInit) {
  parseSource("fn f() { let x = *y = 42; }");

  EXPECT_TRUE(PR.ok());
  const NodeIndex Body =
      PR.Pool.childrenOf(PR.Pool.childrenOf(PR.Root)[0]).back();
  const NodeIndex Let = PR.Pool.childrenOf(Body)[0];
  const auto LetChildren = PR.Pool.childrenOf(Let);
  const NodeIndex Init = LetChildren.back();
  EXPECT_EQ(PR.Pool.kindOf(Init), NodeKind::ProjAssignExpr);
}

TEST(ParserTestProjAssign, StarNotProjAssignWithoutEq) {
  parseSource("fn f() { *x; }");

  EXPECT_TRUE(PR.ok());
  const NodeIndex Body =
      PR.Pool.childrenOf(PR.Pool.childrenOf(PR.Root)[0]).back();
  const NodeIndex S = PR.Pool.childrenOf(Body)[0];
  // Without `=`, `*x` is a UnaryExpr (dereference).
  EXPECT_EQ(PR.Pool.kindOf(S), NodeKind::UnaryExpr);
}

TEST(ParserTestProjAssign, AmpIsUnaryExpr) {
  parseSource("fn f() { &x; }");

  EXPECT_TRUE(PR.ok());
  const NodeIndex Body =
      PR.Pool.childrenOf(PR.Pool.childrenOf(PR.Root)[0]).back();
  const NodeIndex S = PR.Pool.childrenOf(Body)[0];
  EXPECT_EQ(PR.Pool.kindOf(S), NodeKind::UnaryExpr);
}
