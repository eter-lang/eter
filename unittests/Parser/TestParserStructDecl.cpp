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

TEST(ParserTestStructDecl, StructDeclUnitLike) {
  parseSource("struct Empty {}");

  EXPECT_TRUE(PR.ok());
  EXPECT_TRUE(checkChildrenKinds(PR.Root, NodeKind::StructDecl));

  const NodeIndex S = PR.Pool.childrenOf(PR.Root)[0];
  checkInternedString(S, "Empty");
  EXPECT_EQ(PR.Pool.childrenOf(S).size(), 0u);
  checkSpan(S, "struct Empty {}");
}

TEST(ParserTestStructDecl, StructDeclTwoFields) {
  parseSource("struct Point { imm x: i32, mut y: f64 }");

  EXPECT_TRUE(PR.ok());
  EXPECT_TRUE(checkChildrenKinds(PR.Root, NodeKind::StructDecl));

  const NodeIndex S = PR.Pool.childrenOf(PR.Root)[0];
  checkInternedString(S, "Point");
  EXPECT_TRUE(
      checkChildrenKinds(S, NodeKind::StructField, NodeKind::StructField));

  const NodeIndex F0 = PR.Pool.childrenOf(S)[0];
  checkInternedString(F0, "x");
  checkRegime(F0, Regime::Imm);
  EXPECT_TRUE(checkChildrenKinds(F0, NodeKind::NamedType));
  checkInternedString(PR.Pool.childrenOf(F0)[0], "i32");
  checkSpan(F0, "imm x: i32");

  const NodeIndex F1 = PR.Pool.childrenOf(S)[1];
  checkInternedString(F1, "y");
  checkRegime(F1, Regime::Mut);
  EXPECT_TRUE(checkChildrenKinds(F1, NodeKind::NamedType));
  checkInternedString(PR.Pool.childrenOf(F1)[0], "f64");
}

TEST(ParserTestStructDecl, StructDeclTrailingComma) {
  parseSource("struct Point { imm x: i32, imm y: i32, }");

  EXPECT_TRUE(PR.ok());
  const NodeIndex S = PR.Pool.childrenOf(PR.Root)[0];
  checkInternedString(S, "Point");
  EXPECT_TRUE(
      checkChildrenKinds(S, NodeKind::StructField, NodeKind::StructField));
}

TEST(ParserTestStructDecl, StructDeclFieldRegimes) {
  parseSource("struct B { imm a: i32, mut b: i32, proj c: i32 }");

  EXPECT_TRUE(PR.ok());
  const NodeIndex S = PR.Pool.childrenOf(PR.Root)[0];
  EXPECT_TRUE(checkChildrenKinds(S, NodeKind::StructField,
                                 NodeKind::StructField, NodeKind::StructField));

  const NodeIndex F0 = PR.Pool.childrenOf(S)[0];
  checkInternedString(F0, "a");
  checkRegime(F0, Regime::Imm);

  const NodeIndex F1 = PR.Pool.childrenOf(S)[1];
  checkInternedString(F1, "b");
  checkRegime(F1, Regime::Mut);

  const NodeIndex F2 = PR.Pool.childrenOf(S)[2];
  checkInternedString(F2, "c");
  checkRegime(F2, Regime::Proj);
}

TEST(ParserTestStructDecl, StructDeclWithDocComment) {
  parseSource("/// A 2D point.\nstruct Point { imm x: i32 }");

  EXPECT_TRUE(PR.ok());
  const NodeIndex S = PR.Pool.childrenOf(PR.Root)[0];
  checkInternedString(S, "Point");
  EXPECT_TRUE(
      checkChildrenKinds(S, NodeKind::DocComment, NodeKind::StructField));
}

TEST(ParserTestStructDecl, StructDeclFollowedByFn) {
  parseSource("struct S { imm x: i32 } fn main() {}");

  EXPECT_TRUE(PR.ok());
  EXPECT_TRUE(
      checkChildrenKinds(PR.Root, NodeKind::StructDecl, NodeKind::FnDecl));
}

TEST(ParserTestStructDecl, StructDeclMissingName) {
  parseSource("struct { imm x: i32 }");

  EXPECT_FALSE(PR.ok());
  expectDiag(DiagID::ExpectedName);
}

