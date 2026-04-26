//===----------------------------------------------------------------------===//
//
// Part of the Eter Project, under the Apache License v2.0 with LLVM Exceptions.
// See /LICENSE for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef ETER_LEXER_LEXER_H
#define ETER_LEXER_LEXER_H

#include "eter/Base/DiagnosticEngine.h"
#include "eter/Lexer/Token.h"

#include <llvm/ADT/StringRef.h>

namespace eter::lex {
class Lexer {
public:
  Lexer(llvm::StringRef SourceBuffer, DiagnosticEngine &Diagnostics)
      : SourceBuffer(SourceBuffer), CurrentPosition(SourceBuffer.begin()),
        Diagnostics(Diagnostics) {}

  void Lex(Token &Result) {};

private:
  /// Consumes whitespace and comments (handling nested /* */ as per spec).
  void skipWhitespaceAndComments();

  /// Specialized lexing routines for different token categories.
  void lexIdentifier(Token &Result, const char *TokStart);
  void lexNumericLiteral(Token &Result, const char *TokStart);
  void lexStringLiteral(Token &Result, const char *TokStart,
                        bool IsRaw = false);

  llvm::StringRef SourceBuffer;
  const char *CurrentPosition;
  DiagnosticEngine &Diagnostics;
};
} // namespace eter::lex

#endif // ETER_LEXER_LEXER_H
