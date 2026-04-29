//===----------------------------------------------------------------------===//
//
// Part of the Eter Project, under the Apache License v2.0 with LLVM Exceptions.
// See /LICENSE for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "eter/Lexer/Lexer.h"

#include <llvm/ADT/StringSwitch.h>

#include <cctype>

namespace eter::lexer {

void Lexer::pushLexerError(std::vector<LexerItem> &Items, LexerError::Kind Kind,
                           const char *Start, const char *End,
                           const char *BufferStart) {
  Items.push_back(
      LexerError{Kind, eter::Span(Start - BufferStart, End - BufferStart)});
}
bool Lexer::isHexDigit(char C) {
  return std::isxdigit(static_cast<unsigned char>(C)) != 0;
}
bool Lexer::isReservedKeyword(llvm::StringRef Str) {
  return llvm::StringSwitch<bool>(Str)
#define ETER_RESERVED_KEYWORD(X, Y) .Case(Y, true)
#include "eter/Lexer/TokenKinds.def"
      .Default(false);
}
bool Lexer::lexUnicodeEscape(const char *EscapeStart, const char *&CurPtr,
                             const char *BufferEnd,
                             std::vector<LexerItem> &LexerItems,
                             const char *BufferStart) {
  if (CurPtr >= BufferEnd || *CurPtr != 'u') {
    return false;
  }
  if (CurPtr + 1 >= BufferEnd || CurPtr[1] != '{') {
    return false;
  }

  CurPtr += 2; // consume 'u' and '{'
  const char *DigitsStart = CurPtr;
  while (CurPtr < BufferEnd && isHexDigit(*CurPtr)) {
    CurPtr++;
  }

  bool const HasDigits = CurPtr > DigitsStart;
  if (CurPtr >= BufferEnd) {
    Lexer::pushLexerError(LexerItems, LexerError::Kind::InvalidUnicodeEscape,
                          EscapeStart, CurPtr, BufferStart);
    return true;
  }

  if (*CurPtr != '}') {
    Lexer::pushLexerError(LexerItems, LexerError::Kind::InvalidUnicodeEscape,
                          EscapeStart, CurPtr, BufferStart);
    return true;
  }

  CurPtr++; // consume '}'
  if (!HasDigits) {
    Lexer::pushLexerError(LexerItems, LexerError::Kind::InvalidUnicodeEscape,
                          EscapeStart, CurPtr, BufferStart);
  }
  return true;
}

/// Lexes a specific byte range within the source buffer and returns a list of
/// lexer items. This is the core engine for incremental lexing.
std::vector<LexerItem> Lexer::lex(SourceBuffer &SourceBuffer, Span Span) {
  llvm::StringRef const Buffer = SourceBuffer.getBuffer();
  std::vector<LexerItem> LexerItems;

  if (Span.Start > Buffer.size() || Span.End > Buffer.size() ||
      Span.Start > Span.End) {
    return LexerItems;
  }

  // Pre-allocate memory to avoid multiple reallocations.
  // Heuristic: ~1 token per 8 characters
  LexerItems.reserve((Span.End - Span.Start) / 8 + 1);

  // Initialize the sliding window pointers for this lexing session.
  BufferStart = Buffer.data();
  CurPtr = BufferStart + Span.Start;
  BufferEnd = BufferStart + Span.End;

  // Main lexing loop: consume characters until we reach the end of the span.
  while (CurPtr < BufferEnd) {
    skipWhitespace();
    if (CurPtr >= BufferEnd) {
      break;
    }

    const char *TokStart = CurPtr;
    char const C = *CurPtr++;
    unsigned char const UC = static_cast<unsigned char>(C);

    // 0. Comment handling (doc comments, regular comments, block comments)
    if (C == '/') {

      Token Result(Token::Kind::comment, eter::Span(0, 0));
      if (lexComment(Result, TokStart, LexerItems)) {
        LexerItems.push_back(Result);
        continue;
      }
      // Fall through: not a comment, let the switch handle division operator
    }

    // 1. Identifiers and Keywords
    if (std::isalpha(UC) || C == '_') {

      Token Result(Token::Kind::identifier, eter::Span(0, 0));
      lexIdentifier(Result, TokStart);
      if (isReservedKeyword(
              Buffer.slice(Result.TokenSpan.Start, Result.TokenSpan.End))) {
        LexerItems.push_back(
            LexerError{LexerError::Kind::ReservedKeyword, Result.TokenSpan});
      } else {
        LexerItems.push_back(Result);
      }
      continue;
    }

    // 2. Numeric Literals
    if (std::isdigit(UC)) {

      Token Result(Token::Kind::integer_literal, eter::Span(0, 0));
      bool const Valid = lexNumericLiteral(Result, TokStart);
      if (Valid) {
        LexerItems.push_back(Result);
      } else {
        LexerItems.push_back(LexerError{LexerError::Kind::InvalidNumericLiteral,
                                        Result.TokenSpan});
      }
      continue;
    }

    // 3. String Literals
    if (C == '"') {

      Token Result(Token::Kind::string_literal, eter::Span(0, 0));
      bool const Terminated = lexStringLiteral(Result, TokStart, LexerItems);
      if (Terminated) {
        LexerItems.push_back(Result);
      } else {
        LexerItems.push_back(LexerError{
            LexerError::Kind::UnterminatedStringLiteral, Result.TokenSpan});
      }
      continue;
    }

    // 4. Character Literals
    if (C == '\'') {

      Token Result(Token::Kind::char_literal, eter::Span(0, 0));

      size_t CharCount = 0;
      bool const Terminated =
          lexCharacterLiteral(Result, TokStart, LexerItems, CharCount);
      if (Terminated && CharCount == 1) {
        LexerItems.push_back(Result);
      } else {
        if (!Terminated) {
          LexerItems.push_back(LexerError{
              LexerError::Kind::UnterminatedCharLiteral, Result.TokenSpan});
        } else if (CharCount == 0) {
          LexerItems.push_back(
              LexerError{LexerError::Kind::EmptyCharLiteral, Result.TokenSpan});
        } else {
          LexerItems.push_back(LexerError{
              LexerError::Kind::MultiCharCharLiteral, Result.TokenSpan});
        }
      }
      continue;
    }

    // 5. Symbols and Operators
    Token::Kind Kind = Token::Kind::unknown;

    switch (C) {
    //===----------------------------------------------------------------------===//
    // Punctuation and Grouping
    //===----------------------------------------------------------------------===//
    case '(':
      Kind = Token::Kind::l_paren;
      break;
    case ')':
      Kind = Token::Kind::r_paren;
      break;
    case '{':
      Kind = Token::Kind::l_brace;
      break;
    case '}':
      Kind = Token::Kind::r_brace;
      break;
    case '[':
      Kind = Token::Kind::l_square;
      break;
    case ']':
      Kind = Token::Kind::r_square;
      break;
    case ',':
      Kind = Token::Kind::comma;
      break;
    case ';':
      Kind = Token::Kind::semi;
      break;
    case ':':
      if (CurPtr < BufferEnd && *CurPtr == ':') {
        CurPtr++;
        Kind = Token::Kind::colon_colon;
      } else {
        Kind = Token::Kind::colon;
      }
      break;

    //===----------------------------------------------------------------------===//
    // Arithmetic Operators
    //===----------------------------------------------------------------------===//
    case '+':
      if (CurPtr < BufferEnd && *CurPtr == '+') {
        CurPtr++;
        Kind = Token::Kind::plus_plus;
      } else if (CurPtr < BufferEnd && *CurPtr == '=') {
        CurPtr++;
        Kind = Token::Kind::plus_eq;
      } else {
        Kind = Token::Kind::plus;
      }
      break;
    case '-':
      if (CurPtr < BufferEnd && *CurPtr == '-') {
        CurPtr++;
        Kind = Token::Kind::minus_minus;
      } else if (CurPtr < BufferEnd && *CurPtr == '=') {
        CurPtr++;
        Kind = Token::Kind::minus_eq;
      } else {
        Kind = Token::Kind::minus;
      }
      break;
    case '*':
      if (CurPtr < BufferEnd && *CurPtr == '=') {
        CurPtr++;
        Kind = Token::Kind::star_eq;
      } else {
        Kind = Token::Kind::star;
      }
      break;
    case '/':
      if (CurPtr < BufferEnd && *CurPtr == '=') {
        CurPtr++;
        Kind = Token::Kind::slash_eq;
      } else {
        Kind = Token::Kind::slash;
      }
      break;
    case '%':
      if (CurPtr < BufferEnd && *CurPtr == '=') {
        CurPtr++;
        Kind = Token::Kind::percent_eq;
      } else {
        Kind = Token::Kind::percent;
      }
      break;

    //===----------------------------------------------------------------------===//
    // Logical and Bitwise Operators
    //===----------------------------------------------------------------------===//
    case '&':
      if (CurPtr < BufferEnd && *CurPtr == '&') {
        CurPtr++;
        Kind = Token::Kind::amp_amp;
      } else if (CurPtr < BufferEnd && *CurPtr == '=') {
        CurPtr++;
        Kind = Token::Kind::amp_eq;
      } else {
        Kind = Token::Kind::amp;
      }
      break;
    case '|':
      if (CurPtr < BufferEnd && *CurPtr == '|') {
        CurPtr++;
        Kind = Token::Kind::pipe_pipe;
      } else if (CurPtr < BufferEnd && *CurPtr == '=') {
        CurPtr++;
        Kind = Token::Kind::pipe_eq;
      } else {
        Kind = Token::Kind::pipe;
      }
      break;
    case '^':
      if (CurPtr < BufferEnd && *CurPtr == '=') {
        CurPtr++;
        Kind = Token::Kind::caret_eq;
      } else {
        Kind = Token::Kind::caret;
      }
      break;

    //===----------------------------------------------------------------------===//
    // Relational and Assignment Operators
    //===----------------------------------------------------------------------===//
    case '<':
      if (CurPtr < BufferEnd && *CurPtr == '<') {
        CurPtr++;
        if (CurPtr < BufferEnd && *CurPtr == '=') {
          CurPtr++;
          Kind = Token::Kind::less_less_eq;
        } else {
          Kind = Token::Kind::less_less;
        }
      } else if (CurPtr < BufferEnd && *CurPtr == '=') {
        CurPtr++;
        Kind = Token::Kind::less_eq;
      } else {
        Kind = Token::Kind::less;
      }
      break;
    case '>':
      if (CurPtr < BufferEnd && *CurPtr == '>') {
        CurPtr++;
        if (CurPtr < BufferEnd && *CurPtr == '=') {
          CurPtr++;
          Kind = Token::Kind::greater_greater_eq;
        } else {
          Kind = Token::Kind::greater_greater;
        }
      } else if (CurPtr < BufferEnd && *CurPtr == '=') {
        CurPtr++;
        Kind = Token::Kind::greater_eq;
      } else {
        Kind = Token::Kind::greater;
      }
      break;
    case '=':
      if (CurPtr < BufferEnd && *CurPtr == '=') {
        CurPtr++;
        Kind = Token::Kind::eq_eq;
      } else if (CurPtr < BufferEnd && *CurPtr == '>') {
        CurPtr++;
        Kind = Token::Kind::fat_arrow;
      } else {
        Kind = Token::Kind::eq;
      }
      break;
    case '!':
      if (CurPtr < BufferEnd && *CurPtr == '=') {
        CurPtr++;
        Kind = Token::Kind::bang_eq;
      } else {
        Kind = Token::Kind::bang;
      }
      break;

    //===----------------------------------------------------------------------===//
    // Object/Member Access
    //===----------------------------------------------------------------------===//
    case '.':
      Kind = Token::Kind::dot;
      break;

    default:
      break;
    }

    if (Kind != Token::Kind::unknown) {
      LexerItems.push_back(Token(
          Kind, eter::Span(TokStart - BufferStart, CurPtr - BufferStart)));
    } else {
      // Emit error and unknown token for unrecognized characters
      const struct Span ErrorSpan(TokStart - BufferStart, CurPtr - BufferStart);
      LexerItems.push_back(
          LexerError{LexerError::Kind::InvalidCharacter, ErrorSpan});
      LexerItems.push_back(Token(Token::Kind::unknown, ErrorSpan));
    }
  }

  // Append EOF token if lexing the entire buffer
  if (Span.Start == 0 && Span.End == Buffer.size()) {
    LexerItems.push_back(
        Token(Token::Kind::eof, eter::Span(Buffer.size(), Buffer.size())));
  }

  return LexerItems;
}

/// Advances the internal cursor past whitespace characters.
/// Handles ASCII whitespace only (space, tab, newline, carriage return).
void Lexer::skipWhitespace() {
  while (CurPtr < BufferEnd) {
    char const C = *CurPtr;
    if (C == ' ' || C == '\t' || C == '\n' || C == '\r') {
      CurPtr++;
    } else {
      break;
    }
  }
}

/// Lexes an identifier or keyword.
/// Assumes `CurPtr` is pointing to the start of the token.
void Lexer::lexIdentifier(Token &Result, const char *TokStart) {
  while (CurPtr < BufferEnd) {
    unsigned char const C = static_cast<unsigned char>(*CurPtr);
    if (std::isalnum(C) || *CurPtr == '_') {
      CurPtr++;
    } else {
      break;
    }
  }

  llvm::StringRef const Str(TokStart, CurPtr - TokStart);

  Result.TokenKind = llvm::StringSwitch<Token::Kind>(Str)
#define ETER_KEYWORD(X, Y) .Case(Y, Token::Kind::kw_##X)
#include "eter/Lexer/TokenKinds.def"
                         .Default(Token::Kind::identifier);

  Result.TokenSpan = eter::Span(TokStart - BufferStart, CurPtr - BufferStart);
}

/// Lexes a numeric literal (integer or floating-point).
/// Assumes `CurPtr` is pointing to the first digit.
bool Lexer::lexNumericLiteral(Token &Result, const char *TokStart) {
  bool IsHex = false;
  size_t HexDigits = 0;
  bool HasLeadingUnderscore = false;
  if (TokStart[0] == '0' && CurPtr < BufferEnd &&
      (*CurPtr == 'x' || *CurPtr == 'X')) {
    CurPtr++;
    IsHex = true;
  } else if (TokStart[0] == '_') {
    HasLeadingUnderscore = true;
  }

  // Consume the integer part.
  while (CurPtr < BufferEnd) {
    if (IsHex && isHexDigit(*CurPtr)) {
      HexDigits++;
      CurPtr++;
    } else if (!IsHex && std::isdigit(static_cast<unsigned char>(*CurPtr))) {
      CurPtr++;
    } else if (*CurPtr == '_') {
      CurPtr++;
    } else {
      break;
    }
  }

  if (HasLeadingUnderscore && HexDigits == 0 && !IsHex) {
    // Handle single underscore or leading underscore without digits
    Result.TokenKind = Token::Kind::identifier;
    Result.TokenSpan = eter::Span(TokStart - BufferStart, CurPtr - BufferStart);
    return true;
  }

  // Float literal handling
  if (!IsHex && CurPtr < BufferEnd && *CurPtr == '.') {
    if (CurPtr + 1 < BufferEnd &&
        std::isdigit(static_cast<unsigned char>(CurPtr[1]))) {
      CurPtr++; // consume '.'
      while (CurPtr < BufferEnd) {
        if (std::isdigit(static_cast<unsigned char>(*CurPtr)) ||
            *CurPtr == '_') {
          CurPtr++;
        } else {
          break;
        }
      }

      // Optional 'f' suffix for floating point literals.
      if (CurPtr < BufferEnd && *CurPtr == 'f')
        CurPtr++;

      Result.TokenKind = Token::Kind::float_literal;
      Result.TokenSpan =
          eter::Span(TokStart - BufferStart, CurPtr - BufferStart);
      return true;
    }
  }

  // Handle optional 'f' suffix without a decimal point (e.g., 1f).
  if (!IsHex && CurPtr < BufferEnd && *CurPtr == 'f') {
    CurPtr++;
    Result.TokenKind = Token::Kind::float_literal;
    Result.TokenSpan = eter::Span(TokStart - BufferStart, CurPtr - BufferStart);
    return true;
  }

  Result.TokenKind = Token::Kind::integer_literal;
  Result.TokenSpan = eter::Span(TokStart - BufferStart, CurPtr - BufferStart);

  // Invalid: hex literal with no digits, or leading/trailing separators
  if (IsHex && HexDigits == 0) {
    return false;
  }

  // Check for trailing underscore without digits (e.g., 42_)
  if (CurPtr > TokStart && *(CurPtr - 1) == '_') {
    return false;
  }

  return true;
}

/// Lexes a string literal enclosed in double quotes.
/// Handles basic escape sequences.
bool Lexer::lexStringLiteral(Token &Result, const char *TokStart,
                             std::vector<LexerItem> &LexerItems) {
  while (CurPtr < BufferEnd) {
    char const C = *CurPtr++;
    if (C == '\\') {
      const char *EscapeStart = CurPtr - 1;
      if (CurPtr >= BufferEnd) {
        Lexer::pushLexerError(LexerItems,
                              LexerError::Kind::InvalidEscapeSequence,
                              EscapeStart, CurPtr, BufferStart);
        break;
      }

      if (lexUnicodeEscape(EscapeStart, CurPtr, BufferEnd, LexerItems,
                           BufferStart)) {
        continue;
      }

      char const Next = *CurPtr;
      switch (Next) {
      case 'n':
      case 't':
      case 'r':
      case '0':
      case '\\':
      case '"':
      case '\'':
        CurPtr++;
        break;
      default:
        CurPtr++;
        Lexer::pushLexerError(LexerItems,
                              LexerError::Kind::InvalidEscapeSequence,
                              EscapeStart, CurPtr, BufferStart);
        break;
      }
    } else if (C == '"') {
      Result.TokenKind = Token::Kind::string_literal;
      Result.TokenSpan =
          eter::Span(TokStart - BufferStart, CurPtr - BufferStart);
      return true;
    } else if (C == '\n' || C == '\r') {
      break;
    }
  }

  // Emit token even if unterminated for error recovery
  Result.TokenKind = Token::Kind::string_literal;
  Result.TokenSpan = eter::Span(TokStart - BufferStart, CurPtr - BufferStart);
  return false;
}

/// Lexes a character literal enclosed in single quotes.
/// Handles basic escape sequences.
bool Lexer::lexCharacterLiteral(Token &Result, const char *TokStart,
                                std::vector<LexerItem> &LexerItems,
                                size_t &CharCount) {
  while (CurPtr < BufferEnd) {
    char const C = *CurPtr++;
    if (C == '\\') {
      const char *EscapeStart = CurPtr - 1;
      if (CurPtr >= BufferEnd) {
        Lexer::pushLexerError(LexerItems,
                              LexerError::Kind::InvalidEscapeSequence,
                              EscapeStart, CurPtr, BufferStart);
        break;
      }

      if (lexUnicodeEscape(EscapeStart, CurPtr, BufferEnd, LexerItems,
                           BufferStart)) {
        CharCount++;
        continue;
      }

      char const Next = *CurPtr;
      switch (Next) {
      case 'n':
      case 't':
      case 'r':
      case '0':
      case '\\':
      case '"':
      case '\'':
        CurPtr++;
        CharCount++;
        break;
      default:
        CurPtr++;
        CharCount++;
        Lexer::pushLexerError(LexerItems,
                              LexerError::Kind::InvalidEscapeSequence,
                              EscapeStart, CurPtr, BufferStart);
        break;
      }
    } else if (C == '\'') {
      Result.TokenKind = Token::Kind::char_literal;
      Result.TokenSpan =
          eter::Span(TokStart - BufferStart, CurPtr - BufferStart);
      return true;
    } else if (C == '\n' || C == '\r') {
      break;
    } else {
      CharCount++;
    }
  }

  // Unterminated character literal
  Result.TokenKind = Token::Kind::char_literal;
  Result.TokenSpan = eter::Span(TokStart - BufferStart, CurPtr - BufferStart);
  return false;
}

/// Lexes a comment (line, doc, or block comment).
/// Assumes `CurPtr` is pointing to the second '/' character.
/// Returns true if a comment was successfully lexed, false if this is
/// a division operator (not a comment).
bool Lexer::lexComment(Token &Result, const char *TokStart,
                       std::vector<LexerItem> &LexerItems) {
  if (CurPtr >= BufferEnd || *CurPtr != '/') {
    // Check if it's a block comment
    if (CurPtr < BufferEnd && *CurPtr == '*') {
      CurPtr++; // consume '*'
      int NestingDepth = 1;
      while (CurPtr < BufferEnd && NestingDepth > 0) {
        if (CurPtr + 1 < BufferEnd && *CurPtr == '/' && CurPtr[1] == '*') {
          NestingDepth++;
          CurPtr += 2;
        } else if (CurPtr + 1 < BufferEnd && *CurPtr == '*' &&
                   CurPtr[1] == '/') {
          NestingDepth--;
          CurPtr += 2;
        } else {
          CurPtr++;
        }
      }

      if (NestingDepth > 0) {
        Lexer::pushLexerError(LexerItems,
                              LexerError::Kind::UnterminatedBlockComment,
                              TokStart, CurPtr, BufferStart);
      }

      Result.TokenKind = Token::Kind::comment;
      Result.TokenSpan =
          eter::Span(TokStart - BufferStart, CurPtr - BufferStart);
      return true;
    }
    return false;
  }

  // Line comment: check for doc comment (///)
  if (CurPtr + 1 < BufferEnd && CurPtr[1] == '/' &&
      (CurPtr + 2 >= BufferEnd || CurPtr[2] != '/')) {
    // Doc comment (/// but not ////)
    CurPtr += 2; // consume second and third slashes
    while (CurPtr < BufferEnd && *CurPtr != '\n' && *CurPtr != '\r') {
      CurPtr++;
    }
    Result.TokenKind = Token::Kind::doc_comment;
  } else {
    // Regular line comment (//)
    CurPtr++; // consume second slash
    while (CurPtr < BufferEnd && *CurPtr != '\n' && *CurPtr != '\r') {
      CurPtr++;
    }
    Result.TokenKind = Token::Kind::comment;
  }

  Result.TokenSpan = eter::Span(TokStart - BufferStart, CurPtr - BufferStart);
  return true;
}

} // namespace eter::lexer
