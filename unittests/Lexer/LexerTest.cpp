//===----------------------------------------------------------------------===//
//
// Part of the Eter Project, under the Apache License v2.0 with LLVM Exceptions.
// See /LICENSE for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "eter/Base/SourceBuffer.h"
#include "eter/Lexer/Lexer.h"

#include <llvm/Support/VirtualFileSystem.h>

#include <variant>
#include <vector>

#include "LexerHelper.h"
#include "gtest/gtest.h"

using namespace eter;
using namespace eter::lexer;
using eter::test::createTestBuffer;
using eter::test::getError;
using eter::test::getToken;
using eter::test::isError;
using eter::test::isToken;

//============================================================================//
// Test: Lexer Static Helper Functions
//============================================================================//

TEST(LexerTest, IsHexDigit) {
  // Valid hex digits
  EXPECT_TRUE(Lexer::isHexDigit('0'));
  EXPECT_TRUE(Lexer::isHexDigit('9'));
  EXPECT_TRUE(Lexer::isHexDigit('a'));
  EXPECT_TRUE(Lexer::isHexDigit('f'));
  EXPECT_TRUE(Lexer::isHexDigit('A'));
  EXPECT_TRUE(Lexer::isHexDigit('F'));

  // Invalid hex digits
  EXPECT_FALSE(Lexer::isHexDigit('g'));
  EXPECT_FALSE(Lexer::isHexDigit('G'));
  EXPECT_FALSE(Lexer::isHexDigit(' '));
  EXPECT_FALSE(Lexer::isHexDigit('-'));
  EXPECT_FALSE(Lexer::isHexDigit('@'));
}

TEST(LexerTest, IsReservedKeyword) {
  // These depend on what keywords are defined in TokenKinds.def
  EXPECT_FALSE(Lexer::isReservedKeyword("notakeyword"));
  EXPECT_FALSE(Lexer::isReservedKeyword("identifier123"));
  EXPECT_TRUE(Lexer::isReservedKeyword("type"));
  EXPECT_TRUE(Lexer::isReservedKeyword("return"));
  EXPECT_TRUE(Lexer::isReservedKeyword("ref"));
  EXPECT_TRUE(Lexer::isReservedKeyword("where"));
}

//============================================================================//
// Test: Basic Lexing - Identifiers
//============================================================================//

TEST(LexerTest, LexIdentifier) {
  Lexer L;
  auto Buffer = createTestBuffer("hello");
  auto Items = L.lex(Buffer);

  ASSERT_EQ(Items.size(), 2);
  EXPECT_TRUE(isToken(Items[0]));
  EXPECT_EQ(getToken(Items[0]).TokenKind, Token::Kind::identifier);
  EXPECT_TRUE(isToken(Items[1]));
  EXPECT_EQ(getToken(Items[1]).TokenKind, Token::Kind::eof);
}

TEST(LexerTest, LexIdentifierWithUnderscore) {
  Lexer L;
  auto Buffer = createTestBuffer("_test_123");
  auto Items = L.lex(Buffer);

  ASSERT_EQ(Items.size(), 2);
  EXPECT_TRUE(isToken(Items[0]));
  EXPECT_EQ(getToken(Items[0]).TokenKind, Token::Kind::identifier);
  EXPECT_TRUE(isToken(Items[1]));
  EXPECT_EQ(getToken(Items[1]).TokenKind, Token::Kind::eof);
}

TEST(LexerTest, LexMultipleIdentifiers) {
  Lexer L;
  auto Buffer = createTestBuffer("foo bar baz");
  auto Items = L.lex(Buffer);

  // Should have: foo, bar, baz, eof
  ASSERT_EQ(Items.size(), 4);
  EXPECT_EQ(getToken(Items[0]).TokenKind, Token::Kind::identifier);
  EXPECT_EQ(getToken(Items[1]).TokenKind, Token::Kind::identifier);
  EXPECT_EQ(getToken(Items[2]).TokenKind, Token::Kind::identifier);
  EXPECT_EQ(getToken(Items[3]).TokenKind, Token::Kind::eof);
}

