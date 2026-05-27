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

// Testing the correct use of test suite. Remember to remove!
using namespace std;

TEST(ParserTestExpr, ConstDeclWithExpr) {
  SI = StringInterner();
  Lexer L;
  SourceBuffer SB = createTestBuffer("const v : i32 = 3 + 4;");
  auto Tokens = L.lex(SB);

  const TokenStream Ts = TokenStream(Tokens, SB.getBuffer());

  PR = Parser::parse(Ts, SI);

  EXPECT_TRUE(checkChildrenKinds(PR.Root, NodeKind::ConstDecl));
  const NodeIndex ConstDeclNode = PR.Pool.childrenOf(PR.Root)[0];

  checkInternedString(ConstDeclNode, "v");

  EXPECT_TRUE(checkChildrenKinds(ConstDeclNode, NodeKind::NamedType,
                                 NodeKind::BinaryExpr));

  const NodeIndex ExprNode = PR.Pool.childrenOf(ConstDeclNode)[1];
  EXPECT_EQ(NodePool::payloadOp(PR.Pool[ExprNode].Payload),
            static_cast<uint16_t>(lexer::Token::Kind::plus));

  EXPECT_TRUE(
      checkChildrenKinds(ExprNode, NodeKind::LitExpr, NodeKind::LitExpr));
}
