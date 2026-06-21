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

TEST(ParserTestTraitDecl, EmptyTrait) {
  parseSource("trait Shape {}");

  EXPECT_TRUE(PR.ok());
  EXPECT_TRUE(checkChildrenKinds(PR.Root, NodeKind::TraitDecl));

  const NodeIndex Trait = PR.Pool.childrenOf(PR.Root)[0];
  checkInternedString(Trait, "Shape");
  EXPECT_EQ(PR.Pool.childrenOf(Trait).size(), 0u);
}

TEST(ParserTestTraitDecl, TraitWithOneMethod) {
  parseSource("trait Shape { fn area(imm self: Self): f32; }");

  EXPECT_TRUE(PR.ok());
  EXPECT_TRUE(checkChildrenKinds(PR.Root, NodeKind::TraitDecl));

  const NodeIndex Trait = PR.Pool.childrenOf(PR.Root)[0];
  checkInternedString(Trait, "Shape");
  EXPECT_TRUE(checkChildrenKinds(Trait, NodeKind::FnDecl));

  const NodeIndex Method = PR.Pool.childrenOf(Trait)[0];
  checkInternedString(Method, "area");
  // FnDecl children: [ParamList, NamedType (return), NullNode (no body)]
  EXPECT_EQ(PR.Pool.childrenOf(Method).size(), 3u);
  EXPECT_EQ(PR.Pool.kindOf(PR.Pool.childrenOf(Method)[0]), NodeKind::ParamList);
  EXPECT_EQ(PR.Pool.kindOf(PR.Pool.childrenOf(Method)[1]), NodeKind::NamedType);
  EXPECT_EQ(PR.Pool.childrenOf(Method)[2], NullNode);
}

TEST(ParserTestTraitDecl, TraitWithMultipleMethods) {
  parseSource("trait Shape {\n"
              "    fn area(imm self: Self): f32;\n"
              "    fn perimeter(imm self: Self): f32;\n"
              "}");

  EXPECT_TRUE(PR.ok());
  const NodeIndex Trait = PR.Pool.childrenOf(PR.Root)[0];
  checkInternedString(Trait, "Shape");
  EXPECT_TRUE(checkChildrenKinds(Trait, NodeKind::FnDecl, NodeKind::FnDecl));

  const NodeIndex Area = PR.Pool.childrenOf(Trait)[0];
  checkInternedString(Area, "area");

  const NodeIndex Perimeter = PR.Pool.childrenOf(Trait)[1];
  checkInternedString(Perimeter, "perimeter");
}

TEST(ParserTestTraitDecl, PubTrait) {
  parseSource("pub trait Foo {}");

  EXPECT_TRUE(PR.ok());
  const NodeIndex Trait = PR.Pool.childrenOf(PR.Root)[0];
  EXPECT_NE(PR.Pool.flagsOf(Trait) & NodePool::PubFlag, 0u);
}

TEST(ParserTestTraitDecl, TraitMissingName) {
  parseSource("trait {}");

  EXPECT_FALSE(PR.ok());
  expectDiag(DiagID::ExpectedName);
}

TEST(ParserTestTraitDecl, TraitMissingOpen) {
  parseSource("trait Foo");

  EXPECT_FALSE(PR.ok());
  expectDiag(DiagID::ExpectedOpen);
}

TEST(ParserTestTraitDecl, TraitMissingClose) {
  parseSource("trait Foo {");

  EXPECT_FALSE(PR.ok());
  expectDiag(DiagID::ExpectedClose);
}

TEST(ParserTestTraitDecl, TraitMethodMissingSemi) {
  parseSource("trait Foo { fn bar() }");

  EXPECT_FALSE(PR.ok());
  expectDiag(DiagID::ExpectedTraitFnSemi);
}
