//===----------------------------------------------------------------------===//
//
// Part of the Eter Project, under the Apache License v2.0 with LLVM Exceptions.
// See /LICENSE for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef ETER_LEXER_TOKEN_H
#define ETER_LEXER_TOKEN_H

#include <string_view>

namespace eter::lex {

/// Represents a single token produced by the lexer.
struct Token {
  /// The kind of token (identifier, number, string, keyword, symbol, etc.)
  enum class Kind {
    Identifier,
    Number,
    String,
    Keyword,
    Symbol,
    EndOfFile,
    Unknown
  };

  /// The kind of the token.
  Kind TokenKind;
  /// The actual text of the token as it appears in the source code.
  std::string_view Lexeme;
  /// The position of the token in the source code (e.g., byte offset).
  uint32_t StartPosition;

  explicit Token(Kind TokenKind, std::string_view Lexeme,
                 uint32_t StartPosition)
      : TokenKind(TokenKind), Lexeme(Lexeme), StartPosition(StartPosition) {}
};

} // namespace eter::lex

#endif // ETER_LEXER_TOKEN_H
