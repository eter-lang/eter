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

static NodeIndex bodyOfFn() {
  // FnDecl children: [ParamList, BlockExpr] or [ParamList, ReturnType,
  // BlockExpr]
  const auto Children = PR.Pool.childrenOf(PR.Pool.childrenOf(PR.Root)[0]);
  return Children[Children.size() - 1];
}
static NodeIndex firstStmtInBody() { return PR.Pool.childrenOf(bodyOfFn())[0]; }

TEST(ParserTestForStmt, SimpleFor) {
  parseSource("fn f() { for x in [1, 2, 3] {} }");

  EXPECT_TRUE(PR.ok());
  const NodeIndex S = firstStmtInBody();
  EXPECT_EQ(PR.Pool.kindOf(S), NodeKind::ForStmt);

  // ForStmt children: [Expr (iterable), BlockExpr]
  EXPECT_EQ(PR.Pool.childrenOf(S).size(), 2u);
  EXPECT_EQ(PR.Pool.kindOf(PR.Pool.childrenOf(S)[0]), NodeKind::ArrayLitExpr);
  EXPECT_EQ(PR.Pool.kindOf(PR.Pool.childrenOf(S)[1]), NodeKind::BlockExpr);

  checkInternedString(S, "x");
  checkRegime(S, Regime::None);
}

TEST(ParserTestForStmt, ForWithBody) {
  parseSource("fn f() { for i in [1, 2] { ret i; } }");

  EXPECT_TRUE(PR.ok());
  const NodeIndex S = firstStmtInBody();
  EXPECT_EQ(PR.Pool.kindOf(S), NodeKind::ForStmt);

  const NodeIndex Body = PR.Pool.childrenOf(S)[1];
  EXPECT_GT(PR.Pool.childrenOf(Body).size(), 0u);
}

TEST(ParserTestForStmt, ForMut) {
  parseSource("fn f() { for mut i in [1] {} }");

  EXPECT_TRUE(PR.ok());
  const NodeIndex S = firstStmtInBody();
  EXPECT_EQ(PR.Pool.kindOf(S), NodeKind::ForStmt);
  checkRegime(S, Regime::Mut);
}

TEST(ParserTestForStmt, ForMissingIn) {
  parseSource("fn f() { for x [1] {} }");

  EXPECT_FALSE(PR.ok());
  expectDiag(DiagID::ExpectedKeyword);
}

TEST(ParserTestForStmt, ForMissingBody) {
  parseSource("fn f() { for x in [1] }");

  EXPECT_FALSE(PR.ok());
  expectDiag(DiagID::ExpectedOpen);
}

TEST(ParserTestForStmt, ForMissingVarName) {
  parseSource("fn f() { for in [1] {} }");

  EXPECT_FALSE(PR.ok());
  expectDiag(DiagID::ExpectedName);
}
