//===----------------------------------------------------------------------===//
//
// Part of the Eter Project, under the Apache License v2.0 with LLVM Exceptions.
// See /LICENSE for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "eter/Parser/NodePool.h"

#include "TestParserHelper.h"
#include "gtest/gtest.h"

using namespace ParserTestHelper;

// Count children of `Node` whose kind matches `K`.
static size_t countOfKind(NodeIndex Node, NodeKind K) {
  size_t N = 0;
  for (const NodeIndex C : PR.Pool.childrenOf(Node))
    if (PR.Pool.kindOf(C) == K)
      ++N;
  return N;
}

static NodeIndex firstChildOfKind(NodeIndex Node, NodeKind K) {
  for (NodeIndex C : PR.Pool.childrenOf(Node))
    if (PR.Pool.kindOf(C) == K)
      return C;
  return NodeIndex{};
}

TEST(ParserDocCommentsTest, RegularLineCommentIsStripped) {
  parseSource("// this is just a regular comment\nfn foo() {}\n");
  ASSERT_TRUE(PR.ok());

  // SourceFile should have exactly one child: the FnDecl. The `//` comment
  // is filtered out by TokenStream.
  const auto Top = PR.Pool.childrenOf(PR.Root);
  ASSERT_EQ(Top.size(), 1u);
  EXPECT_EQ(PR.Pool.kindOf(Top[0]), NodeKind::FnDecl);
}

TEST(ParserDocCommentsTest, BlockCommentIsStripped) {
  parseSource("/* block comment */ fn foo() {}\n");
  ASSERT_TRUE(PR.ok());

  const auto Top = PR.Pool.childrenOf(PR.Root);
  ASSERT_EQ(Top.size(), 1u);
  EXPECT_EQ(PR.Pool.kindOf(Top[0]), NodeKind::FnDecl);
}

TEST(ParserDocCommentsTest, OuterDocCommentAttachesToFnDecl) {
  parseSource("/// fn-level doc\nfn foo() {}\n");
  ASSERT_TRUE(PR.ok());

  const auto Top = PR.Pool.childrenOf(PR.Root);
  ASSERT_EQ(Top.size(), 1u);
  EXPECT_EQ(PR.Pool.kindOf(Top[0]), NodeKind::FnDecl);

  EXPECT_EQ(countOfKind(Top[0], NodeKind::DocComment), 1u);

  // Doc-comment text is interned raw, including the `///` prefix.
  const NodeIndex Doc = firstChildOfKind(Top[0], NodeKind::DocComment);
  const auto Payload = NodePool::payloadStr(PR.Pool[Doc].Payload);
  EXPECT_EQ(SI.get(Payload), "/// fn-level doc");
}

TEST(ParserDocCommentsTest, MultipleOuterDocCommentsAttachInOrder) {
  parseSource("/// line 1\n"
              "/// line 2\n"
              "/// line 3\n"
              "fn foo() {}\n");
  ASSERT_TRUE(PR.ok());

  const auto Top = PR.Pool.childrenOf(PR.Root);
  ASSERT_EQ(Top.size(), 1u);

  const auto FnChildren = PR.Pool.childrenOf(Top[0]);
  ASSERT_GE(FnChildren.size(), 3u);
  EXPECT_EQ(PR.Pool.kindOf(FnChildren[0]), NodeKind::DocComment);
  EXPECT_EQ(PR.Pool.kindOf(FnChildren[1]), NodeKind::DocComment);
  EXPECT_EQ(PR.Pool.kindOf(FnChildren[2]), NodeKind::DocComment);

  EXPECT_EQ(SI.get(NodePool::payloadStr(PR.Pool[FnChildren[0]].Payload)),
            "/// line 1");
  EXPECT_EQ(SI.get(NodePool::payloadStr(PR.Pool[FnChildren[1]].Payload)),
            "/// line 2");
  EXPECT_EQ(SI.get(NodePool::payloadStr(PR.Pool[FnChildren[2]].Payload)),
            "/// line 3");
}

TEST(ParserDocCommentsTest, FileDocCommentAttachesToSourceFile) {
  parseSource("//! crate-level documentation\n"
              "fn foo() {}\n");
  ASSERT_TRUE(PR.ok());

  const auto Top = PR.Pool.childrenOf(PR.Root);
  ASSERT_EQ(Top.size(), 2u);
  EXPECT_EQ(PR.Pool.kindOf(Top[0]), NodeKind::FileDocComment);
  EXPECT_EQ(PR.Pool.kindOf(Top[1]), NodeKind::FnDecl);
}

TEST(ParserDocCommentsTest, DocCommentOnConstDecl) {
  parseSource("/// max retries\nconst MAX: T = 3;\n");
  ASSERT_TRUE(PR.ok());

  const auto Top = PR.Pool.childrenOf(PR.Root);
  ASSERT_EQ(Top.size(), 1u);
  EXPECT_EQ(PR.Pool.kindOf(Top[0]), NodeKind::ConstDecl);
  EXPECT_EQ(countOfKind(Top[0], NodeKind::DocComment), 1u);
}

TEST(ParserDocCommentsTest, DocCommentOnModFileDecl) {
  parseSource("/// nested module\nmod foo;\n");
  ASSERT_TRUE(PR.ok());

  const auto Top = PR.Pool.childrenOf(PR.Root);
  ASSERT_EQ(Top.size(), 1u);
  EXPECT_EQ(PR.Pool.kindOf(Top[0]), NodeKind::ModDeclFile);
  EXPECT_EQ(countOfKind(Top[0], NodeKind::DocComment), 1u);
}

TEST(ParserDocCommentsTest, DocCommentOnInlineMod) {
  parseSource("/// inline module\nmod foo { fn bar() {} }\n");
  ASSERT_TRUE(PR.ok());

  const auto Top = PR.Pool.childrenOf(PR.Root);
  ASSERT_EQ(Top.size(), 1u);
  EXPECT_EQ(PR.Pool.kindOf(Top[0]), NodeKind::ModDecl);
  EXPECT_EQ(countOfKind(Top[0], NodeKind::DocComment), 1u);
}

TEST(ParserDocCommentsTest, MixedFileDocsAndOuterDocs) {
  parseSource("//! file doc 1\n"
              "//! file doc 2\n"
              "/// fn doc\n"
              "fn foo() {}\n");
  ASSERT_TRUE(PR.ok());

  const auto Top = PR.Pool.childrenOf(PR.Root);
  ASSERT_EQ(Top.size(), 3u);
  EXPECT_EQ(PR.Pool.kindOf(Top[0]), NodeKind::FileDocComment);
  EXPECT_EQ(PR.Pool.kindOf(Top[1]), NodeKind::FileDocComment);
  EXPECT_EQ(PR.Pool.kindOf(Top[2]), NodeKind::FnDecl);
  EXPECT_EQ(countOfKind(Top[2], NodeKind::DocComment), 1u);
}

TEST(ParserDocCommentsTest, RegularCommentBetweenDocAndDeclIsTransparent) {
  parseSource("/// doc\n"
              "// regular comment\n"
              "fn foo() {}\n");
  ASSERT_TRUE(PR.ok());

  const auto Top = PR.Pool.childrenOf(PR.Root);
  ASSERT_EQ(Top.size(), 1u);
  EXPECT_EQ(PR.Pool.kindOf(Top[0]), NodeKind::FnDecl);
  EXPECT_EQ(countOfKind(Top[0], NodeKind::DocComment), 1u);
}
