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

std::vector<Token> Lexer::lex(SourceBuffer &SourceBuffer, Span Span) {
  llvm::StringRef Buffer = SourceBuffer.getBuffer();
  std::vector<Token> LexerItems;

  if (Span.Start >= Buffer.size() || Span.End > Buffer.size() ||
      Span.Start > Span.End) {
    return LexerItems;
  }

  BufferStart = Buffer.data();
  CurPtr = BufferStart + Span.Start;
  BufferEnd = BufferStart + Span.End;

  while (CurPtr < BufferEnd) {
    skipWhitespaceAndComments();
    if (CurPtr >= BufferEnd) {
      break;
    }

    const char *TokStart = CurPtr;
    char C = *CurPtr++;

    if (std::isalpha(C) || C == '_') {
      Token Result(Token::Kind::identifier, eter::Span(0, 0));
      lexIdentifier(Result, TokStart);
      LexerItems.push_back(Result);
      continue;
    }

    if (std::isdigit(C)) {
      Token Result(Token::Kind::integer_literal, eter::Span(0, 0));
      lexNumericLiteral(Result, TokStart);
      LexerItems.push_back(Result);
      continue;
    }

    if (C == '"') {
      Token Result(Token::Kind::string_literal, eter::Span(0, 0));
      lexStringLiteral(Result, TokStart);
      LexerItems.push_back(Result);
      continue;
    }

    Token::Kind Kind = Token::Kind::unknown;

    switch (C) {
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
      Kind = Token::Kind::percent;
      break;
    case '&':
      if (CurPtr < BufferEnd && *CurPtr == '&') {
        CurPtr++;
        Kind = Token::Kind::amp_amp;
      } else {
        Kind = Token::Kind::amp;
      }
      break;
    case '|':
      if (CurPtr < BufferEnd && *CurPtr == '|') {
        CurPtr++;
        Kind = Token::Kind::pipe_pipe;
      } else {
        Kind = Token::Kind::pipe;
      }
      break;
    case '^':
      Kind = Token::Kind::caret;
      break;
    case '<':
      if (CurPtr < BufferEnd && *CurPtr == '<') {
        CurPtr++;
        Kind = Token::Kind::less_less;
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
        Kind = Token::Kind::greater_greater;
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
    case '.':
      Kind = Token::Kind::dot;
      break;
    }

    if (Kind != Token::Kind::unknown) {
      LexerItems.push_back(Token(
          Kind, eter::Span(TokStart - BufferStart, CurPtr - BufferStart)));
    } else {
      // Emit an unknown token
      LexerItems.push_back(
          Token(Token::Kind::unknown,
                eter::Span(TokStart - BufferStart, CurPtr - BufferStart)));
    }
  }

  if (Span.Start == 0 && Span.End == Buffer.size()) {
    // Append EOF token if we parsed the whole buffer
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

void Lexer::skipWhitespaceAndComments() {
  while (CurPtr < BufferEnd) {
    char C = *CurPtr;
    if (C == ' ' || C == '\t' || C == '\n' || C == '\r') {
      CurPtr++;
    } else if (C == '/' && CurPtr + 1 < BufferEnd) {
      if (CurPtr[1] == '/') {
        CurPtr += 2;
        while (CurPtr < BufferEnd && *CurPtr != '\n' && *CurPtr != '\r') {
          CurPtr++;
        }
      } else if (CurPtr[1] == '*') {
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
        break;
      }
    } else {
      break;
    }
  }
}

void Lexer::lexIdentifier(Token &Result, const char *TokStart) {
  while (CurPtr < BufferEnd && (std::isalnum(*CurPtr) || *CurPtr == '_')) {
    CurPtr++;
  }

  llvm::StringRef Str(TokStart, CurPtr - TokStart);

  Result.TokenKind = llvm::StringSwitch<Token::Kind>(Str)
#define ETER_KEYWORD(X, Y) .Case(Y, Token::Kind::kw_##X)
#include "eter/Lexer/TokenKinds.def"
                         .Default(Token::Kind::identifier);

  Result.TokenSpan = eter::Span(TokStart - BufferStart, CurPtr - BufferStart);
}

void Lexer::lexNumericLiteral(Token &Result, const char *TokStart) {
  while (CurPtr < BufferEnd && std::isdigit(*CurPtr))
    CurPtr++;

  if (CurPtr < BufferEnd && *CurPtr == '.') {
    // Check if the next char is a digit to differentiate from a method call
    // (e.g. `1.method()`)
    if (CurPtr + 1 < BufferEnd && std::isdigit(CurPtr[1])) {
      CurPtr++; // Consume '.'
      while (CurPtr < BufferEnd && std::isdigit(*CurPtr))
        CurPtr++;
      if (CurPtr < BufferEnd && *CurPtr == 'f')
        CurPtr++;
      Result.TokenKind = Token::Kind::float_literal;
      Result.TokenSpan =
          eter::Span(TokStart - BufferStart, CurPtr - BufferStart);
      return;
    }
  }
  if (CurPtr < BufferEnd && *CurPtr == 'f') {
    CurPtr++;
    Result.TokenKind = Token::Kind::float_literal;
    Result.TokenSpan = eter::Span(TokStart - BufferStart, CurPtr - BufferStart);
  }

  Result.TokenKind = Token::Kind::integer_literal;
  Result.TokenSpan = eter::Span(TokStart - BufferStart, CurPtr - BufferStart);
}

void Lexer::lexStringLiteral(Token &Result, const char *TokStart) {
  while (CurPtr < BufferEnd) {
    char C = *CurPtr++;
    if (C == '\\') {
      // Skip escaped character
      if (CurPtr < BufferEnd) {
        CurPtr++;
      }
    } else if (C == '"') {
      // String successfully terminated
      Result.TokenKind = Token::Kind::string_literal;
      Result.TokenSpan =
          eter::Span(TokStart - BufferStart, CurPtr - BufferStart);
      return;
    } else if (C == '\n' || C == '\r') {
      // Unterminated string literal on this line
      break;
    }
  }

  // If we reach here, the string is unterminated (hit EOF or newline)
  // Emit string literal anyway for recovery
  Result.TokenKind = Token::Kind::string_literal;
  Result.TokenSpan = eter::Span(TokStart - BufferStart, CurPtr - BufferStart);
}

} // namespace eter::lexer
