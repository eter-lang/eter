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

// Helpers to reduce boilerplate in expression tests.
static NodeIndex fnBody() {
  return PR.Pool.childrenOf(PR.Pool.childrenOf(PR.Root)[0])[1];
}
static NodeIndex firstLet() { return PR.Pool.childrenOf(fnBody())[0]; }

TEST(ParserTestExpr, LetBindingSimpleLiteralExpr) {
  parseSource("fn main(){ let imm x: i32 = 10; }");

  EXPECT_TRUE(PR.ok());
  EXPECT_TRUE(checkChildrenKinds(PR.Root, NodeKind::FnDecl));
  const NodeIndex Fn = PR.Pool.childrenOf(PR.Root)[0];
  EXPECT_TRUE(checkChildrenKinds(Fn, NodeKind::ParamList, NodeKind::BlockExpr));
  EXPECT_TRUE(checkChildrenKinds(PR.Pool.childrenOf(Fn)[0]));

  const NodeIndex Body = PR.Pool.childrenOf(Fn)[1];
  EXPECT_TRUE(checkChildrenKinds(Body, NodeKind::LetStmt));

  const NodeIndex Let = PR.Pool.childrenOf(Body)[0];
  EXPECT_TRUE(checkChildrenKinds(Let, NodeKind::NamedType, NodeKind::LitExpr));
  checkInternedString(Let, "x");
  checkRegime(Let, Regime::Imm);
  checkInternedString(PR.Pool.childrenOf(Let)[0], "i32");
  checkInternedString(PR.Pool.childrenOf(Let)[1], "10");
}

TEST(ParserTestExpr, LetMutWithType) {
  parseSource("fn main(){ let mut x: i32 = 10; }");

  EXPECT_TRUE(PR.ok());
  EXPECT_TRUE(checkChildrenKinds(PR.Root, NodeKind::FnDecl));

  const NodeIndex Let = firstLet();
  EXPECT_TRUE(checkChildrenKinds(Let, NodeKind::NamedType, NodeKind::LitExpr));
  checkInternedString(Let, "x");
  checkRegime(Let, Regime::Mut);
  checkInternedString(PR.Pool.childrenOf(Let)[0], "i32");
  checkInternedString(PR.Pool.childrenOf(Let)[1], "10");
}

TEST(ParserTestExpr, LetProjWithType) {
  parseSource("fn main(){ let proj x: i32 = 10; }");

  EXPECT_TRUE(PR.ok());

  const NodeIndex Let = firstLet();
  EXPECT_TRUE(checkChildrenKinds(Let, NodeKind::NamedType, NodeKind::LitExpr));
  checkInternedString(Let, "x");
  checkRegime(Let, Regime::Proj);
  checkInternedString(PR.Pool.childrenOf(Let)[0], "i32");
  checkInternedString(PR.Pool.childrenOf(Let)[1], "10");
}

TEST(ParserTestExpr, LetWithBoolLit) {
  parseSource("fn main(){ let imm x: bool = true; }");

  EXPECT_TRUE(PR.ok());

  const NodeIndex Let = firstLet();
  EXPECT_TRUE(checkChildrenKinds(Let, NodeKind::NamedType, NodeKind::LitExpr));
  checkInternedString(PR.Pool.childrenOf(Let)[0], "bool");
  checkInternedString(PR.Pool.childrenOf(Let)[1], "true");
}

TEST(ParserTestExpr, LetWithStringLit) {
  parseSource(R"(fn main(){ let imm x: str = "hello"; })");

  EXPECT_TRUE(PR.ok());

  const NodeIndex Let = firstLet();
  EXPECT_TRUE(checkChildrenKinds(Let, NodeKind::NamedType, NodeKind::LitExpr));
  checkInternedString(PR.Pool.childrenOf(Let)[0], "str");
  checkInternedString(PR.Pool.childrenOf(Let)[1], "\"hello\"");
}

TEST(ParserTestExpr, LetWithFloatLit) {
  parseSource("fn main(){ let mut y: f32 = 3.14; }");

  EXPECT_TRUE(PR.ok());

  const NodeIndex Let = firstLet();
  EXPECT_TRUE(checkChildrenKinds(Let, NodeKind::NamedType, NodeKind::LitExpr));
  checkInternedString(Let, "y");
  checkRegime(Let, Regime::Mut);
  checkInternedString(PR.Pool.childrenOf(Let)[0], "f32");
  checkInternedString(PR.Pool.childrenOf(Let)[1], "3.14");
}