//============================================================================//
// Test: Basic Lexing - Numeric Literals
//============================================================================//

TEST(LexerTest, LexIntegerLiteral) {
  Lexer L;
  auto Buffer = createTestBuffer("12345");
  auto Items = L.lex(Buffer);

  ASSERT_EQ(Items.size(), 2);
  EXPECT_TRUE(isToken(Items[0]));
  EXPECT_EQ(getToken(Items[0]).TokenKind, Token::Kind::integer_literal);
  EXPECT_TRUE(isToken(Items[1]));
  EXPECT_EQ(getToken(Items[1]).TokenKind, Token::Kind::eof);
}

TEST(LexerTest, LexHexIntegerLiteral) {
  Lexer L;
  auto Buffer = createTestBuffer("0x1A3F");
  auto Items = L.lex(Buffer);

  ASSERT_EQ(Items.size(), 2);
  EXPECT_TRUE(isToken(Items[0]));
  EXPECT_EQ(getToken(Items[0]).TokenKind, Token::Kind::integer_literal);
  EXPECT_TRUE(isToken(Items[1]));
  EXPECT_EQ(getToken(Items[1]).TokenKind, Token::Kind::eof);
}

TEST(LexerTest, LexFloatLiteral) {
  Lexer L;
  auto Buffer = createTestBuffer("123.456");
  auto Items = L.lex(Buffer);

  ASSERT_EQ(Items.size(), 2);
  EXPECT_TRUE(isToken(Items[0]));
  EXPECT_EQ(getToken(Items[0]).TokenKind, Token::Kind::float_literal);
  EXPECT_TRUE(isToken(Items[1]));
  EXPECT_EQ(getToken(Items[1]).TokenKind, Token::Kind::eof);
}

TEST(LexerTest, LexFloatFLiteral) {
  Lexer L;
  auto Buffer = createTestBuffer("123.4f");
  auto Items = L.lex(Buffer);

  ASSERT_EQ(Items.size(), 2);
  EXPECT_TRUE(isToken(Items[0]));
  EXPECT_EQ(getToken(Items[0]).TokenKind, Token::Kind::float_literal);
  EXPECT_TRUE(isToken(Items[1]));
  EXPECT_EQ(getToken(Items[1]).TokenKind, Token::Kind::eof);
}

//============================================================================//
// Test: Basic Lexing - String Literals
//============================================================================//

TEST(LexerTest, LexStringLiteral) {
  Lexer L;
  auto Buffer = createTestBuffer("\"hello world\"");
  auto Items = L.lex(Buffer);

  ASSERT_EQ(Items.size(), 2);
  EXPECT_TRUE(isToken(Items[0]));
  EXPECT_EQ(getToken(Items[0]).TokenKind, Token::Kind::string_literal);
  EXPECT_TRUE(isToken(Items[1]));
  EXPECT_EQ(getToken(Items[1]).TokenKind, Token::Kind::eof);
}

TEST(LexerTest, LexEmptyStringLiteral) {
  Lexer L;
  auto Buffer = createTestBuffer("\"\"");
  auto Items = L.lex(Buffer);

  ASSERT_EQ(Items.size(), 2);
  EXPECT_TRUE(isToken(Items[0]));
  EXPECT_EQ(getToken(Items[0]).TokenKind, Token::Kind::string_literal);
  EXPECT_TRUE(isToken(Items[1]));
  EXPECT_EQ(getToken(Items[1]).TokenKind, Token::Kind::eof);
}

TEST(LexerTest, LexStringWithEscape) {
  Lexer L;
  auto Buffer = createTestBuffer("\"hello\\nworld\\t!\"");
  auto Items = L.lex(Buffer);

  ASSERT_EQ(Items.size(), 2);
  EXPECT_TRUE(isToken(Items[0]));
  EXPECT_EQ(getToken(Items[0]).TokenKind, Token::Kind::string_literal);
  EXPECT_TRUE(isToken(Items[1]));
  EXPECT_EQ(getToken(Items[1]).TokenKind, Token::Kind::eof);
}

