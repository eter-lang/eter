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

TEST(ParserTestEnumDecl, EnumDeclEmpty) {
  parseSource("enum Never {}");

  EXPECT_TRUE(PR.ok());
  EXPECT_TRUE(checkChildrenKinds(PR.Root, NodeKind::EnumDecl));

  const NodeIndex E = PR.Pool.childrenOf(PR.Root)[0];
  checkInternedString(E, "Never");
  EXPECT_EQ(PR.Pool.childrenOf(E).size(), 0u);
  checkSpan(E, "enum Never {}");
}

TEST(ParserTestEnumDecl, EnumDeclUnitOnly) {
  parseSource("enum Balls { Tennis, Golf, Soccer }");

  EXPECT_TRUE(PR.ok());
  const NodeIndex E = PR.Pool.childrenOf(PR.Root)[0];
  checkInternedString(E, "Balls");
  EXPECT_TRUE(checkChildrenKinds(E, NodeKind::EnumVariantUnit,
                                 NodeKind::EnumVariantUnit,
                                 NodeKind::EnumVariantUnit));

  const NodeIndex V0 = PR.Pool.childrenOf(E)[0];
  checkInternedString(V0, "Tennis");
  EXPECT_EQ(PR.Pool.childrenOf(V0).size(), 0u);
  checkSpan(V0, "Tennis");

  checkInternedString(PR.Pool.childrenOf(E)[1], "Golf");
  checkInternedString(PR.Pool.childrenOf(E)[2], "Soccer");
}

TEST(ParserTestEnumDecl, EnumDeclTrailingComma) {
  parseSource("enum Balls { Tennis, Golf, }");

  EXPECT_TRUE(PR.ok());
  const NodeIndex E = PR.Pool.childrenOf(PR.Root)[0];
  EXPECT_TRUE(checkChildrenKinds(E, NodeKind::EnumVariantUnit,
                                 NodeKind::EnumVariantUnit));
}

TEST(ParserTestEnumDecl, EnumDeclDiscriminants) {
  parseSource("enum Balls { Tennis, Golf = 10, Soccer }");

  EXPECT_TRUE(PR.ok());
  const NodeIndex E = PR.Pool.childrenOf(PR.Root)[0];
  EXPECT_TRUE(checkChildrenKinds(E, NodeKind::EnumVariantUnit,
                                 NodeKind::EnumVariantUnit,
                                 NodeKind::EnumVariantUnit));

  // Unspecified discriminant: no children.
  EXPECT_EQ(PR.Pool.childrenOf(PR.Pool.childrenOf(E)[0]).size(), 0u);

  // Explicit discriminant: a single ConstExpr child.
  const NodeIndex Golf = PR.Pool.childrenOf(E)[1];
  checkInternedString(Golf, "Golf");
  EXPECT_TRUE(checkChildrenKinds(Golf, NodeKind::LitExpr));
  checkInternedString(PR.Pool.childrenOf(Golf)[0], "10");
  checkSpan(Golf, "Golf = 10");

  EXPECT_EQ(PR.Pool.childrenOf(PR.Pool.childrenOf(E)[2]).size(), 0u);
}

TEST(ParserTestEnumDecl, EnumDeclTupleVariant) {
  parseSource("enum Animals { Dog(str, i32) }");

  EXPECT_TRUE(PR.ok());
  const NodeIndex E = PR.Pool.childrenOf(PR.Root)[0];
  EXPECT_TRUE(checkChildrenKinds(E, NodeKind::EnumVariantTuple));

  const NodeIndex Dog = PR.Pool.childrenOf(E)[0];
  checkInternedString(Dog, "Dog");
  EXPECT_TRUE(
      checkChildrenKinds(Dog, NodeKind::NamedType, NodeKind::NamedType));
  checkInternedString(PR.Pool.childrenOf(Dog)[0], "str");
  checkInternedString(PR.Pool.childrenOf(Dog)[1], "i32");
  checkSpan(Dog, "Dog(str, i32)");
}

