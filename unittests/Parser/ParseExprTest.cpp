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

#include <unistd.h>

#include "TestParserHelper.h"
#include "gtest/gtest.h"

using namespace eter;
using namespace eter::parser;
using namespace eter::lexer;
using namespace ParserTestHelper;

using namespace std;

TEST(ParserTestExpr, LetBindingSimpleLiteralExpr) {
  parseSource("fn main(){ let imm x: i32 = 10; }");

  EXPECT_TRUE(checkChildrenKinds(PR.Root, NodeKind::FnDecl));
  const NodeIndex FnDeclarationNode = PR.Pool.childrenOf(PR.Root)[0];

  EXPECT_TRUE(checkChildrenKinds(FnDeclarationNode, NodeKind::ParamList,
                                 NodeKind::BlockExpr));

  EXPECT_TRUE(checkChildrenKinds(PR.Pool.childrenOf(FnDeclarationNode)[0]));

  const NodeIndex BlockExprNode = PR.Pool.childrenOf(FnDeclarationNode)[1];
  EXPECT_TRUE(checkChildrenKinds(BlockExprNode, NodeKind::LetStmt));

  const NodeIndex LetStmtNode = PR.Pool.childrenOf(BlockExprNode)[0];
  EXPECT_TRUE(
      checkChildrenKinds(LetStmtNode, NodeKind::NamedType, NodeKind::LitExpr));
  checkInternedString(LetStmtNode, "x");
  checkRegime(LetStmtNode, Regime::Imm);
}
