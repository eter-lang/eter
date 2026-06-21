//===----------------------------------------------------------------------===//
//
// Part of the Eter Project, under the Apache License v2.0 with LLVM Exceptions.
// See /LICENSE for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "eter/Parser/NodePool.h"
#include "eter/Parser/Parser.h"

#include "TestParserHelper.h"
#include "gtest/gtest.h"

using namespace eter;
using namespace eter::parser;
using namespace eter::lexer;
using namespace ParserTestHelper;

TEST(ParserTestUseDecl, SimpleUse) {
  parseSource("use foo;");

  EXPECT_TRUE(PR.ok());
  EXPECT_TRUE(checkChildrenKinds(PR.Root, NodeKind::UseDecl));

  const NodeIndex Use = PR.Pool.childrenOf(PR.Root)[0];
  checkInternedString(Use, "foo");
  EXPECT_EQ(PR.Pool.flagsOf(Use) & NodePool::PubFlag, 0u);
}

TEST(ParserTestUseDecl, UseNestedPath) {
  parseSource("use foo::bar::Baz;");

  EXPECT_TRUE(PR.ok());
  EXPECT_TRUE(checkChildrenKinds(PR.Root, NodeKind::UseDecl));

  const NodeIndex Use = PR.Pool.childrenOf(PR.Root)[0];
  checkInternedString(Use, "foo::bar::Baz");
}

TEST(ParserTestUseDecl, PubUse) {
  parseSource("pub use math::Vec;");

  EXPECT_TRUE(PR.ok());
  EXPECT_TRUE(checkChildrenKinds(PR.Root, NodeKind::UseDecl));

  const NodeIndex Use = PR.Pool.childrenOf(PR.Root)[0];
  checkInternedString(Use, "math::Vec");
  EXPECT_NE(PR.Pool.flagsOf(Use) & NodePool::PubFlag, 0u);
}

TEST(ParserTestUseDecl, UseDeepPath) {
  parseSource("use a::b::c::d::e;");

  EXPECT_TRUE(PR.ok());
  EXPECT_TRUE(checkChildrenKinds(PR.Root, NodeKind::UseDecl));

  const NodeIndex Use = PR.Pool.childrenOf(PR.Root)[0];
  checkInternedString(Use, "a::b::c::d::e");
}

TEST(ParserTestUseDecl, UseSpan) {
  parseSource("use animals::Cat;");

  EXPECT_TRUE(PR.ok());
  const NodeIndex Use = PR.Pool.childrenOf(PR.Root)[0];
  checkSpan(Use, "use animals::Cat");
}

TEST(ParserTestUseDecl, MissingSemi) {
  parseSource("use foo");

  EXPECT_FALSE(PR.ok());
  expectDiag(DiagID::ExpectedSemi);
}

TEST(ParserTestUseDecl, MissingPath) {
  parseSource("use ;");

  EXPECT_FALSE(PR.ok());
  expectDiag(DiagID::ExpectedIdentifier);
}

TEST(ParserTestUseDecl, GlobImport) {
  parseSource("use foo::*;");

  EXPECT_TRUE(PR.ok());
  const NodeIndex Use = PR.Pool.childrenOf(PR.Root)[0];
  checkInternedString(Use, "foo::*");
}

TEST(ParserTestUseDecl, GlobImportDeepPath) {
  parseSource("use foo::bar::*;");

  EXPECT_TRUE(PR.ok());
  const NodeIndex Use = PR.Pool.childrenOf(PR.Root)[0];
  checkInternedString(Use, "foo::bar::*");
}

TEST(ParserTestUseDecl, Alias) {
  parseSource("use foo as Bar;");

  EXPECT_TRUE(PR.ok());
  const NodeIndex Use = PR.Pool.childrenOf(PR.Root)[0];
  checkInternedString(Use, "foo as Bar");
}

TEST(ParserTestUseDecl, AliasNestedPath) {
  parseSource("use foo::Bar as Baz;");

  EXPECT_TRUE(PR.ok());
  const NodeIndex Use = PR.Pool.childrenOf(PR.Root)[0];
  checkInternedString(Use, "foo::Bar as Baz");
}

TEST(ParserTestUseDecl, PubGlobImport) {
  parseSource("pub use math::*;");

  EXPECT_TRUE(PR.ok());
  const NodeIndex Use = PR.Pool.childrenOf(PR.Root)[0];
  checkInternedString(Use, "math::*");
  EXPECT_NE(PR.Pool.flagsOf(Use) & NodePool::PubFlag, 0u);
}
