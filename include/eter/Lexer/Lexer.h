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
#include <cstdint>
#include <variant>
#include <vector>

namespace eter::lexer {

struct LexerError {
  enum class Kind : uint8_t {
#define ETER_LEXER_ERROR(X, Y) X,
#include "eter/Lexer/LexerErrors.def"
#undef ETER_LEXER_ERROR
  };

  Kind ErrorKind;
  Span ErrorSpan;

  static const char *getErrorName(Kind K) {
    switch (K) {
#define ETER_LEXER_ERROR(X, Y)                                                \
  case Kind::X:                                                                \
    return Y;
#include "eter/Lexer/LexerErrors.def"
#undef ETER_LEXER_ERROR
    }
    return "unknown";
  }
};

using LexerItem = std::variant<Token, LexerError>;

/// A high-performance, incremental-ready lexer.
///
/// The lexer converts a raw character stream from a `SourceBuffer` into a
/// sequence of `LexerItem`s. It is designed to be highly stateless and operates
/// via zero-copy string references to support fast, incremental updates in
/// an IDE/Language Server environment.
class Lexer {
public:
  /// Default constructor.
  Lexer() = default;

  /// Lexes the entire contents of the provided source buffer from start to finish.
  /// \param SourceBuffer The source buffer to lex.
  /// \returns A vector of lexer items extracted from the source buffer.
  std::vector<LexerItem> lex(SourceBuffer &SourceBuffer) {
    return lex(SourceBuffer, Span(0, SourceBuffer.getBuffer().size()));
  }

  /// Lexes a specific byte range within the source buffer.
  ///
  /// Core engine for incremental lexing. It allows the caller
  /// to re-lex only the spans of text that have been modified, avoiding the
  /// overhead of re-lexing the entire file.
  /// \param SourceBuffer The source buffer to lex.
  /// \param Span The byte range within the source buffer to lex.
  /// \returns A vector of lexer items extracted from the specified span.
  std::vector<LexerItem> lex(SourceBuffer &SourceBuffer, Span Span);

private:
  /// Advances the internal cursor past any consecutive whitespace characters.
  void skipWhitespace();

  /// Specialized lexing routines for extracting specific token categories.
  /// These functions assume `CurPtr` is pointing to the start of the token,
  /// and they advance `CurPtr` to the token's end upon completion.
  /// \param Result The token to populate with the lexed information.
  /// \param TokStart A pointer to the start of the token in the source buffer.
  void lexIdentifier(Token &Result, const char *TokStart);

  /// \param Result The token to populate with the lexed information.
  /// \param TokStart A pointer to the start of the token in the source buffer.
  /// \returns true if the numeric literal is valid, false otherwise.
  bool lexNumericLiteral(Token &Result, const char *TokStart);

  /// \param Result The token to populate with the lexed information.
  /// \param TokStart A pointer to the start of the token in the source buffer.
  /// \param LexerItems The output list to append lexer errors to.
  /// \returns true if the string was properly terminated, false otherwise.
  bool lexStringLiteral(Token &Result, const char *TokStart,
                        std::vector<LexerItem> &LexerItems);

  /// \param Result The token to populate with the lexed information.
  /// \param TokStart A pointer to the start of the token in the source buffer.
  /// \param LexerItems The output list to append lexer errors to.
  /// \param CharCount The number of logical characters consumed.
  /// \returns true if the character literal was properly terminated, false otherwise.
  bool lexCharacterLiteral(Token &Result, const char *TokStart,
                           std::vector<LexerItem> &LexerItems,
                           size_t &CharCount);

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
