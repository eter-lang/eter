

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

#include <unistd.h>
#include <variant>
#include <vector>

#include "gtest/gtest.h"

using namespace eter;
using namespace eter::parser;
using namespace eter::lexer;

// Testing the correct use of test suite. Remember to remove!
using namespace std;

ParseResult pr;

static SourceBuffer createTestBuffer(llvm::StringRef Content) {
  return SourceBuffer::makeFromString(Content);
}

template <typename... Kinds>
static bool checkChildrenKinds(NodeIndex Node, Kinds... Expected) {
  // Ensure the caller only passes NodeKind arguments
  static_assert((std::is_same_v<Kinds, NodeKind> && ...),
                "All expected children arguments must be of type NodeKind");

  llvm::ArrayRef<NodeIndex> Children = pr.Pool.childrenOf(Node);

  // Early exit if the arity doesn't match the number of variadic arguments
  if (Children.size() != sizeof...(Expected))
    return false;

  size_t Index = 0;
  // Fold expression checking each child's kind against the expected variadic
  // pack
  return ((pr.Pool.kindOf(Children[Index++]) == Expected) && ...);
}

// ========================================== TESTS
// =================================

TEST(ParserTestExpr, ConstDecl) {
  StringInterner si = StringInterner();
  Lexer L;
  SourceBuffer bg = createTestBuffer("const v : i32 = 3 + 4;");
  auto tokens = L.lex(bg);

  TokenStream ts = TokenStream(tokens, bg.getBuffer());

  pr = Parser::parse(ts, si);

  EXPECT_TRUE(checkChildrenKinds(pr.Root, NodeKind::ConstDecl));
  NodeIndex constDeclNode = pr.Pool.childrenOf(pr.Root)[0];

  EXPECT_EQ(si.get(NodePool::payloadOp(pr.Pool[constDeclNode].Payload)), "v");

  EXPECT_TRUE(checkChildrenKinds(constDeclNode, NodeKind::NamedType,
                                 NodeKind::BinaryExpr));

  NodeIndex exprNode = pr.Pool.childrenOf(constDeclNode)[1];
  EXPECT_EQ(NodePool::payloadOp(pr.Pool[exprNode].Payload),
            static_cast<uint16_t>(lexer::Token::Kind::plus));

  EXPECT_TRUE(
      checkChildrenKinds(exprNode, NodeKind::LitExpr, NodeKind::LitExpr));
}