TEST(LexerTest, LexUnterminatedString) {
  Lexer L;
  auto Buffer = createTestBuffer("\"hello world");
  auto Items = L.lex(Buffer);

  // Should have an error for unterminated string
  bool FoundError = false;
  for (const auto &Item : Items) {
    if (isError(Item)) {
      EXPECT_EQ(getError(Item).ErrorKind,
                LexerError::Kind::UnterminatedStringLiteral);
      FoundError = true;
    }
  }
  EXPECT_TRUE(FoundError);
}

//============================================================================//
// Test: Basic Lexing - Character Literals
//============================================================================//

TEST(LexerTest, LexCharLiteral) {
  Lexer L;
  auto Buffer = createTestBuffer("'a'");
  auto Items = L.lex(Buffer);

  ASSERT_EQ(Items.size(), 2);
  EXPECT_TRUE(isToken(Items[0]));
  EXPECT_EQ(getToken(Items[0]).TokenKind, Token::Kind::char_literal);
  EXPECT_TRUE(isToken(Items[1]));
  EXPECT_EQ(getToken(Items[1]).TokenKind, Token::Kind::eof);
}

TEST(LexerTest, LexCharLiteralWithEscape) {
  Lexer L;
  auto Buffer = createTestBuffer("'\\n'");
  auto Items = L.lex(Buffer);

  ASSERT_EQ(Items.size(), 2);
  EXPECT_TRUE(isToken(Items[0]));
  EXPECT_EQ(getToken(Items[0]).TokenKind, Token::Kind::char_literal);
  EXPECT_TRUE(isToken(Items[1]));
  EXPECT_EQ(getToken(Items[1]).TokenKind, Token::Kind::eof);
}

TEST(LexerTest, LexUnterminatedChar) {
  Lexer L;
  auto Buffer = createTestBuffer("'a");
  auto Items = L.lex(Buffer);

  // Should have an error for unterminated char
  bool FoundError = false;
  for (const auto &Item : Items) {
    if (isError(Item)) {
      EXPECT_EQ(getError(Item).ErrorKind,
                LexerError::Kind::UnterminatedCharLiteral);
      FoundError = true;
    }
  }
  EXPECT_TRUE(FoundError);
}

//============================================================================//
// Test: Basic Lexing - Symbols and Operators
//============================================================================//

TEST(LexerTest, LexSimpleSymbols) {
  Lexer L;
  auto Buffer = createTestBuffer("(){}[],;:");
  auto Items = L.lex(Buffer);

  ASSERT_EQ(Items.size(), 10);
  EXPECT_EQ(getToken(Items[0]).TokenKind, Token::Kind::l_paren);
  EXPECT_EQ(getToken(Items[1]).TokenKind, Token::Kind::r_paren);
  EXPECT_EQ(getToken(Items[2]).TokenKind, Token::Kind::l_brace);
  EXPECT_EQ(getToken(Items[3]).TokenKind, Token::Kind::r_brace);
  EXPECT_EQ(getToken(Items[4]).TokenKind, Token::Kind::l_square);
  EXPECT_EQ(getToken(Items[5]).TokenKind, Token::Kind::r_square);
  EXPECT_EQ(getToken(Items[6]).TokenKind, Token::Kind::comma);
  EXPECT_EQ(getToken(Items[7]).TokenKind, Token::Kind::semi);
  EXPECT_EQ(getToken(Items[8]).TokenKind, Token::Kind::colon);
  EXPECT_EQ(getToken(Items[9]).TokenKind, Token::Kind::eof);
}

