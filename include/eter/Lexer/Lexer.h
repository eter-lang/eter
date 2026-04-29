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

/// Represents a lexer error with a specific kind and the source span where it
/// occurred. Errors are non-fatal and are collected alongside valid tokens
/// during lexing.
struct LexerError {
  enum class Kind : uint8_t {
#define ETER_LEXER_ERROR(X, Y) X,
#include "eter/Lexer/LexerErrors.def"
#undef ETER_LEXER_ERROR
  };

  Kind ErrorKind;
  Span ErrorSpan;

  static const llvm::StringRef getErrorString(Kind K) {
    switch (K) {
#define ETER_LEXER_ERROR(X, Y)                                                 \
  case Kind::X:                                                                \
    return Y;
#include "eter/Lexer/LexerErrors.def"
#undef ETER_LEXER_ERROR
    }
    return "unknown";
  }
};

/// A variant type representing either a successfully lexed token or a lexer
/// error. This allows the lexer to report errors without aborting the lexing
/// process, enabling incremental re-lexing even in the presence of malformed
/// input.
using LexerItem = std::variant<Token, LexerError>;

/// A high-performance, incremental-ready lexer.
///
/// The lexer converts a raw character stream from a `SourceBuffer` into a
/// sequence of `LexerItem`s. It is designed to be highly stateless and operates
/// via zero-copy string references to support fast, incremental updates.
class Lexer {
public:
  Span makeTokenSpan(const char *TokStart) const;

  /// Checks if a character is a valid hexadecimal digit (0-9, a-f, A-F).
  static bool isHexDigit(char C);

  /// Determines whether a given identifier string is a reserved keyword.
  static bool isReservedKeyword(llvm::StringRef Str);

  /// Determines whether a given token kind is a reserved symbol.
  static bool isReservedSymbolKind(Token::Kind K);

  /// Lexes a Unicode escape sequence (e.g., \uXXXX or \UXXXXXXXX).
  /// \param EscapeStart Pointer to the start of the escape sequence (the '\').
  /// \param CurPtr Reference to the current pointer, advanced past the escape.
  /// \param BufferEnd Pointer to the end of the buffer to prevent overreads.
  /// \param LexerItems Output list to append errors if the escape is malformed.
  /// \param BufferStart Pointer to the start of the buffer for span
  /// calculation.
  /// \returns true if the escape was successfully parsed, false otherwise.
  static bool lexUnicodeEscape(const char *EscapeStart, const char *&CurPtr,
                               const char *BufferEnd,
                               std::vector<LexerItem> &LexerItems,
                               const char *BufferStart);

  /// Default constructor. Internal state pointers are initialized to nullptr
  /// and set during each `lex` call.
  Lexer() = default;

  /// Lexes the entire contents of the provided source buffer from start to
  /// finish.
  /// \param SourceBuffer The source buffer to lex.
  /// \returns A vector of lexer items extracted from the source buffer.
  [[nodiscard]]
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
  [[nodiscard]]
  std::vector<LexerItem> lex(SourceBuffer &SourceBuffer, Span Span);

private:
  /// Constructs a `LexerError` and appends it to the output items list.
  /// \param Items The output list to append the error to.
  /// \param Kind The specific category of the lexer error.
  /// \param Start Pointer to the start of the erroneous token/sequence.
  /// \param End Pointer to the end of the erroneous token/sequence.
  /// \param BufferStart Pointer to the start of the buffer for span
  /// calculation.
  static void pushLexerError(std::vector<LexerItem> &Items,
                             LexerError::Kind Kind, const char *Start,
                             const char *End, const char *BufferStart);

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
  [[nodiscard]]
  bool lexNumericLiteral(Token &Result, const char *TokStart);

  /// \param Result The token to populate with the lexed information.
  /// \param TokStart A pointer to the start of the token in the source buffer.
  /// \param LexerItems The output list to append lexer errors to.
  /// \returns true if the string was properly terminated, false otherwise.
  [[nodiscard]]
  bool lexStringLiteral(Token &Result, const char *TokStart,
                        std::vector<LexerItem> &LexerItems);

  /// \param Result The token to populate with the lexed information.
  /// \param TokStart A pointer to the start of the token in the source buffer.
  /// \param LexerItems The output list to append lexer errors to.
  /// \param CharCount The number of logical characters consumed.
  /// \returns true if the character literal was properly terminated, false
  /// otherwise.
  [[nodiscard]]
  bool lexCharacterLiteral(Token &Result, const char *TokStart,
                           std::vector<LexerItem> &LexerItems,
                           size_t &CharCount);

  /// \param Result The token to populate with the lexed information.
  /// \param TokStart A pointer to the start of the token in the source buffer.
  /// \param LexerItems The output list to append lexer errors to.
  /// \returns true if a comment was lexed, false if this is a division
  /// operator.
  [[nodiscard]]
  bool lexComment(Token &Result, const char *TokStart,
                  std::vector<LexerItem> &LexerItems);

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
