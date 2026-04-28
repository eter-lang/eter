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

/// Lexes a specific byte range within the source buffer and returns a list of tokens.
/// This is the core engine for incremental lexing.
std::vector<Token> Lexer::lex(SourceBuffer &SourceBuffer, Span Span) {
  llvm::StringRef Buffer = SourceBuffer.getBuffer();
  std::vector<Token> LexerItems;

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
    // Skip any irrelevant whitespace or comments before parsing the next token.
    skipWhitespaceAndComments();
    if (CurPtr >= BufferEnd) {
      break;
    }

    const char *TokStart = CurPtr;
    char C = *CurPtr++;

    // 1. Identifiers and Keywords
    // If the token starts with a letter or underscore, it's an identifier or keyword.
    if (std::isalpha(C) || C == '_') {
      Token Result(Token::Kind::identifier, eter::Span(0, 0));
      lexIdentifier(Result, TokStart);
      LexerItems.push_back(Result);
      continue;
    }

    // 2. Numeric Literals
    // If the token starts with a digit, it's an integer or floating-point literal.
    if (std::isdigit(C)) {
      Token Result(Token::Kind::integer_literal, eter::Span(0, 0));
      lexNumericLiteral(Result, TokStart);
      LexerItems.push_back(Result);
      continue;
    }

    // 3. String Literals
    // If the token starts with a quote, parse it as a string literal.
    if (C == '"') {
      Token Result(Token::Kind::string_literal, eter::Span(0, 0));
      lexStringLiteral(Result, TokStart);
      LexerItems.push_back(Result);
      continue;
    }

    // 4. Character Literals
    // If the token starts with a single quote, parse it as a character literal.
    if (C == '\'') {
      Token Result(Token::Kind::char_literal, eter::Span(0, 0));
      lexCharacterLiteral(Result, TokStart);
      LexerItems.push_back(Result);
      continue;
    }

    // 5. C-Style Strings
    // If the token is @c", parse it as a C-style string literal.
    if (C == '@') {
      if (CurPtr < BufferEnd && *CurPtr == 'c' && CurPtr + 1 < BufferEnd && CurPtr[1] == '"') {
        CurPtr += 2; // Consume 'c' and '"'
        Token Result(Token::Kind::c_string_literal, eter::Span(0, 0));
        lexStringLiteral(Result, TokStart);
        Result.TokenKind = Token::Kind::c_string_literal; // override what lexStringLiteral sets
        LexerItems.push_back(Result);
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
      // Emit an unknown token to allow the parser to attempt error recovery.
      LexerItems.push_back(
          Token(Token::Kind::unknown,
                eter::Span(TokStart - BufferStart, CurPtr - BufferStart)));
    }
  }

  // If we lexed the entire buffer (not just a chunk), append an EOF token.
  // This simplifies parser logic by guaranteeing an explicit end marker.
  if (Span.Start == 0 && Span.End == Buffer.size()) {
    if (!LexerItems.empty()) {
      if (LexerItems.back().TokenKind != Token::Kind::eof) {
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

/// Advances the internal cursor past whitespace characters and comments.
/// Handles both line comments (//) and nested block comments (/* */).
void Lexer::skipWhitespaceAndComments() {
  while (CurPtr < BufferEnd) {
    char C = *CurPtr;
    if (C == ' ' || C == '\t' || C == '\n' || C == '\r') {
      CurPtr++;
    } else if (C == '/' && CurPtr + 1 < BufferEnd) {
      if (CurPtr[1] == '/') {
        // Single-line comment: skip until the end of the line.
        CurPtr += 2;
        while (CurPtr < BufferEnd && *CurPtr != '\n' && *CurPtr != '\r') {
          CurPtr++;
        }
      } else if (CurPtr[1] == '*') {
        // Multi-line block comment: support nesting to adhere to language spec.
        CurPtr += 2;
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
      } else {
        break; // It's a division operator, not a comment.
      }
    } else {
      break; // Not whitespace and not a comment.
    }
  }
}

/// Lexes an identifier or keyword.
/// Assumes `CurPtr` is pointing to the start of the token.
void Lexer::lexIdentifier(Token &Result, const char *TokStart) {
  // Consume alphanumeric characters and underscores.
  while (CurPtr < BufferEnd && (std::isalnum(*CurPtr) || *CurPtr == '_')) {
    CurPtr++;
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
void Lexer::lexNumericLiteral(Token &Result, const char *TokStart) {
  bool IsHex = false;
  if (TokStart[0] == '0' && CurPtr < BufferEnd && (*CurPtr == 'x' || *CurPtr == 'X')) {
    CurPtr++;
    IsHex = true;
  }

  // Consume the integer part.
  while (CurPtr < BufferEnd) {
    if (IsHex && std::isxdigit(*CurPtr)) {
      CurPtr++;
    } else if (!IsHex && std::isdigit(*CurPtr)) {
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
    if (CurPtr + 1 < BufferEnd && std::isdigit(CurPtr[1])) {
      CurPtr++; // Consume '.'
      while (CurPtr < BufferEnd) {
        if (std::isdigit(*CurPtr) || *CurPtr == '_') {
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
      return;
    }
  }

  // Handle optional 'f' suffix without a decimal point (e.g., 1f).
  if (!IsHex && CurPtr < BufferEnd && *CurPtr == 'f') {
    CurPtr++;
    Result.TokenKind = Token::Kind::float_literal;
    Result.TokenSpan = eter::Span(TokStart - BufferStart, CurPtr - BufferStart);
    return;
  }

  Result.TokenKind = Token::Kind::integer_literal;
  Result.TokenSpan = eter::Span(TokStart - BufferStart, CurPtr - BufferStart);
}

/// Lexes a string literal enclosed in double quotes.
/// Handles basic escape sequences.
void Lexer::lexStringLiteral(Token &Result, const char *TokStart) {
  while (CurPtr < BufferEnd) {
    char C = *CurPtr++;
    if (C == '\\') {
      // Skip the escaped character (e.g., \n, \", \\).
      if (CurPtr < BufferEnd) {
        CurPtr++;
      }
    } else if (C == '"') {
      // String successfully terminated.
      Result.TokenKind = Token::Kind::string_literal;
      Result.TokenSpan =
          eter::Span(TokStart - BufferStart, CurPtr - BufferStart);
      return;
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
}

/// Lexes a character literal enclosed in single quotes.
/// Handles basic escape sequences.
void Lexer::lexCharacterLiteral(Token &Result, const char *TokStart) {
  while (CurPtr < BufferEnd) {
    char C = *CurPtr++;
    if (C == '\\') {
      // Skip the escaped character.
      if (CurPtr < BufferEnd) {
        CurPtr++;
      }
    } else if (C == '\'') {
      // Character literal successfully terminated.
      Result.TokenKind = Token::Kind::char_literal;
      Result.TokenSpan =
          eter::Span(TokStart - BufferStart, CurPtr - BufferStart);
      return;
    } else if (C == '\n' || C == '\r') {
      // Unterminated character literal on this line.
      break;
    }
  }

  // Unterminated character literal
  Result.TokenKind = Token::Kind::char_literal;
  Result.TokenSpan = eter::Span(TokStart - BufferStart, CurPtr - BufferStart);
}

} // namespace eter::lexer