TEST(LexerTest, LexDoubleColon) {
  Lexer L;
  auto Buffer = createTestBuffer("::");
  auto Items = L.lex(Buffer);

  ASSERT_EQ(Items.size(), 2);
  EXPECT_EQ(getToken(Items[0]).TokenKind, Token::Kind::colon_colon);
  EXPECT_EQ(getToken(Items[1]).TokenKind, Token::Kind::eof);
}

TEST(LexerTest, LexArithmeticOps) {
  Lexer L;
  auto Buffer = createTestBuffer("+-*/");
  auto Items = L.lex(Buffer);

  ASSERT_EQ(Items.size(), 5);
  EXPECT_EQ(getToken(Items[0]).TokenKind, Token::Kind::plus);
  EXPECT_EQ(getToken(Items[1]).TokenKind, Token::Kind::minus);
  EXPECT_EQ(getToken(Items[2]).TokenKind, Token::Kind::star);
  EXPECT_EQ(getToken(Items[3]).TokenKind, Token::Kind::slash);
  EXPECT_EQ(getToken(Items[4]).TokenKind, Token::Kind::eof);
}

TEST(LexerTest, LexIncrementDecrement) {
  Lexer L;
  auto Buffer = createTestBuffer("++ --");
  auto Items = L.lex(Buffer);

  ASSERT_EQ(Items.size(), 3);
  EXPECT_EQ(getToken(Items[0]).TokenKind, Token::Kind::plus_plus);
  EXPECT_EQ(getToken(Items[1]).TokenKind, Token::Kind::minus_minus);
  EXPECT_EQ(getToken(Items[2]).TokenKind, Token::Kind::eof);
}

TEST(LexerTest, LexComparisonOps) {
  Lexer L;
  auto Buffer = createTestBuffer("< > <= >= == !=");
  auto Items = L.lex(Buffer);

  ASSERT_EQ(Items.size(), 7);
  EXPECT_EQ(getToken(Items[0]).TokenKind, Token::Kind::less);
  EXPECT_EQ(getToken(Items[1]).TokenKind, Token::Kind::greater);
  EXPECT_EQ(getToken(Items[2]).TokenKind, Token::Kind::less_eq);
  EXPECT_EQ(getToken(Items[3]).TokenKind, Token::Kind::greater_eq);
  EXPECT_EQ(getToken(Items[4]).TokenKind, Token::Kind::eq_eq);
  EXPECT_EQ(getToken(Items[5]).TokenKind, Token::Kind::bang_eq);
  EXPECT_EQ(getToken(Items[6]).TokenKind, Token::Kind::eof);
}

TEST(LexerTest, LexAssignmentOps) {
  Lexer L;
  auto Buffer = createTestBuffer("= += -= *= /=");
  auto Items = L.lex(Buffer);

  ASSERT_EQ(Items.size(), 6);
  EXPECT_EQ(getToken(Items[0]).TokenKind, Token::Kind::eq);
  EXPECT_EQ(getToken(Items[1]).TokenKind, Token::Kind::plus_eq);
  EXPECT_EQ(getToken(Items[2]).TokenKind, Token::Kind::minus_eq);
  EXPECT_EQ(getToken(Items[3]).TokenKind, Token::Kind::star_eq);
  EXPECT_EQ(getToken(Items[4]).TokenKind, Token::Kind::slash_eq);
  EXPECT_EQ(getToken(Items[5]).TokenKind, Token::Kind::eof);
}

TEST(LexerTest, LexArrow) {
  Lexer L;
  auto Buffer = createTestBuffer("->");
  auto Items = L.lex(Buffer);

  // Arrow is a reserved symbol, should produce an error + eof
  ASSERT_EQ(Items.size(), 2);
  EXPECT_TRUE(isError(Items[0])) << "Expected LexerError for reserved arrow";
  if (isError(Items[0])) {
    EXPECT_EQ(getError(Items[0]).ErrorKind, LexerError::Kind::ReservedSymbol);
  }
  EXPECT_TRUE(isToken(Items[1]));
  EXPECT_EQ(getToken(Items[1]).TokenKind, Token::Kind::eof);
}

