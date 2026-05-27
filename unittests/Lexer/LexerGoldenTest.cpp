//===----------------------------------------------------------------------===//
//
// Part of the Eter Project, under the Apache License v2.0 with LLVM Exceptions.
// See /LICENSE for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// File-driven lexer golden tests.
//
// Each .et file in GOLDEN_TESTS_INPUT_DIR is a short Eter source snippet
// that exercises specific language features. The test lexes the file and
// asserts the expected token stream using standard gtest macros.
//
// The shared .et inputs can also be used by other stages (parser, etc.),
// which is why they live in unittests/GoldenTests/Inputs/ rather than
// being duplicated per stage.
//
// To add a golden test:
//   1. Create <name>.et in unittests/GoldenTests/Inputs/.
//   2. Add a TEST(LexerGoldenTest, <Name>) case below.
//
//===----------------------------------------------------------------------===//

#include "eter/Lexer/Token.h"

#include "LexerHelper.h"
#include "gtest/gtest.h"

using namespace eter;
using namespace eter::lexer;
using eter::test::getToken;
using eter::test::isToken;
using eter::test::lexFile;

//============================================================================//
// Tests -- one per .et file
//============================================================================//

TEST(LexerGoldenTest, Empty) {
  auto [Buffer, Items] = lexFile("empty");

  ASSERT_EQ(Items.size(), 1);
  EXPECT_EQ(getToken(Items[0]).TokenKind, Token::Kind::eof);
}

TEST(LexerGoldenTest, EmptyMain) {
  auto [Buffer, Items] = lexFile("emptymain");

  ASSERT_EQ(Items.size(), 7);
  EXPECT_TRUE(isToken(Items[0]));
  EXPECT_EQ(getToken(Items[0]).TokenKind, Token::Kind::kw_fn);
  EXPECT_TRUE(isToken(Items[1]));
  EXPECT_EQ(getToken(Items[1]).TokenKind, Token::Kind::identifier);
  EXPECT_TRUE(isToken(Items[2]));
  EXPECT_EQ(getToken(Items[2]).TokenKind, Token::Kind::l_paren);
  EXPECT_TRUE(isToken(Items[3]));
  EXPECT_EQ(getToken(Items[3]).TokenKind, Token::Kind::r_paren);
  EXPECT_TRUE(isToken(Items[4]));
  EXPECT_EQ(getToken(Items[4]).TokenKind, Token::Kind::l_brace);
  EXPECT_TRUE(isToken(Items[5]));
  EXPECT_EQ(getToken(Items[5]).TokenKind, Token::Kind::r_brace);
  EXPECT_TRUE(isToken(Items[6]));
  EXPECT_EQ(getToken(Items[6]).TokenKind, Token::Kind::eof);
}
