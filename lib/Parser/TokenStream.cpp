//===----------------------------------------------------------------------===//
//
// Part of the Eter Project, under the Apache License v2.0 with LLVM Exceptions.
// See /LICENSE for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "eter/Parser/TokenStream.h"

#include <variant>

namespace eter::parser {

TokenStream::TokenStream(std::vector<lexer::LexerItem> Items,
                         llvm::StringRef Source)
    : Source(Source) {
  Tokens.reserve(Items.size());
  for (auto &Item : Items) {
    if (std::holds_alternative<lexer::Token>(Item)) {
      auto T = std::get<lexer::Token>(std::move(Item));
      // Regular comments (`//`, `/* */`) carry no semantic value for the
      // parser and are dropped here. Doc comments (`///`) and file-level doc
      // comments (`//!`) are kept so the parser can attach them to the
      // declaration / source-file they document.
      if (T.TokenKind == lexer::Token::Kind::comment)
        continue;
      Tokens.push_back(T);
    } else {
      auto &Err = std::get<lexer::LexerError>(Item);
      // Replace error position with an `unknown` token for parser recovery.
      Tokens.push_back(
          lexer::Token(lexer::Token::Kind::unknown, Err.ErrorSpan));
      LexErrors.push_back(Err);
    }
  }
}

lexer::Token::Kind TokenStream::peek(uint32_t Offset) const {
  const uint32_t Idx = Cursor + Offset;
  if (Idx >= Tokens.size())
    return lexer::Token::Kind::eof;
  return Tokens[Idx].TokenKind;
}

lexer::Token TokenStream::peekToken(uint32_t Offset) const {
  const uint32_t Idx = Cursor + Offset;
  if (Idx >= Tokens.size()) {
    const uint32_t EndOff = Tokens.empty() ? 0 : Tokens.back().TokenSpan.End;
    return lexer::Token(lexer::Token::Kind::eof, Span{EndOff, EndOff});
  }
  return Tokens[Idx];
}

lexer::Token TokenStream::previous() const {
  if (Cursor == 0) {
    const Span Start = Tokens.empty() ? Span{0, 0} : Tokens.front().TokenSpan;
    return lexer::Token(lexer::Token::Kind::eof, Start);
  }
  return Tokens[Cursor - 1];
}

bool TokenStream::check(lexer::Token::Kind K) const { return peek() == K; }

bool TokenStream::atEof() const { return check(lexer::Token::Kind::eof); }

lexer::Token TokenStream::advance() {
  if (Cursor < Tokens.size())
    return Tokens[Cursor++];
  const uint32_t EndOff = Tokens.empty() ? 0 : Tokens.back().TokenSpan.End;
  return lexer::Token(lexer::Token::Kind::eof, Span{EndOff, EndOff});
}

bool TokenStream::consume(lexer::Token::Kind K) {
  if (!check(K))
    return false;
  advance();
  return true;
}

} // namespace eter::parser
