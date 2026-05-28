//===----------------------------------------------------------------------===//
//
// Part of the Eter Project, under the Apache License v2.0 with LLVM Exceptions.
// See /LICENSE for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "eter/Base/SourceBuffer.h"
#include "eter/Base/StringInterner.h"
#include "eter/Lexer/Lexer.h"
#include "eter/Parser/NodePool.h"
#include "eter/Parser/Parser.h"
#include "eter/Parser/TokenStream.h"

#include <llvm/Support/VirtualFileSystem.h>

#include <unistd.h>
#include <vector>

#include "TestParserHelper.h"
#include "gtest/gtest.h"

using namespace eter;
using namespace eter::parser;
using namespace eter::lexer;
using namespace ParserTestHelper;

using namespace std;

TEST(ParserTestExpr, LetBindingSimpleExpr) {
  StringInterner SI{};
  Lexer L;
  SourceBuffer SB = createTestBuffer("{ let x: i32 = 10; }");
  auto Tokens = L.lex(SB);

  const TokenStream TS = TokenStream(Tokens, SB.getBuffer());

  EXPECT_TRUE(PR.ok());

  PR = Parser::parse(TS, SI);
  EXPECT_TRUE(checkChildrenKinds(PR.Root, NodeKind::BlockExpr));
  const NodeIndex BlockDeclarationNode = PR.Pool.childrenOf(PR.Root)[0];

  EXPECT_TRUE(checkChildrenKinds(BlockDeclarationNode, NodeKind::LetStmt));
  const NodeIndex LetStmtNode = PR.Pool.childrenOf(BlockDeclarationNode)[0];

  EXPECT_TRUE(
      checkChildrenKinds(LetStmtNode, NodeKind::NamedType, NodeKind::LitExpr));
  checkInternedString(LetStmtNode, "x");
}
