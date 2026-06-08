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

#include <llvm/Support/VirtualFileSystem.h>

#include "TestParserHelper.h"
#include "gtest/gtest.h"

using namespace eter;
using namespace eter::parser;
using namespace eter::lexer;
using namespace ParserTestHelper;

using EToken = eter::lexer::Token;

static NodeIndex fnBody() {
  return PR.Pool.childrenOf(PR.Pool.childrenOf(PR.Root)[0])[1];
}
static NodeIndex firstStmt() { return PR.Pool.childrenOf(fnBody())[0]; }

//===----------------------------------------------------------------------===//
// Return statements
//===----------------------------------------------------------------------===//

TEST(ParserTestStmt, RetStmtNoExpr) {
  parseSource("fn main() { ret; }");

  EXPECT_FALSE(PR.ok());
  EXPECT_TRUE(hasDiag(DiagID::ExpectedExpr));
}

TEST(ParserTestStmt, RetStmtWithLiteral) {
  parseSource("fn f(): i32 { ret 42; }");

  EXPECT_TRUE(PR.ok());

  const NodeIndex Fn = PR.Pool.childrenOf(PR.Root)[0];
  // FnDecl children: [ParamList, ReturnType, BlockExpr]
  const NodeIndex Body = PR.Pool.childrenOf(Fn)[2];
  EXPECT_TRUE(checkChildrenKinds(Body, NodeKind::RetStmt));

  const NodeIndex Ret = PR.Pool.childrenOf(Body)[0];
  EXPECT_TRUE(checkChildrenKinds(Ret, NodeKind::LitExpr));
  EXPECT_EQ(PR.Pool.childrenOf(Ret).size(), 1u);
  checkInternedString(PR.Pool.childrenOf(Ret)[0], "42");
}

//===----------------------------------------------------------------------===//
// If expressions
//===----------------------------------------------------------------------===//

TEST(ParserTestStmt, IfExpr) {
  parseSource("fn main() { if true { } }");

  EXPECT_TRUE(PR.ok());

  const NodeIndex If = firstStmt();
  EXPECT_EQ(PR.Pool.kindOf(If), NodeKind::IfExpr);
  checkSpan(If, "if true { }");
  // IfExpr children: [cond, then_block]
  EXPECT_TRUE(checkChildrenKinds(If, NodeKind::LitExpr, NodeKind::BlockExpr));
  EXPECT_EQ(PR.Pool.childrenOf(If).size(), 2u);
  checkInternedString(PR.Pool.childrenOf(If)[0], "true");

  const NodeIndex Then = PR.Pool.childrenOf(If)[1];
  EXPECT_EQ(PR.Pool.childrenOf(Then).size(), 0u); // empty block
}

TEST(ParserTestStmt, IfElseExpr) {
  parseSource("fn main() { if true { 1; } else { 2; } }");

  EXPECT_TRUE(PR.ok());

  const NodeIndex If = firstStmt();
  checkSpan(If, "if true { 1; } else { 2; }");
  // IfExpr children: [cond, then_block, else_block]
  EXPECT_TRUE(checkChildrenKinds(If, NodeKind::LitExpr, NodeKind::BlockExpr,
                                 NodeKind::BlockExpr));
  EXPECT_EQ(PR.Pool.childrenOf(If).size(), 3u);

  const NodeIndex Then = PR.Pool.childrenOf(If)[1];
  EXPECT_TRUE(checkChildrenKinds(Then, NodeKind::LitExpr));
  checkInternedString(PR.Pool.childrenOf(Then)[0], "1");

  const NodeIndex Else = PR.Pool.childrenOf(If)[2];
  EXPECT_TRUE(checkChildrenKinds(Else, NodeKind::LitExpr));
  checkInternedString(PR.Pool.childrenOf(Else)[0], "2");
}

//===----------------------------------------------------------------------===//
// While loops
//===----------------------------------------------------------------------===//

TEST(ParserTestStmt, WhileStmt) {
  parseSource("fn main() { while true { } }");

  EXPECT_TRUE(PR.ok());

  const NodeIndex While = firstStmt();
  EXPECT_EQ(PR.Pool.kindOf(While), NodeKind::WhileStmt);
  EXPECT_TRUE(
      checkChildrenKinds(While, NodeKind::LitExpr, NodeKind::BlockExpr));
  EXPECT_EQ(PR.Pool.childrenOf(While).size(), 2u);
  checkInternedString(PR.Pool.childrenOf(While)[0], "true");

  const NodeIndex Body = PR.Pool.childrenOf(While)[1];
  EXPECT_EQ(PR.Pool.childrenOf(Body).size(), 0u);
}

