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

bool isHexDigit(char C) {
  return std::isxdigit(static_cast<unsigned char>(C)) != 0;
}

static void pushLexerError(std::vector<LexerItem> &Items,
                           LexerError::Kind Kind, const char *Start,
                           const char *End, const char *BufferStart) {
  Items.push_back(
      LexerError{Kind, eter::Span(Start - BufferStart, End - BufferStart)});
}

/// Lexes a specific byte range within the source buffer and returns a list of lexer items.
/// This is the core engine for incremental lexing.
std::vector<LexerItem> Lexer::lex(SourceBuffer &SourceBuffer, Span Span) {
  llvm::StringRef Buffer = SourceBuffer.getBuffer();
  std::vector<LexerItem> LexerItems;

  // Validate the span to prevent out-of-bounds memory access.
  if (Span.Start > Buffer.size() || Span.End > Buffer.size() ||
      Span.Start > Span.End) {
    return LexerItems;
  }

  // Pre-allocate memory to avoid multiple reallocations
  // A rough heuristic of 1 token per 8 characters
  LexerItems.reserve((Span.End - Span.Start) / 8 + 1);

  // Initialize the sliding window pointers for this lexing session.
  BufferStart = Buffer.data();
  CurPtr = BufferStart + Span.Start;
  BufferEnd = BufferStart + Span.End;

  // Main lexing loop: consume characters until we reach the end of the span.
  while (CurPtr < BufferEnd) {
    // Skip any irrelevant whitespace before parsing the next token.
    skipWhitespace();
    if (CurPtr >= BufferEnd) {
      break;
    }

    const char *TokStart = CurPtr;
    char C = *CurPtr++;
    unsigned char UC = static_cast<unsigned char>(C);

    if (C == '/' && CurPtr < BufferEnd) {
      if (*CurPtr == '/') {
        // Line comment (either doc comment or regular comment)
        if (CurPtr + 1 < BufferEnd && CurPtr[1] == '/' &&
            (CurPtr + 2 >= BufferEnd || CurPtr[2] != '/')) {
          // Doc comment
          CurPtr += 2; // Consume the other two slashes
          while (CurPtr < BufferEnd && *CurPtr != '\n' && *CurPtr != '\r') {
            CurPtr++;
          }
          LexerItems.push_back(
              Token(Token::Kind::doc_comment,
                    eter::Span(TokStart - BufferStart, CurPtr - BufferStart)));
          continue;
        } else {
          // Regular line comment
          CurPtr++; // Consume the second slash
          while (CurPtr < BufferEnd && *CurPtr != '\n' && *CurPtr != '\r') {
            CurPtr++;
          }
          LexerItems.push_back(
              Token(Token::Kind::comment,
                    eter::Span(TokStart - BufferStart, CurPtr - BufferStart)));
          continue;
        }
      } else if (*CurPtr == '*') {
        // Block comment
        CurPtr++; // Consume '*'
        int NestingDepth = 1;
        while (CurPtr < BufferEnd && NestingDepth > 0) {
          if (CurPtr + 1 < BufferEnd && *CurPtr == '/' && CurPtr[1] == '*') {
            NestingDepth++;
            CurPtr += 2;
          } else if (CurPtr + 1 < BufferEnd && *CurPtr == '*' && CurPtr[1] == '/') {
            NestingDepth--;
            CurPtr += 2;
          } else {
            CurPtr++;
          }
        }
        
        if (NestingDepth > 0) {
          LexerItems.push_back(LexerError{
              LexerError::Kind::UnterminatedBlockComment,
              eter::Span(TokStart - BufferStart, CurPtr - BufferStart)});
          // Optional: break or continue? Let's just return to stop lexing
          return LexerItems;
        }
        LexerItems.push_back(
            Token(Token::Kind::comment,
                  eter::Span(TokStart - BufferStart, CurPtr - BufferStart)));
        continue;
      }
    }

    // 1. Identifiers and Keywords
    // If the token starts with a letter or underscore, it's an identifier or keyword.
    if (std::isalpha(UC) || C == '_') {
      Token Result(Token::Kind::identifier, eter::Span(0, 0));
      lexIdentifier(Result, TokStart);
      LexerItems.push_back(Result);
      continue;
    }

    // 2. Numeric Literals
    // If the token starts with a digit, it's an integer or floating-point literal.
    if (std::isdigit(UC)) {
      Token Result(Token::Kind::integer_literal, eter::Span(0, 0));
      bool Valid = lexNumericLiteral(Result, TokStart);
      LexerItems.push_back(Result);
      if (!Valid) {
        LexerItems.push_back(LexerError{LexerError::Kind::InvalidNumericLiteral,
                                        Result.TokenSpan});
      }
      continue;
    }

    // 3. String Literals
    // If the token starts with a quote, parse it as a string literal.
    if (C == '"') {
      Token Result(Token::Kind::string_literal, eter::Span(0, 0));
      bool Terminated = lexStringLiteral(Result, TokStart, LexerItems);
      LexerItems.push_back(Result);
      if (!Terminated) {
        LexerItems.push_back(LexerError{
            LexerError::Kind::UnterminatedStringLiteral, Result.TokenSpan});
      }
      continue;
    }

    // 4. Character Literals
    // If the token starts with a single quote, parse it as a character literal.
    if (C == '\'') {
      Token Result(Token::Kind::char_literal, eter::Span(0, 0));
      size_t CharCount = 0;
      bool Terminated =
          lexCharacterLiteral(Result, TokStart, LexerItems, CharCount);
      LexerItems.push_back(Result);
      if (!Terminated) {
        LexerItems.push_back(LexerError{
            LexerError::Kind::UnterminatedCharLiteral, Result.TokenSpan});
      } else if (CharCount != 1) {
        LexerItems.push_back(LexerError{
            LexerError::Kind::InvalidCharLiteralLength, Result.TokenSpan});
      }
      continue;
    }

    // 5. C-Style Strings
    // If the token is @c", parse it as a C-style string literal.
    if (C == '@') {
      if (CurPtr < BufferEnd && *CurPtr == 'c' && CurPtr + 1 < BufferEnd &&
          CurPtr[1] == '"') {
        CurPtr += 2; // Consume 'c' and '"'
        Token Result(Token::Kind::c_string_literal, eter::Span(0, 0));
        bool Terminated = lexStringLiteral(Result, TokStart, LexerItems);
        Result.TokenKind = Token::Kind::c_string_literal; // override what lexStringLiteral sets
        LexerItems.push_back(Result);
        if (!Terminated) {
          LexerItems.push_back(LexerError{
              LexerError::Kind::UnterminatedStringLiteral, Result.TokenSpan});
        }
        continue;
      }
    }

    // 6. Symbols and Operators
    // Resolve single-character and multi-character operators.
    Token::Kind Kind = Token::Kind::unknown;

    switch (C) {
    // --- Punctuation and Grouping ---
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

    // --- Arithmetic Operators ---
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

    // --- Logical and Bitwise Operators ---
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

    // --- Relational and Assignment Operators ---
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

    // --- Object/Member Access ---
    case '.':
      Kind = Token::Kind::dot;
      break;
    }

    if (Kind != Token::Kind::unknown) {
      LexerItems.push_back(Token(
          Kind, eter::Span(TokStart - BufferStart, CurPtr - BufferStart)));
    } else {
      struct Span ErrorSpan(TokStart - BufferStart, CurPtr - BufferStart);
      LexerItems.push_back(
          LexerError{LexerError::Kind::InvalidCharacter, ErrorSpan});
      // Emit an unknown token to allow the parser to attempt error recovery.
      LexerItems.push_back(Token(Token::Kind::unknown, ErrorSpan));
    }
  }

  // If we lexed the entire buffer (not just a chunk), append an EOF token.
  // This simplifies parser logic by guaranteeing an explicit end marker.
  if (Span.Start == 0 && Span.End == Buffer.size()) {
    if (!LexerItems.empty()) {
      if (const auto *Tok = std::get_if<Token>(&LexerItems.back())) {
        if (Tok->TokenKind != Token::Kind::eof) {
          LexerItems.push_back(
              Token(Token::Kind::eof,
                    eter::Span(Buffer.size(), Buffer.size())));
        }
      } else {
        LexerItems.push_back(
            Token(Token::Kind::eof, eter::Span(Buffer.size(), Buffer.size())));
      }
    } else {
      LexerItems.push_back(
          Token(Token::Kind::eof, eter::Span(Buffer.size(), Buffer.size())));
    }
  }

  return LexerItems;
}

