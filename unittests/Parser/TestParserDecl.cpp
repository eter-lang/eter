
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

#include <vector>

#include "TestParserHelper.h"
#include "gtest/gtest.h"

using namespace eter;
using namespace eter::parser;
using namespace eter::lexer;
using namespace ParserTestHelper;
// Testing the correct use of test suite. Remember to remove!
using namespace std;

TEST(ParserTestDecl, DISABLED_NodeKind) {
  Si = StringInterner();
  Lexer L;

  SourceBuffer Sb = createTestBuffer("fn foo() {}");
  auto Tokens = L.lex(Sb);
  const TokenStream Ts = TokenStream(Tokens, Sb.getBuffer());

  Pr = Parser::parse(Ts, Si);

  EXPECT_TRUE(Pr.ok());
  // NodeIndex root = pr.Root;
  // EXPECT_EQ(NodeKind::FnDecl, pr.Pool.kindOf(root));
}

TEST(ParserTestDecl, ConstDeclWithLit) {
  Lexer L;
  Si = StringInterner();
  SourceBuffer Sb = createTestBuffer("const v : i32 = 10;");
  auto Tokens = L.lex(Sb);
  const TokenStream Ts = TokenStream(Tokens, Sb.getBuffer());

  Pr = Parser::parse(Ts, Si);

  EXPECT_TRUE(Pr.ok());
  EXPECT_TRUE(checkChildrenKinds(Pr.Root, NodeKind::ConstDecl));

  const NodeIndex Cdecl = Pr.Pool.childrenOf(Pr.Root)[0];

  EXPECT_TRUE(
      checkChildrenKinds(Cdecl, NodeKind::NamedType, NodeKind::LitExpr));

  const NodeIndex Type = Pr.Pool.childrenOf(Cdecl)[0];
  const NodeIndex Literal = Pr.Pool.childrenOf(Cdecl)[1];

  checkInternedString(Type, "i32");
  checkInternedString(Literal, "10");
}