//===----------------------------------------------------------------------===//
// Match expressions
//===----------------------------------------------------------------------===//

TEST(ParserTestStmt, MatchExprWildcard) {
  parseSource("fn main() { let imm x: i32 = 5; match x { _ => 42 } }");

  EXPECT_TRUE(PR.ok());

  const NodeIndex Match = PR.Pool.childrenOf(fnBody())[1];
  EXPECT_EQ(PR.Pool.kindOf(Match), NodeKind::MatchExpr);
  // MatchExpr children: [scrutinee, MatchArm*]
  EXPECT_TRUE(
      checkChildrenKinds(Match, NodeKind::IdentExpr, NodeKind::MatchArm));
  EXPECT_EQ(PR.Pool.childrenOf(Match).size(), 2u);
  checkInternedString(PR.Pool.childrenOf(Match)[0], "x");

  const NodeIndex Arm = PR.Pool.childrenOf(Match)[1];
  EXPECT_TRUE(
      checkChildrenKinds(Arm, NodeKind::WildcardPat, NodeKind::LitExpr));
  checkInternedString(PR.Pool.childrenOf(Arm)[1], "42");
}

TEST(ParserTestStmt, MatchExprLiteralPat) {
  parseSource("fn main() {let imm x: i32 = 5; match x { 42 => true } }");

  EXPECT_TRUE(PR.ok());

  const NodeIndex Match = PR.Pool.childrenOf(fnBody())[1];
  EXPECT_EQ(PR.Pool.kindOf(Match), NodeKind::MatchExpr);
  EXPECT_TRUE(
      checkChildrenKinds(Match, NodeKind::IdentExpr, NodeKind::MatchArm));
  EXPECT_EQ(PR.Pool.childrenOf(Match).size(), 2u);

  const NodeIndex Arm = PR.Pool.childrenOf(Match)[1];
  EXPECT_TRUE(checkChildrenKinds(Arm, NodeKind::LiteralPat, NodeKind::LitExpr));

  // Arm has [Pat, Expr]. Pat is LiteralPat (leaf, no children).
  // The first child of arm is LiteralPat, which is a leaf with payload =
  // InternedStr("42")
  const NodeIndex Pat = PR.Pool.childrenOf(Arm)[0];
  EXPECT_EQ(PR.Pool.kindOf(Pat), NodeKind::LiteralPat);
  EXPECT_EQ(PR.Pool.childrenOf(Pat).size(), 0u);
  checkInternedString(Pat, "42");

  // The second child of arm is LitExpr("true")
  const NodeIndex Expr = PR.Pool.childrenOf(Arm)[1];
  EXPECT_EQ(PR.Pool.kindOf(Expr), NodeKind::LitExpr);
  checkInternedString(Expr, "true");
}

TEST(ParserTestStmt, MatchExprIdentPat) {
  parseSource("fn main() {let imm x: i32 = 5; let imm y: i32 = 5; match x { y "
              "=> y; } }");

  EXPECT_TRUE(PR.ok());

  const NodeIndex Match = PR.Pool.childrenOf(fnBody())[2];
  EXPECT_EQ(PR.Pool.kindOf(Match), NodeKind::MatchExpr);
  EXPECT_TRUE(
      checkChildrenKinds(Match, NodeKind::IdentExpr, NodeKind::MatchArm));

  const NodeIndex Arm = PR.Pool.childrenOf(Match)[1];
  EXPECT_TRUE(checkChildrenKinds(Arm, NodeKind::IdentPat, NodeKind::IdentExpr));

  // IdentPat: leaf with payload makePayload("y", Regime::None)
  const NodeIndex Pat = PR.Pool.childrenOf(Arm)[0];
  EXPECT_EQ(PR.Pool.kindOf(Pat), NodeKind::IdentPat);
  EXPECT_EQ(PR.Pool.childrenOf(Pat).size(), 0u);
  checkInternedString(Pat, "y");
  checkRegime(Pat, Regime::None);

  // Expr: IdentExpr("y")
  const NodeIndex Expr = PR.Pool.childrenOf(Arm)[1];
  EXPECT_EQ(PR.Pool.kindOf(Expr), NodeKind::IdentExpr);
  checkInternedString(Expr, "y");
}

