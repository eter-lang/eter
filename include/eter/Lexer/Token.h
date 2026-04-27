//===----------------------------------------------------------------------===//
//
// Part of the Eter Project, under the Apache License v2.0 with LLVM Exceptions.
// See /LICENSE for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef ETER_LEXER_TOKEN_H
#define ETER_LEXER_TOKEN_H

#include <llvm/ADT/StringRef.h>

namespace eter::lexer {

/// Represents a single token produced by the lexer.
struct Token {
  /// The kind of token (identifier, number, string, keyword, symbol, etc.)
  enum class Kind : uint16_t {
#define TOK(X) X,
#include "eter/Lexer/TokenKinds.def"
#undef TOK
  };

  /// The kind of the token.
  Kind TokenKind;
  /// The actual text of the token as it appears in the source code.
  llvm::StringRef Lexeme;
  /// The position of the token in the source code (e.g., byte offset).
  uint32_t StartPosition;

  explicit Token(Kind TokenKind, llvm::StringRef Lexeme, uint32_t StartPosition)
      : TokenKind(TokenKind), Lexeme(Lexeme), StartPosition(StartPosition) {}

  static const char *getTokenName(Kind K);
};

} // namespace eter::lexer

#endif // ETER_LEXER_TOKEN_H