TEST(LexerTest, LexFatArrow) {
  Lexer L;
  auto Buffer = createTestBuffer("=>");
  auto Items = L.lex(Buffer);

  ASSERT_EQ(Items.size(), 2);
  EXPECT_EQ(getToken(Items[0]).TokenKind, Token::Kind::fat_arrow);
  EXPECT_EQ(getToken(Items[1]).TokenKind, Token::Kind::eof);
}

//============================================================================//
// Test: Comment Lexing
//============================================================================//

TEST(LexerTest, LexSingleLineComment) {
  Lexer L;
  auto Buffer = createTestBuffer("// this is a comment\n");
  auto Items = L.lex(Buffer);

  ASSERT_EQ(Items.size(), 2);
  EXPECT_EQ(getToken(Items[0]).TokenKind, Token::Kind::comment);
  EXPECT_EQ(getToken(Items[1]).TokenKind, Token::Kind::eof);
}

TEST(LexerTest, LexMultiLineComment) {
  Lexer L;
  auto Buffer = createTestBuffer("/* this is a\nmulti-line\ncomment */");
  auto Items = L.lex(Buffer);

  ASSERT_EQ(Items.size(), 2);
  EXPECT_EQ(getToken(Items[0]).TokenKind, Token::Kind::comment);
  EXPECT_EQ(getToken(Items[1]).TokenKind, Token::Kind::eof);
}

TEST(LexerTest, LexDocComment) {
  Lexer L;
  auto Buffer = createTestBuffer("/// doc comment");
  auto Items = L.lex(Buffer);

  ASSERT_EQ(Items.size(), 2);
  EXPECT_EQ(getToken(Items[0]).TokenKind, Token::Kind::doc_comment);
  EXPECT_EQ(getToken(Items[1]).TokenKind, Token::Kind::eof);
}

TEST(LexerTest, LexFileDocComment) {
  Lexer L;
  auto Buffer = createTestBuffer("//! file doc comment");
  auto Items = L.lex(Buffer);

  ASSERT_EQ(Items.size(), 2);
  EXPECT_EQ(getToken(Items[0]).TokenKind, Token::Kind::file_doc_comment);
  EXPECT_EQ(getToken(Items[1]).TokenKind, Token::Kind::eof);
}

//============================================================================//
// Test: Whitespace Handling
//============================================================================//

TEST(LexerTest, LexWithWhitespace) {
  Lexer L;
  auto Buffer = createTestBuffer("   \t  hello  \n  world  ");
  auto Items = L.lex(Buffer);

  // Should skip whitespace and only return tokens
  EXPECT_GE(Items.size(), 2);
  EXPECT_EQ(getToken(Items[0]).TokenKind, Token::Kind::identifier);
  EXPECT_EQ(getToken(Items[1]).TokenKind, Token::Kind::identifier);
}

//============================================================================//
// Test: Invalid Character Handling
//============================================================================//

TEST(LexerTest, LexInvalidCharacter) {
  Lexer L;
  auto Buffer = createTestBuffer("~");
  auto Items = L.lex(Buffer);

  // Should have an error for invalid character
  bool FoundError = false;
  for (const auto &Item : Items) {
    if (isError(Item)) {
      EXPECT_EQ(getError(Item).ErrorKind, LexerError::Kind::InvalidCharacter);
      FoundError = true;
    }
  }
  EXPECT_TRUE(FoundError);
}

//============================================================================//
// Test: Unicode Escape Sequences
//============================================================================//

TEST(LexerTest, LexUnicodeEscape) {
  Lexer L;
  auto Buffer = createTestBuffer("\"\\u{0041}\""); // 'A' in unicode escape
  auto Items = L.lex(Buffer);

  ASSERT_EQ(Items.size(), 2);
  EXPECT_TRUE(isToken(Items[0]));
  EXPECT_EQ(getToken(Items[0]).TokenKind, Token::Kind::string_literal);
  EXPECT_EQ(getToken(Items[1]).TokenKind, Token::Kind::eof);
}