TEST(ParserTestStmt, MatchExprMultipleArms) {
  parseSource(
      "fn main() { let imm x: int = 5; match x { 1 => true, 2 => false } }");

  EXPECT_TRUE(PR.ok());

  const NodeIndex Match = PR.Pool.childrenOf(fnBody())[1];
  EXPECT_EQ(PR.Pool.kindOf(Match), NodeKind::MatchExpr);
  EXPECT_TRUE(checkChildrenKinds(Match, NodeKind::IdentExpr, NodeKind::MatchArm,
                                 NodeKind::MatchArm));
  EXPECT_EQ(PR.Pool.childrenOf(Match).size(), 3u);
  checkInternedString(PR.Pool.childrenOf(Match)[0], "x");

  // First arm: 1 => true
  const NodeIndex Arm0 = PR.Pool.childrenOf(Match)[1];
  EXPECT_TRUE(
      checkChildrenKinds(Arm0, NodeKind::LiteralPat, NodeKind::LitExpr));
  // Arm children: [Pat=LiteralPat(leaf), Expr=LitExpr("true")]
  EXPECT_EQ(PR.Pool.kindOf(PR.Pool.childrenOf(Arm0)[0]), NodeKind::LiteralPat);
  checkInternedString(PR.Pool.childrenOf(Arm0)[0], "1");
  checkInternedString(PR.Pool.childrenOf(Arm0)[1], "true");

  // Second arm: 2 => false
  const NodeIndex Arm1 = PR.Pool.childrenOf(Match)[2];
  EXPECT_TRUE(
      checkChildrenKinds(Arm1, NodeKind::LiteralPat, NodeKind::LitExpr));
  checkInternedString(PR.Pool.childrenOf(Arm1)[0], "2");
  checkInternedString(PR.Pool.childrenOf(Arm1)[1], "false");
}

//===----------------------------------------------------------------------===//
// Nested blocks
//===----------------------------------------------------------------------===//

TEST(ParserTestStmt, NestedBlock) {
  parseSource("fn main() { { ret 1; } }");

  EXPECT_TRUE(PR.ok());

  const NodeIndex Body = fnBody();
  EXPECT_TRUE(checkChildrenKinds(Body, NodeKind::BlockExpr));

  const NodeIndex InnerBlock = PR.Pool.childrenOf(Body)[0];
  EXPECT_TRUE(checkChildrenKinds(InnerBlock, NodeKind::RetStmt));

  const NodeIndex Ret = PR.Pool.childrenOf(InnerBlock)[0];
  EXPECT_EQ(PR.Pool.kindOf(Ret), NodeKind::RetStmt);
  EXPECT_EQ(PR.Pool.childrenOf(Ret).size(), 1u);
}

//===----------------------------------------------------------------------===//
// Error recovery
//===----------------------------------------------------------------------===//

TEST(ParserTestStmt, ErrorExpectedStmt) {
  parseSource("fn main() { @ }");

  EXPECT_FALSE(PR.ok());
  EXPECT_TRUE(hasDiag(DiagID::ExpectedStmt));
}

TEST(ParserTestStmt, ErrorExpectedExpr) {
  parseSource("fn main() { let imm x: i32 = @; }");

  EXPECT_FALSE(PR.ok());
  EXPECT_TRUE(hasDiag(DiagID::ExpectedExpr));
}

TEST(ParserTestStmt, ErrorMissingExprInLetNoCascade) {
  // Regression test: missing expression in a let initializer should emit
  // exactly one `ExpectedExpr` diagnostic, and the surrounding block should
  // close cleanly (no spurious `ExpectedBlockClose` / `ExpectedStmt`).
  parseSource("fn main() { let imm x: i32 = ; }");

  EXPECT_FALSE(PR.ok());
  EXPECT_TRUE(hasDiag(DiagID::ExpectedExpr));
  EXPECT_FALSE(hasDiag(DiagID::ExpectedStmt));
  EXPECT_FALSE(hasDiag(DiagID::ExpectedBlockClose));
}

TEST(ParserTestStmt, ErrorRecoveryThroughStatementKeyword) {
  // Regression test: after a parse error inside a block, `synchronize()` must
  // stop at the next statement-introducing keyword (here `let`) rather than
  // greedily consuming the whole statement. The `let` that follows the error
  // should be parsed as a normal let-statement.
  parseSource("fn main() { @ let imm x: i32 = 5; }");

  EXPECT_FALSE(PR.ok());
  EXPECT_TRUE(hasDiag(DiagID::ExpectedStmt));
  // The let-statement should still have been built (the error node is its
  // sibling, not a replacement).
  const NodeIndex Body = fnBody();
  const auto &BodyChildren = PR.Pool.childrenOf(Body);
  ASSERT_EQ(BodyChildren.size(), 2u);
  EXPECT_EQ(PR.Pool.kindOf(BodyChildren[0]), NodeKind::Error);
  EXPECT_EQ(PR.Pool.kindOf(BodyChildren[1]), NodeKind::LetStmt);
}