TEST(ParserTestExpr, LetWithCharLit) {
  parseSource("fn main(){ let imm c: char = 'a'; }");

  EXPECT_TRUE(PR.ok());

  const NodeIndex Let = firstLet();
  EXPECT_TRUE(checkChildrenKinds(Let, NodeKind::NamedType, NodeKind::LitExpr));
  checkInternedString(PR.Pool.childrenOf(Let)[0], "char");
  checkInternedString(PR.Pool.childrenOf(Let)[1], "'a'");
}

TEST(ParserTestExpr, LetWithSimpleBinary) {
  parseSource("fn main(){ let imm x: i32 = 1 + 2; }");

  EXPECT_TRUE(PR.ok());

  const NodeIndex Let = firstLet();
  EXPECT_TRUE(
      checkChildrenKinds(Let, NodeKind::NamedType, NodeKind::BinaryExpr));
  checkInternedString(Let, "x");
  checkRegime(Let, Regime::Imm);

  const NodeIndex Rhs = PR.Pool.childrenOf(Let)[1];
  EXPECT_EQ(NodePool::payloadOp(PR.Pool[Rhs].Payload),
            static_cast<uint16_t>(EToken::Kind::plus));
  EXPECT_TRUE(checkChildrenKinds(Rhs, NodeKind::LitExpr, NodeKind::LitExpr));
  checkInternedString(PR.Pool.childrenOf(Rhs)[0], "1");
  checkInternedString(PR.Pool.childrenOf(Rhs)[1], "2");
}

TEST(ParserTestExpr, LetWithBinaryPrecedence) {
  parseSource("fn main(){ let imm x: i32 = 1 + 2 * 3; }");

  EXPECT_TRUE(PR.ok());

  const NodeIndex Let = firstLet();
  EXPECT_TRUE(
      checkChildrenKinds(Let, NodeKind::NamedType, NodeKind::BinaryExpr));
  checkInternedString(Let, "x");

  // 1 + 2 * 3  =>  1 + (2 * 3)
  const NodeIndex Outer = PR.Pool.childrenOf(Let)[1];
  EXPECT_EQ(NodePool::payloadOp(PR.Pool[Outer].Payload),
            static_cast<uint16_t>(EToken::Kind::plus));
  EXPECT_TRUE(
      checkChildrenKinds(Outer, NodeKind::LitExpr, NodeKind::BinaryExpr));
  checkInternedString(PR.Pool.childrenOf(Outer)[0], "1");

  const NodeIndex Inner = PR.Pool.childrenOf(Outer)[1];
  EXPECT_EQ(NodePool::payloadOp(PR.Pool[Inner].Payload),
            static_cast<uint16_t>(EToken::Kind::star));
  EXPECT_TRUE(checkChildrenKinds(Inner, NodeKind::LitExpr, NodeKind::LitExpr));
  checkInternedString(PR.Pool.childrenOf(Inner)[0], "2");
  checkInternedString(PR.Pool.childrenOf(Inner)[1], "3");
}

TEST(ParserTestExpr, LetWithParenOverride) {
  parseSource("fn main(){ let imm x: i32 = (1 + 2) * 3; }");

  EXPECT_TRUE(PR.ok());

  const NodeIndex Let = firstLet();
  EXPECT_TRUE(
      checkChildrenKinds(Let, NodeKind::NamedType, NodeKind::BinaryExpr));
  checkInternedString(Let, "x");

  // (1 + 2) * 3  =>  (1+2) * 3, parens elided, outer is *
  const NodeIndex Outer = PR.Pool.childrenOf(Let)[1];
  EXPECT_EQ(NodePool::payloadOp(PR.Pool[Outer].Payload),
            static_cast<uint16_t>(EToken::Kind::star));
  EXPECT_TRUE(
      checkChildrenKinds(Outer, NodeKind::BinaryExpr, NodeKind::LitExpr));
  checkInternedString(PR.Pool.childrenOf(Outer)[1], "3");

  // Inner is the (1+2) sub-tree
  const NodeIndex Inner = PR.Pool.childrenOf(Outer)[0];
  EXPECT_EQ(NodePool::payloadOp(PR.Pool[Inner].Payload),
            static_cast<uint16_t>(EToken::Kind::plus));
  EXPECT_TRUE(checkChildrenKinds(Inner, NodeKind::LitExpr, NodeKind::LitExpr));
  checkInternedString(PR.Pool.childrenOf(Inner)[0], "1");
  checkInternedString(PR.Pool.childrenOf(Inner)[1], "2");
}

