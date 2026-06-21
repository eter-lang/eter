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

TEST(ParserTestUnsafe, UnsafeBlockEmpty) {
  parseSource("fn f() { unsafe {} }");

  EXPECT_TRUE(PR.ok());
  // FnDecl children: [ParamList, BlockExpr]
  const NodeIndex Body =
      PR.Pool.childrenOf(PR.Pool.childrenOf(PR.Root)[0]).back();
  const NodeIndex S = PR.Pool.childrenOf(Body)[0];
  EXPECT_EQ(PR.Pool.kindOf(S), NodeKind::UnsafeBlock);
  EXPECT_EQ(PR.Pool.childrenOf(S).size(), 0u);
}

TEST(ParserTestUnsafe, UnsafeBlockWithStmt) {
  parseSource("fn f() { unsafe { ret 42; } }");

  EXPECT_TRUE(PR.ok());
  const NodeIndex Body =
      PR.Pool.childrenOf(PR.Pool.childrenOf(PR.Root)[0]).back();
  const NodeIndex S = PR.Pool.childrenOf(Body)[0];
  EXPECT_EQ(PR.Pool.kindOf(S), NodeKind::UnsafeBlock);
  EXPECT_GT(PR.Pool.childrenOf(S).size(), 0u);
}

TEST(ParserTestUnsafe, UnsafeBlockInExpr) {
  parseSource("fn f() { let x = unsafe {}; }");

  EXPECT_TRUE(PR.ok());
  // FnDecl children: [ParamList, BlockExpr]
  const NodeIndex Body =
      PR.Pool.childrenOf(PR.Pool.childrenOf(PR.Root)[0]).back();
  const NodeIndex Let = PR.Pool.childrenOf(Body)[0];
  EXPECT_EQ(PR.Pool.kindOf(Let), NodeKind::LetStmt);
  // LetStmt children: [Expr (initializer)]
  const auto Children = PR.Pool.childrenOf(Let);
  const NodeIndex Init = Children.back();
  EXPECT_EQ(PR.Pool.kindOf(Init), NodeKind::UnsafeBlock);
}

TEST(ParserTestUnsafe, UnsafeFnAtTopLevel) {
  parseSource("unsafe fn foo() {}");

  EXPECT_TRUE(PR.ok());
  const NodeIndex Fn = PR.Pool.childrenOf(PR.Root)[0];
  EXPECT_EQ(PR.Pool.kindOf(Fn), NodeKind::FnDecl);
  EXPECT_NE(PR.Pool.flagsOf(Fn) & NodePool::UnsafeFlag, 0u);
}

TEST(ParserTestUnsafe, PubUnsafeFn) {
  parseSource("pub unsafe fn foo() {}");

  EXPECT_TRUE(PR.ok());
  const NodeIndex Fn = PR.Pool.childrenOf(PR.Root)[0];
  EXPECT_NE(PR.Pool.flagsOf(Fn) & NodePool::PubFlag, 0u);
  EXPECT_NE(PR.Pool.flagsOf(Fn) & NodePool::UnsafeFlag, 0u);
}

TEST(ParserTestUnsafe, UnsafeFnInsideImpl) {
  parseSource("impl Foo { unsafe fn bar() {} }");

  EXPECT_TRUE(PR.ok());
  const NodeIndex Impl = PR.Pool.childrenOf(PR.Root)[0];
  // ImplDecl children: [NullNode, Type, FnDecl]
  const NodeIndex Fn = PR.Pool.childrenOf(Impl).back();
  EXPECT_EQ(PR.Pool.kindOf(Fn), NodeKind::FnDecl);
  EXPECT_NE(PR.Pool.flagsOf(Fn) & NodePool::UnsafeFlag, 0u);
}

TEST(ParserTestUnsafe, UnsafeBlockMissingBody) {
  parseSource("fn f() { unsafe }");

  EXPECT_FALSE(PR.ok());
  expectDiag(DiagID::ExpectedOpen);
}