/// Advances the internal cursor past whitespace characters.
void Lexer::skipWhitespace() {
  while (CurPtr < BufferEnd) {
    char C = *CurPtr;
    if (C == ' ' || C == '\t' || C == '\n' || C == '\r') {
      CurPtr++;
    } else {
      break; // Not whitespace.
    }
  }
}

/// Lexes an identifier or keyword.
/// Assumes `CurPtr` is pointing to the start of the token.
void Lexer::lexIdentifier(Token &Result, const char *TokStart) {
  // Consume alphanumeric characters and underscores.
  while (CurPtr < BufferEnd) {
    unsigned char C = static_cast<unsigned char>(*CurPtr);
    if (std::isalnum(C) || *CurPtr == '_') {
      CurPtr++;
    } else {
      break;
    }
  }

  llvm::StringRef Str(TokStart, CurPtr - TokStart);

  // Use StringSwitch for O(1) keyword resolution via X-macros.
  // Defaults to a standard identifier if no keyword matches.
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
  if (TokStart[0] == '0' && CurPtr < BufferEnd &&
      (*CurPtr == 'x' || *CurPtr == 'X')) {
    CurPtr++;
    IsHex = true;
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

  if (!IsHex && CurPtr < BufferEnd && *CurPtr == '.') {
    // Check if the next character is a digit to differentiate a float literal
    // (e.g., 3.14) from a method call on an integer (e.g., 1.method()).
    if (CurPtr + 1 < BufferEnd &&
        std::isdigit(static_cast<unsigned char>(CurPtr[1]))) {
      CurPtr++; // Consume '.'
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
  return !(IsHex && HexDigits == 0);
}

static bool lexUnicodeEscape(const char *EscapeStart, const char *&CurPtr,
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

  bool HasDigits = CurPtr > DigitsStart;
  if (CurPtr >= BufferEnd) {
    pushLexerError(LexerItems, LexerError::Kind::InvalidUnicodeEscape,
                   EscapeStart, CurPtr, BufferStart);
    return true;
  }

  if (*CurPtr != '}') {
    pushLexerError(LexerItems, LexerError::Kind::InvalidUnicodeEscape,
                   EscapeStart, CurPtr, BufferStart);
    return true;
  }

  CurPtr++; // consume '}'
  if (!HasDigits) {
    pushLexerError(LexerItems, LexerError::Kind::InvalidUnicodeEscape,
                   EscapeStart, CurPtr, BufferStart);
  }
  return true;
}

/// Lexes a string literal enclosed in double quotes.
/// Handles basic escape sequences.
bool Lexer::lexStringLiteral(Token &Result, const char *TokStart,
                             std::vector<LexerItem> &LexerItems) {
  while (CurPtr < BufferEnd) {
    char C = *CurPtr++;
    if (C == '\\') {
      const char *EscapeStart = CurPtr - 1;
      if (CurPtr >= BufferEnd) {
        pushLexerError(LexerItems, LexerError::Kind::InvalidEscapeSequence,
                       EscapeStart, CurPtr, BufferStart);
        break;
      }

      if (lexUnicodeEscape(EscapeStart, CurPtr, BufferEnd, LexerItems,
                           BufferStart)) {
        continue;
      }

      char Next = *CurPtr;
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
        pushLexerError(LexerItems, LexerError::Kind::InvalidEscapeSequence,
                       EscapeStart, CurPtr, BufferStart);
        break;
      }
    } else if (C == '"') {
      // String successfully terminated.
      Result.TokenKind = Token::Kind::string_literal;
      Result.TokenSpan =
          eter::Span(TokStart - BufferStart, CurPtr - BufferStart);
      return true;
    } else if (C == '\n' || C == '\r') {
      // Unterminated string literal on this line.
      break;
    }
  }

  // If we reach here, the string is unterminated (hit EOF or a newline).
  // We emit it as a string literal anyway to facilitate error recovery
  // in the parser, even though it is malformed.
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
    char C = *CurPtr++;
    if (C == '\\') {
      const char *EscapeStart = CurPtr - 1;
      if (CurPtr >= BufferEnd) {
        pushLexerError(LexerItems, LexerError::Kind::InvalidEscapeSequence,
                       EscapeStart, CurPtr, BufferStart);
        break;
      }

      if (lexUnicodeEscape(EscapeStart, CurPtr, BufferEnd, LexerItems,
                           BufferStart)) {
        CharCount++;
        continue;
      }

      char Next = *CurPtr;
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
        pushLexerError(LexerItems, LexerError::Kind::InvalidEscapeSequence,
                       EscapeStart, CurPtr, BufferStart);
        break;
      }
    } else if (C == '\'') {
      // Character literal successfully terminated.
      Result.TokenKind = Token::Kind::char_literal;
      Result.TokenSpan =
          eter::Span(TokStart - BufferStart, CurPtr - BufferStart);
      return true;
    } else if (C == '\n' || C == '\r') {
      // Unterminated character literal on this line.
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

} // namespace eter::lexer