TEST(ParserTestExpr, LetWithLeftAssoc) {
  parseSource("fn main(){ let imm x: i32 = 1 + 2 - 3; }");

  EXPECT_TRUE(PR.ok());

  const NodeIndex Let = firstLet();
  EXPECT_TRUE(
      checkChildrenKinds(Let, NodeKind::NamedType, NodeKind::BinaryExpr));

  // 1 + 2 - 3  =>  (1 + 2) - 3  (left-associative)
  const NodeIndex Outer = PR.Pool.childrenOf(Let)[1];
  EXPECT_EQ(NodePool::payloadOp(PR.Pool[Outer].Payload),
            static_cast<uint16_t>(EToken::Kind::minus));
  EXPECT_TRUE(
      checkChildrenKinds(Outer, NodeKind::BinaryExpr, NodeKind::LitExpr));
  checkInternedString(PR.Pool.childrenOf(Outer)[1], "3");

  const NodeIndex Inner = PR.Pool.childrenOf(Outer)[0];
  EXPECT_EQ(NodePool::payloadOp(PR.Pool[Inner].Payload),
            static_cast<uint16_t>(EToken::Kind::plus));
  EXPECT_TRUE(checkChildrenKinds(Inner, NodeKind::LitExpr, NodeKind::LitExpr));
  checkInternedString(PR.Pool.childrenOf(Inner)[0], "1");
  checkInternedString(PR.Pool.childrenOf(Inner)[1], "2");
}

TEST(ParserTestExpr, LetWithUnaryMinus) {
  parseSource("fn main(){ let imm x: i32 = -5; }");

  EXPECT_TRUE(PR.ok());

  const NodeIndex Let = firstLet();
  EXPECT_TRUE(
      checkChildrenKinds(Let, NodeKind::NamedType, NodeKind::UnaryExpr));
  checkInternedString(Let, "x");

  const NodeIndex Rhs = PR.Pool.childrenOf(Let)[1];
  EXPECT_EQ(NodePool::payloadOp(PR.Pool[Rhs].Payload),
            static_cast<uint16_t>(EToken::Kind::minus));
  EXPECT_TRUE(checkChildrenKinds(Rhs, NodeKind::LitExpr));
  checkInternedString(PR.Pool.childrenOf(Rhs)[0], "5");
}

TEST(ParserTestExpr, LetWithUnaryNot) {
  parseSource("fn main(){ let imm x: bool = !true; }");

  EXPECT_TRUE(PR.ok());

  const NodeIndex Let = firstLet();
  EXPECT_TRUE(
      checkChildrenKinds(Let, NodeKind::NamedType, NodeKind::UnaryExpr));

  const NodeIndex Rhs = PR.Pool.childrenOf(Let)[1];
  EXPECT_EQ(NodePool::payloadOp(PR.Pool[Rhs].Payload),
            static_cast<uint16_t>(EToken::Kind::bang));
  EXPECT_TRUE(checkChildrenKinds(Rhs, NodeKind::LitExpr));
  checkInternedString(PR.Pool.childrenOf(Rhs)[0], "true");
}

TEST(ParserTestExpr, LetWithIdentRhs) {
  parseSource("fn main(){ let imm x: i32 = y; }");

  EXPECT_TRUE(PR.ok());

  const NodeIndex Let = firstLet();
  EXPECT_TRUE(
      checkChildrenKinds(Let, NodeKind::NamedType, NodeKind::IdentExpr));
  checkInternedString(Let, "x");
  checkInternedString(PR.Pool.childrenOf(Let)[1], "y");
}

TEST(ParserTestExpr, MultipleLetBindings) {
  parseSource("fn main(){ let imm x: i32 = 10; let mut y: bool = false; }");

  EXPECT_TRUE(PR.ok());

  const NodeIndex Body = fnBody();
  EXPECT_TRUE(checkChildrenKinds(Body, NodeKind::LetStmt, NodeKind::LetStmt));

  const NodeIndex Let1 = PR.Pool.childrenOf(Body)[0];
  EXPECT_TRUE(checkChildrenKinds(Let1, NodeKind::NamedType, NodeKind::LitExpr));
  checkInternedString(Let1, "x");
  checkRegime(Let1, Regime::Imm);
  checkInternedString(PR.Pool.childrenOf(Let1)[1], "10");

  const NodeIndex Let2 = PR.Pool.childrenOf(Body)[1];
  EXPECT_TRUE(checkChildrenKinds(Let2, NodeKind::NamedType, NodeKind::LitExpr));
  checkInternedString(Let2, "y");
  checkRegime(Let2, Regime::Mut);
  checkInternedString(PR.Pool.childrenOf(Let2)[1], "false");
}