TEST(LexerTest, LexInvalidUnicodeEscape) {
  Lexer L;
  auto Buffer = createTestBuffer("\"\\u{}\""); // Empty unicode escape
  auto Items = L.lex(Buffer);

  // Should have an error for invalid unicode escape
  bool FoundError = false;
  for (const auto &Item : Items) {
    if (isError(Item)) {
      EXPECT_EQ(getError(Item).ErrorKind,
                LexerError::Kind::InvalidUnicodeEscape);
      FoundError = true;
    }
  }
  EXPECT_TRUE(FoundError);
}

//============================================================================//
// Test: Incremental Lexing
//============================================================================//

TEST(LexerTest, IncrementalLexing) {
  Lexer L;
  auto Buffer = createTestBuffer("hello world foo bar");

  // First lex only part of the buffer
  auto Items1 = L.lex(Buffer, Span(0, 11)); // "hello world"
  EXPECT_EQ(Items1.size(), 2);              // hello, world

  // Then lex the rest - "foo bar" starts at position 12 (after "hello world ")
  auto Items2 = L.lex(Buffer, Span(12, 19)); // "foo bar"
  EXPECT_EQ(Items2.size(), 2);               // foo, bar
}

//============================================================================//
// Test: Token Spans
//============================================================================//

TEST(LexerTest, TokenSpansCorrect) {
  Lexer L;
  const llvm::StringRef Input = "hello";
  auto Buffer = createTestBuffer(Input);
  auto Items = L.lex(Buffer);

  ASSERT_EQ(Items.size(), 2);
  auto &Tok = getToken(Items[0]);
  EXPECT_EQ(Tok.TokenSpan.Start, 0u);
  EXPECT_EQ(Tok.TokenSpan.End, 5u); // "hello" is 5 chars
  EXPECT_EQ(getToken(Items[1]).TokenKind, Token::Kind::eof);
}

TEST(LexerTest, MultipleTokenSpansCorrect) {
  Lexer L;
  const llvm::StringRef Input = "foo bar";
  auto Buffer = createTestBuffer(Input);
  auto Items = L.lex(Buffer);

  ASSERT_EQ(Items.size(), 3);
  EXPECT_EQ(getToken(Items[0]).TokenSpan.Start, 0u);
  EXPECT_EQ(getToken(Items[0]).TokenSpan.End, 3u);   // "foo"
  EXPECT_EQ(getToken(Items[1]).TokenSpan.Start, 4u); // space skipped
  EXPECT_EQ(getToken(Items[1]).TokenSpan.End, 7u);   // "bar"
  EXPECT_EQ(getToken(Items[2]).TokenKind, Token::Kind::eof);
}

//============================================================================//
// Test: More Symbols
//============================================================================//

TEST(LexerTest, LexDotAndAtAndQuestion) {
  Lexer L;
  auto Buffer = createTestBuffer(". @ ?");
  auto Items = L.lex(Buffer);

  ASSERT_EQ(Items.size(), 4);
  EXPECT_EQ(getToken(Items[0]).TokenKind, Token::Kind::dot);
  EXPECT_EQ(getToken(Items[1]).TokenKind, Token::Kind::at);
  EXPECT_EQ(getToken(Items[2]).TokenKind, Token::Kind::question);
  EXPECT_EQ(getToken(Items[3]).TokenKind, Token::Kind::eof);
}

