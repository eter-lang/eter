
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
#include "eter/Parser/ASTNodes.h"
#include "eter/Parser/NodePool.h"
#include "eter/Parser/Parser.h"
#include "eter/Parser/TokenStream.h"

#include <llvm/Support/VirtualFileSystem.h>

#include <variant>
#include <vector>

#include "gtest/gtest.h"

using namespace eter;
using namespace eter::parser;
using namespace eter::lexer;

// Testing the correct use of test suite. Remember to remove!
using namespace std;

static SourceBuffer createTestBuffer(llvm::StringRef Content) {
  return SourceBuffer::makeFromString(Content);
}

TEST(ParserTestDecl, disabled_NodeKind) {
  StringInterner si = StringInterner();
  Lexer L;

  SourceBuffer bf = createTestBuffer("fn foo() {}");
  auto tokens = L.lex(bf);
  TokenStream ts = TokenStream(tokens, bf.getBuffer());

  ParseResult pr = Parser::parse(ts, si);

  EXPECT_TRUE(pr.ok());
  // NodeIndex root = pr.Root;
  // EXPECT_EQ(NodeKind::FnDecl, pr.Pool.kindOf(root));
}