TEST(ParserTestStructDecl, StructDeclMissingOpenBrace) {
  parseSource("struct S imm x: i32 }");

  EXPECT_FALSE(PR.ok());
  expectDiag(DiagID::ExpectedOpen);
}

TEST(ParserTestStructDecl, StructDeclMissingCloseBrace) {
  parseSource("struct S { imm x: i32");

  EXPECT_FALSE(PR.ok());
  expectDiag(DiagID::ExpectedClose);
}

TEST(ParserTestStructDecl, StructDeclMissingFieldColon) {
  parseSource("struct S { imm x i32 }");

  EXPECT_FALSE(PR.ok());
  expectDiag(DiagID::ExpectedColon);
}

TEST(ParserTestStructDecl, StructDeclMissingFieldType) {
  parseSource("struct S { imm x: }");

  EXPECT_FALSE(PR.ok());
  expectDiag(DiagID::ExpectedTypeName);
}

TEST(ParserTestStructDecl, StructDeclFieldRegimeOptional) {
  parseSource("struct S { x: i32 }");

  EXPECT_TRUE(PR.ok());

  const NodeIndex S = PR.Pool.childrenOf(PR.Root)[0];
  EXPECT_TRUE(checkChildrenKinds(S, NodeKind::StructField));
  checkInternedString(PR.Pool.childrenOf(S)[0], "x");
  checkRegime(PR.Pool.childrenOf(S)[0], Regime::None);
}

TEST(ParserTestStructDecl, StructDeclRegimeWithoutNameSingleError) {
  // A field that never starts must produce exactly one diagnostic, not a
  // cascade of name/colon/type errors on the same token.
  parseSource("struct X { imm }\nfn main() {}");

  EXPECT_FALSE(PR.ok());
  expectDiag(DiagID::ExpectedName);
  EXPECT_EQ(PR.Errors.size(), 1u);

  const llvm::ArrayRef<NodeIndex> Top = PR.Pool.childrenOf(PR.Root);
  ASSERT_EQ(Top.size(), 2u);
  EXPECT_EQ(PR.Pool.kindOf(Top[0]), NodeKind::StructDecl);
  EXPECT_EQ(PR.Pool.kindOf(Top[1]), NodeKind::FnDecl);
}

TEST(ParserTestStructDecl, StructDeclBadFieldStartSingleError) {
  // A token that cannot start a field must produce exactly one diagnostic.
  parseSource("struct S { 42 }");

  EXPECT_FALSE(PR.ok());
  expectDiag(DiagID::ExpectedName);
  EXPECT_EQ(PR.Errors.size(), 1u);
}

TEST(ParserTestStructDecl, StructDeclMissingColonAndTypeSingleError) {
  parseSource("struct S { imm x }");

  EXPECT_FALSE(PR.ok());
  expectDiag(DiagID::ExpectedColon);
  EXPECT_EQ(PR.Errors.size(), 1u);
}

TEST(ParserTestStructDecl, StructDeclBadFieldSkipsToNextField) {
  parseSource("struct S { imm 42: i32, imm y: f32 }");

  EXPECT_FALSE(PR.ok());
  expectDiag(DiagID::ExpectedName);
  EXPECT_EQ(PR.Errors.size(), 1u);

  // The bad field becomes an Error node; the following field still parses.
  const NodeIndex S = PR.Pool.childrenOf(PR.Root)[0];
  EXPECT_TRUE(checkChildrenKinds(S, NodeKind::Error, NodeKind::StructField));
  checkInternedString(PR.Pool.childrenOf(S)[1], "y");
}

TEST(ParserTestStructDecl, StructDeclRecoversToNextDecl) {
  // The malformed struct must not swallow the following function.
  parseSource("struct S { imm x i32 } fn main() {}");

  EXPECT_FALSE(PR.ok());
  expectDiag(DiagID::ExpectedColon);

  const llvm::ArrayRef<NodeIndex> Top = PR.Pool.childrenOf(PR.Root);
  ASSERT_EQ(Top.size(), 2u);
  EXPECT_EQ(PR.Pool.kindOf(Top[0]), NodeKind::StructDecl);
  EXPECT_EQ(PR.Pool.kindOf(Top[1]), NodeKind::FnDecl);
}