TEST(LexerTest, LexMoreOperators) {
  Lexer L;
  auto Buffer = createTestBuffer("% ^ | & << >> && || !");
  auto Items = L.lex(Buffer);

  ASSERT_EQ(Items.size(), 10);
  EXPECT_EQ(getToken(Items[0]).TokenKind, Token::Kind::percent);
  EXPECT_EQ(getToken(Items[1]).TokenKind, Token::Kind::caret);
  EXPECT_EQ(getToken(Items[2]).TokenKind, Token::Kind::pipe);
  EXPECT_EQ(getToken(Items[3]).TokenKind, Token::Kind::amp);
  EXPECT_EQ(getToken(Items[4]).TokenKind, Token::Kind::less_less);
  EXPECT_EQ(getToken(Items[5]).TokenKind, Token::Kind::greater_greater);
  EXPECT_EQ(getToken(Items[6]).TokenKind, Token::Kind::amp_amp);
  EXPECT_EQ(getToken(Items[7]).TokenKind, Token::Kind::pipe_pipe);
  EXPECT_EQ(getToken(Items[8]).TokenKind, Token::Kind::bang);
  EXPECT_EQ(getToken(Items[9]).TokenKind, Token::Kind::eof);
}

TEST(LexerTest, LexMoreCompoundAssignments) {
  Lexer L;
  auto Buffer = createTestBuffer("%= &= |= ^= <<= >>=");
  auto Items = L.lex(Buffer);

  ASSERT_EQ(Items.size(), 7);
  EXPECT_EQ(getToken(Items[0]).TokenKind, Token::Kind::percent_eq);
  EXPECT_EQ(getToken(Items[1]).TokenKind, Token::Kind::amp_eq);
  EXPECT_EQ(getToken(Items[2]).TokenKind, Token::Kind::pipe_eq);
  EXPECT_EQ(getToken(Items[3]).TokenKind, Token::Kind::caret_eq);
  EXPECT_EQ(getToken(Items[4]).TokenKind, Token::Kind::less_less_eq);
  EXPECT_EQ(getToken(Items[5]).TokenKind, Token::Kind::greater_greater_eq);
  EXPECT_EQ(getToken(Items[6]).TokenKind, Token::Kind::eof);
}

//============================================================================//
// Test: All Keywords
//============================================================================//

TEST(LexerTest, LexKeywords) {
  Lexer L;
  auto Buffer = createTestBuffer(
      "as break const continue else enum false imm fn "
      "for if let match mod mut proj pub ret struct true use while");
  auto Items = L.lex(Buffer);

  ASSERT_EQ(Items.size(), 23);
  EXPECT_EQ(getToken(Items[0]).TokenKind, Token::Kind::kw_as);
  EXPECT_EQ(getToken(Items[1]).TokenKind, Token::Kind::kw_break);
  EXPECT_EQ(getToken(Items[2]).TokenKind, Token::Kind::kw_const);
  EXPECT_EQ(getToken(Items[3]).TokenKind, Token::Kind::kw_continue);
  EXPECT_EQ(getToken(Items[4]).TokenKind, Token::Kind::kw_else);
  EXPECT_EQ(getToken(Items[5]).TokenKind, Token::Kind::kw_enum);
  EXPECT_EQ(getToken(Items[6]).TokenKind, Token::Kind::kw_false);
  EXPECT_EQ(getToken(Items[7]).TokenKind, Token::Kind::kw_imm);
  EXPECT_EQ(getToken(Items[8]).TokenKind, Token::Kind::kw_fn);
  EXPECT_EQ(getToken(Items[9]).TokenKind, Token::Kind::kw_for);
  EXPECT_EQ(getToken(Items[10]).TokenKind, Token::Kind::kw_if);
  EXPECT_EQ(getToken(Items[11]).TokenKind, Token::Kind::kw_let);
  EXPECT_EQ(getToken(Items[12]).TokenKind, Token::Kind::kw_match);
  EXPECT_EQ(getToken(Items[13]).TokenKind, Token::Kind::kw_mod);
  EXPECT_EQ(getToken(Items[14]).TokenKind, Token::Kind::kw_mut);
  EXPECT_EQ(getToken(Items[15]).TokenKind, Token::Kind::kw_proj);
  EXPECT_EQ(getToken(Items[16]).TokenKind, Token::Kind::kw_pub);
  EXPECT_EQ(getToken(Items[17]).TokenKind, Token::Kind::kw_ret);
  EXPECT_EQ(getToken(Items[18]).TokenKind, Token::Kind::kw_struct);
  EXPECT_EQ(getToken(Items[19]).TokenKind, Token::Kind::kw_true);
  EXPECT_EQ(getToken(Items[20]).TokenKind, Token::Kind::kw_use);
  EXPECT_EQ(getToken(Items[21]).TokenKind, Token::Kind::kw_while);
  EXPECT_EQ(getToken(Items[22]).TokenKind, Token::Kind::eof);
}

