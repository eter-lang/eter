
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

TEST(ParserTestDecl, ConstDeclWithIntLit) {
  Lexer L;
  SI = StringInterner();
  SourceBuffer SB = createTestBuffer("const v: i32 = 10;");
  auto Tokens = L.lex(SB);
  const TokenStream Ts = TokenStream(Tokens, SB.getBuffer());

  PR = Parser::parse(Ts, SI);

  EXPECT_TRUE(PR.ok());
  EXPECT_TRUE(checkChildrenKinds(PR.Root, NodeKind::ConstDecl));

  const NodeIndex Cdecl = PR.Pool.childrenOf(PR.Root)[0];
  checkInternedString(Cdecl, "v");
  EXPECT_TRUE(
      checkChildrenKinds(Cdecl, NodeKind::NamedType, NodeKind::LitExpr));

  const NodeIndex Type = PR.Pool.childrenOf(Cdecl)[0];
  const NodeIndex Literal = PR.Pool.childrenOf(Cdecl)[1];

  checkInternedString(Type, "i32");
  checkInternedString(Literal, "10");
}

TEST(ParserTestDecl, ConstDeclWithFloatLit) {
  Lexer L;
  SI = StringInterner();
  SourceBuffer SB = createTestBuffer("const v: f32 = 10.0;");
  auto Tokens = L.lex(SB);
  const TokenStream Ts = TokenStream(Tokens, SB.getBuffer());

  PR = Parser::parse(Ts, SI);

  EXPECT_TRUE(PR.ok());
  EXPECT_TRUE(checkChildrenKinds(PR.Root, NodeKind::ConstDecl));

  const NodeIndex Cdecl = PR.Pool.childrenOf(PR.Root)[0];
  checkInternedString(Cdecl, "v");
  EXPECT_TRUE(
      checkChildrenKinds(Cdecl, NodeKind::NamedType, NodeKind::LitExpr));

  const NodeIndex Type = PR.Pool.childrenOf(Cdecl)[0];
  const NodeIndex Literal = PR.Pool.childrenOf(Cdecl)[1];
  checkInternedString(Type, "f32");
  checkInternedString(Literal, "10.0");
}

TEST(ParserTestDecl, ConstDeclWithBoolLit) {
  Lexer L;
  SI = StringInterner();
  SourceBuffer SB = createTestBuffer("const v: bool = true;");
  auto Tokens = L.lex(SB);
  const TokenStream Ts = TokenStream(Tokens, SB.getBuffer());

  PR = Parser::parse(Ts, SI);

  EXPECT_TRUE(PR.ok());
  EXPECT_TRUE(checkChildrenKinds(PR.Root, NodeKind::ConstDecl));

  const NodeIndex Cdecl = PR.Pool.childrenOf(PR.Root)[0];
  checkInternedString(Cdecl, "v");
  EXPECT_TRUE(
      checkChildrenKinds(Cdecl, NodeKind::NamedType, NodeKind::LitExpr));

  const NodeIndex Type = PR.Pool.childrenOf(Cdecl)[0];
  const NodeIndex Literal = PR.Pool.childrenOf(Cdecl)[1];
  checkInternedString(Type, "bool");
  checkInternedString(Literal, "true");
}