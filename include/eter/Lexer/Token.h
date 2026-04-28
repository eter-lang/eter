//===----------------------------------------------------------------------===//
//
// Part of the Eter Project, under the Apache License v2.0 with LLVM Exceptions.
// See /LICENSE for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef ETER_LEXER_TOKEN_H
#define ETER_LEXER_TOKEN_H

#include "eter/Base/SourceManager.h"

#include <llvm/ADT/StringRef.h>

namespace eter::lexer {

/// Represents a single token produced by the lexer.
struct Token {
  /// The kind of token (identifier, number, string, keyword, symbol, etc.)
  enum class Kind : uint16_t {
#define ETER_TOKEN(X) X,
#include "eter/Lexer/TokenKinds.def"
#undef TOK
  };

  /// Create a Token with the given kind and span.
  /// \param TokenKind The kind of the token.
  /// \param TokenSpan The span representing the token's position in the source code.
  explicit Token(Kind TokenKind, Span TokenSpan)
      : TokenKind(TokenKind), TokenSpan(TokenSpan) {}

  /// Return the string representation of a token kind.
  /// \param K The token kind to get the name for.
  /// \returns A null-terminated C string with the name of the token kind.
  static const char *getTokenName(Kind K);

  /// The kind of the token.
  Kind TokenKind;
  /// The position of the token in the source code (e.g., byte offset).
  Span TokenSpan;
};

} // namespace eter::lexer

#endif // ETER_LEXER_TOKEN_H
