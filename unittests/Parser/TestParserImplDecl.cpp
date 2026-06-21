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

TEST(ParserTestImplDecl, InherentEmptyImpl) {
  parseSource("impl Foo {}");

  EXPECT_TRUE(PR.ok());
  EXPECT_TRUE(checkChildrenKinds(PR.Root, NodeKind::ImplDecl));

  const NodeIndex Impl = PR.Pool.childrenOf(PR.Root)[0];
  // Children: [NullNode (no trait), Type]
  EXPECT_EQ(PR.Pool.childrenOf(Impl).size(), 2u);
  // child[0] = NullNode (sentinel for no trait)
  EXPECT_EQ(PR.Pool.childrenOf(Impl)[0], NullNode);
  // child[1] = the implementing type
  EXPECT_EQ(PR.Pool.kindOf(PR.Pool.childrenOf(Impl)[1]), NodeKind::NamedType);
  checkInternedString(PR.Pool.childrenOf(Impl)[1], "Foo");
}

TEST(ParserTestImplDecl, InherentWithOneMethod) {
  parseSource("impl Foo { fn new(imm x: i32) {} }");

  EXPECT_TRUE(PR.ok());
  EXPECT_TRUE(checkChildrenKinds(PR.Root, NodeKind::ImplDecl));

  const NodeIndex Impl = PR.Pool.childrenOf(PR.Root)[0];
  // Children: [..., Type, FnDecl]
  const NodeIndex Type = PR.Pool.childrenOf(Impl)[1];
  checkInternedString(Type, "Foo");

  const NodeIndex Method = PR.Pool.childrenOf(Impl)[2];
  checkInternedString(Method, "new");
}

TEST(ParserTestImplDecl, TraitImpl) {
  parseSource("impl Shape for Rectangle { fn area(imm self: Self): f32 {} }");

  EXPECT_TRUE(PR.ok());
  EXPECT_TRUE(checkChildrenKinds(PR.Root, NodeKind::ImplDecl));

  const NodeIndex Impl = PR.Pool.childrenOf(PR.Root)[0];
  // Children: [..., TraitType, Type, FnDecl]
  const NodeIndex Trait = PR.Pool.childrenOf(Impl)[0];
  checkInternedString(Trait, "Shape");

  const NodeIndex Type = PR.Pool.childrenOf(Impl)[1];
  checkInternedString(Type, "Rectangle");
}

TEST(ParserTestImplDecl, TraitImplMultipleMethods) {
  parseSource("impl Shape for Rectangle {\n"
              "    fn area(imm self: Self): f32 {}\n"
              "    fn perimeter(imm self: Self): f32 {}\n"
              "}");

  EXPECT_TRUE(PR.ok());
  const NodeIndex Impl = PR.Pool.childrenOf(PR.Root)[0];
  checkInternedString(PR.Pool.childrenOf(Impl)[0], "Shape");
  checkInternedString(PR.Pool.childrenOf(Impl)[1], "Rectangle");
  // Each method FnDecl has a BlockExpr body (child count >= 3)
  EXPECT_GE(PR.Pool.childrenOf(PR.Pool.childrenOf(Impl)[2]).size(), 3u);
  EXPECT_GE(PR.Pool.childrenOf(PR.Pool.childrenOf(Impl)[3]).size(), 3u);
}

TEST(ParserTestImplDecl, InherentImplWithDocComment) {
  parseSource("/// Doc\nimpl Foo { fn bar() {} }");

  EXPECT_TRUE(PR.ok());
  const NodeIndex Impl = PR.Pool.childrenOf(PR.Root)[0];
  const auto Children = PR.Pool.childrenOf(Impl);
  // Children: [DocComment*, NullNode, Type, FnDecl*]
  EXPECT_GE(Children.size(), 4u);
  EXPECT_EQ(PR.Pool.kindOf(Children[0]), NodeKind::DocComment);
  EXPECT_EQ(Children[1], NullNode);
  EXPECT_EQ(PR.Pool.kindOf(Children[2]), NodeKind::NamedType);
  EXPECT_EQ(PR.Pool.kindOf(Children[3]), NodeKind::FnDecl);
}

TEST(ParserTestImplDecl, ImplMissingOpen) {
  parseSource("impl Foo");

  EXPECT_FALSE(PR.ok());
  expectDiag(DiagID::ExpectedOpen);
}

TEST(ParserTestImplDecl, ImplMissingClose) {
  parseSource("impl Foo {");

  EXPECT_FALSE(PR.ok());
  expectDiag(DiagID::ExpectedClose);
}

TEST(ParserTestImplDecl, ImplMissingType) {
  parseSource("impl {}");

  EXPECT_FALSE(PR.ok());
  expectDiag(DiagID::ExpectedTypeName);
}

TEST(ParserTestImplDecl, ImplMissingForKeyword) {
  // "impl Shape Rectangle { ... }" without "for" should parse as inherent impl
  // of "Shape", then fail on "Rectangle" as unexpected.
  parseSource("impl Shape Rectangle {}");

  EXPECT_FALSE(PR.ok());
}