TEST(ParserTestEnumDecl, EnumDeclStructVariant) {
  parseSource("enum Animals { Cat { imm name: str, mut age: i32 } }");

  EXPECT_TRUE(PR.ok());
  const NodeIndex E = PR.Pool.childrenOf(PR.Root)[0];
  EXPECT_TRUE(checkChildrenKinds(E, NodeKind::EnumVariantStruct));

  const NodeIndex Cat = PR.Pool.childrenOf(E)[0];
  checkInternedString(Cat, "Cat");
  EXPECT_TRUE(
      checkChildrenKinds(Cat, NodeKind::StructField, NodeKind::StructField));

  const NodeIndex F0 = PR.Pool.childrenOf(Cat)[0];
  checkInternedString(F0, "name");
  checkRegime(F0, Regime::Imm);
  checkInternedString(PR.Pool.childrenOf(F0)[0], "str");

  const NodeIndex F1 = PR.Pool.childrenOf(Cat)[1];
  checkInternedString(F1, "age");
  checkRegime(F1, Regime::Mut);
  checkInternedString(PR.Pool.childrenOf(F1)[0], "i32");
}

TEST(ParserTestEnumDecl, EnumDeclMixedVariants) {
  parseSource("enum Animals {"
              "  Dog( str, i32 ),"
              "  Cat{ imm name: str, imm age: i32 },"
              "  Reptile,"
              "}");

  EXPECT_TRUE(PR.ok());
  const NodeIndex E = PR.Pool.childrenOf(PR.Root)[0];
  checkInternedString(E, "Animals");
  EXPECT_TRUE(checkChildrenKinds(E, NodeKind::EnumVariantTuple,
                                 NodeKind::EnumVariantStruct,
                                 NodeKind::EnumVariantUnit));
}

TEST(ParserTestEnumDecl, EnumDeclFollowedByFn) {
  parseSource("enum E { A, B } fn main() {}");

  EXPECT_TRUE(PR.ok());
  EXPECT_TRUE(
      checkChildrenKinds(PR.Root, NodeKind::EnumDecl, NodeKind::FnDecl));
}

TEST(ParserTestEnumDecl, EnumDeclMissingName) {
  parseSource("enum { A }");

  EXPECT_FALSE(PR.ok());
  expectDiag(DiagID::ExpectedName);
}

TEST(ParserTestEnumDecl, EnumDeclMissingCloseBrace) {
  parseSource("enum E { A, B");

  EXPECT_FALSE(PR.ok());
  expectDiag(DiagID::ExpectedClose);
}

TEST(ParserTestEnumDecl, EnumDeclMissingVariantName) {
  parseSource("enum E { 42 }");

  EXPECT_FALSE(PR.ok());
  expectDiag(DiagID::ExpectedName);
}

TEST(ParserTestEnumDecl, EnumDeclBadVariantRecoversToNext) {
  // A malformed variant must produce exactly one diagnostic and must not
  // prevent the following variant from parsing.
  parseSource("enum E { 42, B }\nfn main() {}");

  EXPECT_FALSE(PR.ok());
  expectDiag(DiagID::ExpectedName);
  EXPECT_EQ(PR.Errors.size(), 1u);

  const NodeIndex E = PR.Pool.childrenOf(PR.Root)[0];
  EXPECT_TRUE(
      checkChildrenKinds(E, NodeKind::Error, NodeKind::EnumVariantUnit));
  checkInternedString(PR.Pool.childrenOf(E)[1], "B");
}

TEST(ParserTestEnumDecl, EnumDeclTupleVariantMissingClose) {
  parseSource("enum E { Dog(str, i32 }");

  EXPECT_FALSE(PR.ok());
  expectDiag(DiagID::ExpectedClose);
}

TEST(ParserTestEnumDecl, EnumDeclNonLiteralDiscriminant) {
  parseSource("enum E { A = foo }");

  EXPECT_TRUE(PR.ok());
  // Non-literal discriminants (identifiers) are valid syntax; semantic
  // validation is deferred to the type checker.
  const NodeIndex Var = PR.Pool.childrenOf(PR.Pool.childrenOf(PR.Root)[0])[0];
  const auto VarChildren = PR.Pool.childrenOf(Var);
  // EnumVariantUnit: [ConstExpr?]
  // foo is parsed as IdentExpr
  ASSERT_GT(VarChildren.size(), 0u);
  EXPECT_EQ(PR.Pool.kindOf(VarChildren[0]), NodeKind::IdentExpr);
}