//============================================================================//
// Test: Error Kinds
//============================================================================//

TEST(LexerTest, LexUnterminatedBlockComment) {
  Lexer L;
  auto Buffer = createTestBuffer("/* hello world");
  auto Items = L.lex(Buffer);

  bool FoundError = false;
  for (const auto &Item : Items) {
    if (isError(Item)) {
      EXPECT_EQ(getError(Item).ErrorKind,
                LexerError::Kind::UnterminatedBlockComment);
      FoundError = true;
    }
  }
  EXPECT_TRUE(FoundError);
}

TEST(LexerTest, LexInvalidEscapeSequence) {
  Lexer L;
  auto Buffer = createTestBuffer("\"\\z\"");
  auto Items = L.lex(Buffer);

  bool FoundError = false;
  for (const auto &Item : Items) {
    if (isError(Item)) {
      EXPECT_EQ(getError(Item).ErrorKind,
                LexerError::Kind::InvalidEscapeSequence);
      FoundError = true;
    }
  }
  EXPECT_TRUE(FoundError);
}

TEST(LexerTest, LexEmptyCharLiteral) {
  Lexer L;
  auto Buffer = createTestBuffer("''");
  auto Items = L.lex(Buffer);

  bool FoundError = false;
  for (const auto &Item : Items) {
    if (isError(Item)) {
      EXPECT_EQ(getError(Item).ErrorKind,
                LexerError::Kind::EmptyCharLiteral);
      FoundError = true;
    }
  }
  EXPECT_TRUE(FoundError);
}

TEST(LexerTest, LexMultiCharCharLiteral) {
  Lexer L;
  auto Buffer = createTestBuffer("'ab'");
  auto Items = L.lex(Buffer);

  bool FoundError = false;
  for (const auto &Item : Items) {
    if (isError(Item)) {
      EXPECT_EQ(getError(Item).ErrorKind,
                LexerError::Kind::MultiCharCharLiteral);
      FoundError = true;
    }
  }
  EXPECT_TRUE(FoundError);
}

TEST(LexerTest, LexInvalidNumericLiteral) {
  Lexer L;
  auto Buffer = createTestBuffer("0x");
  auto Items = L.lex(Buffer);

  bool FoundError = false;
  for (const auto &Item : Items) {
    if (isError(Item)) {
      EXPECT_EQ(getError(Item).ErrorKind,
                LexerError::Kind::InvalidNumericLiteral);
      FoundError = true;
    }
  }
  EXPECT_TRUE(FoundError);
}

TEST(LexerTest, LexTrailingNumericSeparator) {
  Lexer L;
  auto Buffer = createTestBuffer("42_");
  auto Items = L.lex(Buffer);

  bool FoundError = false;
  for (const auto &Item : Items) {
    if (isError(Item)) {
      EXPECT_EQ(getError(Item).ErrorKind,
                LexerError::Kind::InvalidNumericLiteral);
      FoundError = true;
    }
  }
  EXPECT_TRUE(FoundError);
}

TEST(LexerTest, LexReservedKeywords) {
  Lexer L;
  auto Buffer = createTestBuffer("type return ref where");
  auto Items = L.lex(Buffer);

  int ErrorCount = 0;
  for (const auto &Item : Items) {
    if (isError(Item)) {
      EXPECT_EQ(getError(Item).ErrorKind,
                LexerError::Kind::ReservedKeyword);
      ++ErrorCount;
    }
  }
  EXPECT_EQ(ErrorCount, 4);
}


