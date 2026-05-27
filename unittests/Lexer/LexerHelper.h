//===----------------------------------------------------------------------===//
//
// Part of the Eter Project, under the Apache License v2.0 with LLVM Exceptions.
// See /LICENSE for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// Shared helpers for lexer tests.
//
// Golden test pattern:
//   Each .et input file in unittests/GoldenTests/Inputs/ exercises a set
//   of language features. The test reads the file, runs the lexer on it,
//   and asserts the expected output using standard gtest macros.
//
//   To add a golden test:
//     1. Create <name>.et in unittests/GoldenTests/Inputs/.
//     2. Add a TEST(LexerGoldenTest, <Name>) case in LexerGoldenTest.cpp.
//
//===----------------------------------------------------------------------===//

#ifndef UNITTESTS_LEXER_LEXERHELPER_H
#define UNITTESTS_LEXER_LEXERHELPER_H

#include "eter/Base/DiagnosticEngine.h"
#include "eter/Base/SourceBuffer.h"
#include "eter/Lexer/Lexer.h"
#include "eter/Lexer/Token.h"

#include <llvm/Support/VirtualFileSystem.h>

#include <variant>

#include "gtest/gtest.h"

namespace eter::test {

inline SourceBuffer createTestBuffer(llvm::StringRef Content) {
  return SourceBuffer::makeFromString(Content);
}

inline bool isToken(const lexer::LexerItem &Item) {
  return std::holds_alternative<lexer::Token>(Item);
}

inline bool isError(const lexer::LexerItem &Item) {
  return std::holds_alternative<lexer::LexerError>(Item);
}

inline const lexer::Token &getToken(const lexer::LexerItem &Item) {
  return std::get<lexer::Token>(Item);
}

inline const lexer::LexerError &getError(const lexer::LexerItem &Item) {
  return std::get<lexer::LexerError>(Item);
}

inline std::pair<SourceBuffer, std::vector<lexer::LexerItem>>
lexBuffer(llvm::StringRef Source) {
  auto Buffer = createTestBuffer(Source);
  lexer::Lexer L;
  auto Items = L.lex(Buffer);
  return {std::move(Buffer), std::move(Items)};
}

inline std::pair<SourceBuffer, std::vector<lexer::LexerItem>>
lexFile(const char *TestName) {
  const std::string InputPath =
      std::string(GOLDEN_TESTS_INPUT_DIR) + "/" + TestName + ".et";
  SimpleDiagnosticEngine SDE;
  auto VFS = llvm::vfs::getRealFileSystem();
  auto BufOrErr = SourceBuffer::makeFromFileName(*VFS, InputPath, SDE);
  EXPECT_TRUE(static_cast<bool>(BufOrErr)) << "Cannot open: " << InputPath;
  SourceBuffer Buffer = std::move(*BufOrErr);
  lexer::Lexer L;
  auto Items = L.lex(Buffer);
  return {std::move(Buffer), std::move(Items)};
}

inline void checkEndsWithEof(const std::vector<lexer::LexerItem> &Items) {
  ASSERT_FALSE(Items.empty());
  EXPECT_TRUE(isToken(Items.back()));
  EXPECT_EQ(getToken(Items.back()).TokenKind, lexer::Token::Kind::eof);
}

inline void checkNoErrors(const std::vector<lexer::LexerItem> &Items) {
  for (size_t I = 0; I + 1 < Items.size(); ++I)
    EXPECT_TRUE(isToken(Items[I])) << "index " << I << " is an error";
}

} // namespace eter::test
#endif
