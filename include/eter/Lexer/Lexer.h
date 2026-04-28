//===----------------------------------------------------------------------===//
//
// Part of the Eter Project, under the Apache License v2.0 with LLVM Exceptions.
// See /LICENSE for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef ETER_LEXER_LEXER_H
#define ETER_LEXER_LEXER_H

#include "eter/Base/SourceBuffer.h"
#include "eter/Lexer/Token.h"

#include <llvm/ADT/StringRef.h>

#include <cstddef>
#include <vector>

namespace eter::lexer {

/// A high-performance, incremental-ready lexer.
///
/// The lexer converts a raw character stream from a `SourceBuffer` into a
/// sequence of `Token`s. It is designed to be highly stateless and operates
/// via zero-copy string references to support fast, incremental updates in
/// an IDE/Language Server environment.
class Lexer {
public:
  Lexer() = default;

  /// Lexes the entire contents of the provided source buffer from start to finish.
  std::vector<Token> lex(SourceBuffer &SourceBuffer){
      return lex(SourceBuffer, Span(0, SourceBuffer.getBuffer().size()));
  }

  /// Lexes a specific byte range within the source buffer.
  ///
  /// This is the core engine for incremental lexing. It allows the caller
  /// to re-lex only the spans of text that have been modified, avoiding the
  /// overhead of re-lexing the entire file.
  std::vector<Token> lex(SourceBuffer &SourceBuffer, Span Span);

private:
  /// Advances the internal cursor past any consecutive whitespace characters
  /// and comments (handling nested /* */ block comments as per spec).
  void skipWhitespaceAndComments();

  /// Specialized lexing routines for extracting specific token categories.
  /// These functions assume `CurPtr` is pointing to the start of the token,
  /// and they advance `CurPtr` to the token's end upon completion.
  void lexIdentifier(Token &Result, const char *TokStart);
  void lexNumericLiteral(Token &Result, const char *TokStart);
  void lexStringLiteral(Token &Result, const char *TokStart);
  void lexCharacterLiteral(Token &Result, const char *TokStart);

  //===--------------------------------------------------------------------===//
  // Internal Traversal State
  //
  // These pointers manage the "sliding window" over the raw memory buffer
  // during a `lexSpan` operation. They are re-initialized per lexing request.
  //===--------------------------------------------------------------------===//

  /// Pointer to the very beginning of the source buffer's memory.
  /// Used as a base anchor to calculate `Span` byte offsets.
  const char *BufferStart = nullptr;

  /// Pointer to the current character being inspected by the lexer.
  const char *CurPtr = nullptr;

  /// Pointer to the memory boundary immediately following the lexing span.
  const char *BufferEnd = nullptr;
};
} // namespace eter::lexer

#endif // ETER_LEXER_LEXER_H
