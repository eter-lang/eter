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

// ========================================== TESTS
// =================================

TEST(ParserTestExpr, ConstDeclWithExpr) {
  Si = StringInterner();
  Lexer L;
  SourceBuffer Sb = createTestBuffer("const v : i32 = 3 + 4;");
  auto Tokens = L.lex(Sb);

  const TokenStream Ts = TokenStream(Tokens, Sb.getBuffer());

  Pr = Parser::parse(Ts, Si);

  EXPECT_TRUE(checkChildrenKinds(Pr.Root, NodeKind::ConstDecl));
  const NodeIndex ConstDeclNode = Pr.Pool.childrenOf(Pr.Root)[0];

  checkInternedString(ConstDeclNode, "v");

  EXPECT_TRUE(checkChildrenKinds(ConstDeclNode, NodeKind::NamedType,
                                 NodeKind::BinaryExpr));

  const NodeIndex ExprNode = Pr.Pool.childrenOf(ConstDeclNode)[1];
  EXPECT_EQ(NodePool::payloadOp(Pr.Pool[ExprNode].Payload),
            static_cast<uint16_t>(lexer::Token::Kind::plus));

  EXPECT_TRUE(
      checkChildrenKinds(ExprNode, NodeKind::LitExpr, NodeKind::LitExpr));
}
