//===----------------------------------------------------------------------===//
//
// Part of the Eter Project, under the Apache License v2.0 with LLVM Exceptions.
// See /LICENSE for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "eter/Parser/NodePool.h"
#include "eter/Parser/Parser.h"

#include "TestParserHelper.h"
#include "gtest/gtest.h"

using namespace eter;
using namespace eter::parser;
using namespace eter::lexer;
using namespace ParserTestHelper;

TEST(ParserTestBreakContinue, BreakInFor) {
  parseSource("fn f() { for x in [1] { break; } }");

  EXPECT_TRUE(PR.ok());
  // FnDecl children: [ParamList, BlockExpr]
  const NodeIndex Body =
      PR.Pool.childrenOf(PR.Pool.childrenOf(PR.Root)[0]).back();
  const NodeIndex For = PR.Pool.childrenOf(Body)[0];
  // ForStmt children: [Expr, BlockExpr]
  const NodeIndex ForBody = PR.Pool.childrenOf(For)[1];
  const NodeIndex S = PR.Pool.childrenOf(ForBody)[0];
  EXPECT_EQ(PR.Pool.kindOf(S), NodeKind::BreakStmt);
}

TEST(ParserTestBreakContinue, ContinueInFor) {
  parseSource("fn f() { for x in [1] { continue; } }");

  EXPECT_TRUE(PR.ok());
  const NodeIndex Body =
      PR.Pool.childrenOf(PR.Pool.childrenOf(PR.Root)[0]).back();
  const NodeIndex For = PR.Pool.childrenOf(Body)[0];
  const NodeIndex ForBody = PR.Pool.childrenOf(For)[1];
  const NodeIndex S = PR.Pool.childrenOf(ForBody)[0];
  EXPECT_EQ(PR.Pool.kindOf(S), NodeKind::ContinueStmt);
}

TEST(ParserTestBreakContinue, BreakMissingSemi) {
  parseSource("fn f() { for x in [1] { break } }");

  EXPECT_FALSE(PR.ok());
  expectDiag(DiagID::ExpectedBreakOrContinueSemi);
}

TEST(ParserTestBreakContinue, ContinueMissingSemi) {
  parseSource("fn f() { for x in [1] { continue } }");

  EXPECT_FALSE(PR.ok());
  expectDiag(DiagID::ExpectedBreakOrContinueSemi);
}