TEST(ParserTestExpr, NestedBlockExpr) {
  parseSource("fn main(){ { let imm x: i32 = 10; } }");

  EXPECT_TRUE(PR.ok());

  const NodeIndex Body = fnBody();
  // Outer block has one child: the inner block
  EXPECT_TRUE(checkChildrenKinds(Body, NodeKind::BlockExpr));

  const NodeIndex InnerBlock = PR.Pool.childrenOf(Body)[0];
  EXPECT_TRUE(checkChildrenKinds(InnerBlock, NodeKind::LetStmt));

  const NodeIndex Let = PR.Pool.childrenOf(InnerBlock)[0];
  EXPECT_TRUE(checkChildrenKinds(Let, NodeKind::NamedType, NodeKind::LitExpr));
  checkInternedString(Let, "x");
  checkRegime(Let, Regime::Imm);
  checkInternedString(PR.Pool.childrenOf(Let)[0], "i32");
  checkInternedString(PR.Pool.childrenOf(Let)[1], "10");
}

TEST(ParserTestExpr, ExprStmtLiteral) {
  parseSource("fn main() { 42; }");

  EXPECT_TRUE(PR.ok());

  const NodeIndex Body = fnBody();
  EXPECT_TRUE(checkChildrenKinds(Body, NodeKind::LitExpr));
  checkInternedString(PR.Pool.childrenOf(Body)[0], "42");
}

TEST(ParserTestExpr, ExprStmtBinary) {
  parseSource("fn main() { 1 + 2; }");

  EXPECT_TRUE(PR.ok());

  const NodeIndex Body = fnBody();
  EXPECT_TRUE(checkChildrenKinds(Body, NodeKind::BinaryExpr));

  const NodeIndex Expr = PR.Pool.childrenOf(Body)[0];
  EXPECT_EQ(NodePool::payloadOp(PR.Pool[Expr].Payload),
            static_cast<uint16_t>(EToken::Kind::plus));
  EXPECT_TRUE(checkChildrenKinds(Expr, NodeKind::LitExpr, NodeKind::LitExpr));
  checkInternedString(PR.Pool.childrenOf(Expr)[0], "1");
  checkInternedString(PR.Pool.childrenOf(Expr)[1], "2");
}

TEST(ParserTestExpr, ExprStmtParen) {
  parseSource("fn main() { (1 + 2); }");

  EXPECT_TRUE(PR.ok());

  const NodeIndex Body = fnBody();
  // Parens are elided; the inner expression is a direct child
  EXPECT_TRUE(checkChildrenKinds(Body, NodeKind::BinaryExpr));

  const NodeIndex Expr = PR.Pool.childrenOf(Body)[0];
  EXPECT_EQ(NodePool::payloadOp(PR.Pool[Expr].Payload),
            static_cast<uint16_t>(EToken::Kind::plus));
}

TEST(ParserTestExpr, ExprStmtIdent) {
  parseSource("fn main() { x; }");

  EXPECT_TRUE(PR.ok());

  const NodeIndex Body = fnBody();
  EXPECT_TRUE(checkChildrenKinds(Body, NodeKind::IdentExpr));
  checkInternedString(PR.Pool.childrenOf(Body)[0], "x");
}

TEST(ParserTestExpr, ExprStmtUnary) {
  parseSource("fn main() { -5; }");

  EXPECT_TRUE(PR.ok());

  const NodeIndex Body = fnBody();
  EXPECT_TRUE(checkChildrenKinds(Body, NodeKind::UnaryExpr));

  const NodeIndex Expr = PR.Pool.childrenOf(Body)[0];
  EXPECT_EQ(NodePool::payloadOp(PR.Pool[Expr].Payload),
            static_cast<uint16_t>(EToken::Kind::minus));
  EXPECT_TRUE(checkChildrenKinds(Expr, NodeKind::LitExpr));
  checkInternedString(PR.Pool.childrenOf(Expr)[0], "5");
}
