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

TEST(ParserDiagnosticsTest, ExpectedFnName) {
  parseSource("fn ");
  EXPECT_FALSE(PR.ok());
  expectDiag(DiagID::ExpectedFnName);
}

TEST(ParserDiagnosticsTest, ExpectedConstSemi) {
  parseSource("const C: T = 1");
  EXPECT_FALSE(PR.ok());
  expectDiag(DiagID::ExpectedConstSemi);
}

TEST(ParserDiagnosticsTest, ExpectedConstName) {
  parseSource("const : T = 1;");
  EXPECT_FALSE(PR.ok());
  expectDiag(DiagID::ExpectedConstName);
}

TEST(ParserDiagnosticsTest, ExpectedConstEquals) {
  parseSource("const C: T 1;");
  EXPECT_FALSE(PR.ok());
  expectDiag(DiagID::ExpectedConstEquals);
}

TEST(ParserDiagnosticsTest, ExpectedConstLiteral) {
  parseSource("const C: T = ;");
  EXPECT_FALSE(PR.ok());
  expectDiag(DiagID::ExpectedConstLiteral);
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
