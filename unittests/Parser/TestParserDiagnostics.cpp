//===----------------------------------------------------------------------===//
//
// Part of the Eter Project, under the Apache License v2.0 with LLVM Exceptions.
// See /LICENSE for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "eter/Base/PhaseDiagnostic.h"
#include "eter/Parser/Parser.h"
#include "eter/Parser/ParserDiagnostics.h"

#include "TestParserHelper.h"
#include "gtest/gtest.h"

using namespace ParserTestHelper;

TEST(ParserDiagnosticsTest, MissingFnName) {
  parseSource("fn ");
  EXPECT_FALSE(PR.ok());
  expectDiag(DiagID::ExpectedName);
}

TEST(ParserDiagnosticsTest, MissingConstSemi) {
  parseSource("const C: T = 1");
  EXPECT_FALSE(PR.ok());
  expectDiag(DiagID::ExpectedSemi);
}

TEST(ParserDiagnosticsTest, MissingConstName) {
  parseSource("const : T = 1;");
  EXPECT_FALSE(PR.ok());
  expectDiag(DiagID::ExpectedName);
}

TEST(ParserDiagnosticsTest, ExpectedEq) {
  parseSource("const C: T 1;");
  EXPECT_FALSE(PR.ok());
  expectDiag(DiagID::ExpectedEq);
}

// ExpectedConstLiteral was removed in P1f (Const Expression Generalization):
// const-expr now delegates to parseExpr, so a missing/invalid expression
// produces ExpectedExpr instead.
TEST(ParserDiagnosticsTest, ExpectedConstSemiAfterExpr) {
  parseSource("const C: T = ;");
  EXPECT_FALSE(PR.ok());
  expectDiag(DiagID::ExpectedExpr);
}

TEST(ParserDiagnosticsTest, ExpectedTopLevelDecl) {
  parseSource("123");
  EXPECT_FALSE(PR.ok());
  expectDiag(DiagID::ExpectedTopLevelDecl);
}

TEST(ParserDiagnosticsTest, ExpectedModOpenOrSemi) {
  parseSource("mod foo 42");
  EXPECT_FALSE(PR.ok());
  expectDiag(DiagID::ExpectedModOpenOrSemi);
}

TEST(ParserDiagnosticsTest, DiagnosticCarriesPhaseParser) {
  parseSource("const C: T = 1");
  ASSERT_FALSE(PR.Errors.empty());
  for (const auto &D : PR.Errors)
    EXPECT_EQ(D.Ph, eter::diag::Phase::Parser);
}

TEST(RenderMessageTest, SubstitutesNamedArg) {
  using namespace eter;
  const std::vector<std::pair<llvm::StringRef, std::string>> Args = {
      {"tok", "fn"}};
  EXPECT_EQ(diag::renderMessage("expected '{tok}'", Args), "expected 'fn'");
}

TEST(RenderMessageTest, LeavesUnmatchedPlaceholderVerbatim) {
  using namespace eter;
  EXPECT_EQ(diag::renderMessage("expected '{tok}'", {}), "expected '{tok}'");
}

TEST(RenderMessageTest, HandlesNoPlaceholders) {
  using namespace eter;
  EXPECT_EQ(diag::renderMessage("plain text", {}), "plain text");
}

//===----------------------------------------------------------------------===//
// Tests for diagnostic notes and help messages
//===----------------------------------------------------------------------===//

TEST(ParserDiagnosticsTest, ExpressionStmtMissingSemiHasNote) {
  parseSource("fn main() { 42 }");
  EXPECT_FALSE(PR.ok());
  expectDiagWithNote(DiagID::ExpectedSemi, "expression statements");
}

TEST(ParserDiagnosticsTest, ExpressionStmtMissingSemiHasHelp) {
  parseSource("fn main() { 42 }");
  EXPECT_FALSE(PR.ok());
  expectDiagWithHelp(DiagID::ExpectedSemi, "add `;`");
}

TEST(ParserDiagnosticsTest, LetStmtMissingSemiHasNote) {
  parseSource("fn main() { let x = 5 }");
  EXPECT_FALSE(PR.ok());
  expectDiagWithNote(DiagID::ExpectedSemi, "must end with `;`");
}

TEST(ParserDiagnosticsTest, LetStmtMissingSemiHasHelp) {
  parseSource("fn main() { let x = 5 }");
  EXPECT_FALSE(PR.ok());
  expectDiagWithHelp(DiagID::ExpectedSemi, "add `;`");
}
