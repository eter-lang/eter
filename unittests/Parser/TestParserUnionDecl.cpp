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

#include <llvm/Support/VirtualFileSystem.h>

#include "TestParserHelper.h"
#include "gtest/gtest.h"

using namespace eter;
using namespace eter::parser;
using namespace eter::lexer;
using namespace ParserTestHelper;

TEST(ParserTestUnionDecl, UnionDeclTwoFields) {
  parseSource("union MyUnion { imm f1: u32, mut f2: f32 }");

  EXPECT_TRUE(PR.ok());
  EXPECT_TRUE(checkChildrenKinds(PR.Root, NodeKind::UnionDecl));

  const NodeIndex U = PR.Pool.childrenOf(PR.Root)[0];
  checkInternedString(U, "MyUnion");
  EXPECT_TRUE(
      checkChildrenKinds(U, NodeKind::StructField, NodeKind::StructField));
  checkSpan(U, "union MyUnion { imm f1: u32, mut f2: f32 }");

  const NodeIndex F0 = PR.Pool.childrenOf(U)[0];
  checkInternedString(F0, "f1");
  checkRegime(F0, Regime::Imm);
  EXPECT_TRUE(checkChildrenKinds(F0, NodeKind::NamedType));
  checkInternedString(PR.Pool.childrenOf(F0)[0], "u32");

  const NodeIndex F1 = PR.Pool.childrenOf(U)[1];
  checkInternedString(F1, "f2");
  checkRegime(F1, Regime::Mut);
  checkInternedString(PR.Pool.childrenOf(F1)[0], "f32");
}

TEST(ParserTestUnionDecl, UnionDeclEmpty) {
  parseSource("union Empty {}");

  EXPECT_TRUE(PR.ok());
  const NodeIndex U = PR.Pool.childrenOf(PR.Root)[0];
  checkInternedString(U, "Empty");
  EXPECT_EQ(PR.Pool.childrenOf(U).size(), 0u);
}

TEST(ParserTestUnionDecl, UnionDeclTrailingComma) {
  parseSource("union U { imm f1: u32, imm f2: f32, }");

  EXPECT_TRUE(PR.ok());
  const NodeIndex U = PR.Pool.childrenOf(PR.Root)[0];
  EXPECT_TRUE(
      checkChildrenKinds(U, NodeKind::StructField, NodeKind::StructField));
}

TEST(ParserTestUnionDecl, UnionDeclFollowedByFn) {
  parseSource("union U { imm f1: u32 } fn main() {}");

  EXPECT_TRUE(PR.ok());
  EXPECT_TRUE(
      checkChildrenKinds(PR.Root, NodeKind::UnionDecl, NodeKind::FnDecl));
}

TEST(ParserTestUnionDecl, UnionDeclMissingName) {
  parseSource("union { imm f1: u32 }");

  EXPECT_FALSE(PR.ok());
  expectDiag(DiagID::ExpectedName);
}

TEST(ParserTestUnionDecl, UnionDeclMissingOpenBrace) {
  parseSource("union U imm f1: u32 }");

  EXPECT_FALSE(PR.ok());
  expectDiag(DiagID::ExpectedOpen);
}

TEST(ParserTestUnionDecl, UnionDeclMissingCloseBrace) {
  parseSource("union U { imm f1: u32");

  EXPECT_FALSE(PR.ok());
  expectDiag(DiagID::ExpectedClose);
}

TEST(ParserTestUnionDecl, UnionDeclMissingFieldColon) {
  parseSource("union U { imm f1 u32 }");

  EXPECT_FALSE(PR.ok());
  expectDiag(DiagID::ExpectedColon);
}

TEST(ParserTestUnionDecl, UnionDeclFieldRegimeOptional) {
  parseSource("union U { f1: u32 }");

  EXPECT_TRUE(PR.ok());

  const NodeIndex U = PR.Pool.childrenOf(PR.Root)[0];
  EXPECT_TRUE(checkChildrenKinds(U, NodeKind::StructField));
  checkRegime(PR.Pool.childrenOf(U)[0], Regime::None);
}
