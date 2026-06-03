//===----------------------------------------------------------------------===//
//
// Part of the Eter Project, under the Apache License v2.0 with LLVM Exceptions.
// See /LICENSE for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "eter/Base/Debug.h"
#include "eter/Base/StringInterner.h"
#include "eter/Lexer/Token.h"
#include "eter/Parser/NodePool.h"
#include "eter/Parser/Parser.h"
#include "eter/Parser/Regime.h"

#include <llvm/ADT/StringRef.h>
#include <llvm/Support/ErrorHandling.h>
#include <llvm/Support/raw_ostream.h>

#include <cstdint>

#define DEBUG_TYPE "parser-expr"

namespace eter::parser {

NodeIndex Parser::parseExpr(int MinBP) {
  ETER_DEBUG(llvm::dbgs() << "[" DEBUG_TYPE "] parseExpr minBP=" << MinBP
                          << "\n");

  NodeIndex Lhs = parsePrefixExpr();

  while (true) {
    Lhs = parsePostfixOrCallExpr(Lhs);

    const auto [LeftBP, RightBP] = infixBindingPower(peek());
    if (LeftBP < MinBP)
      break;

    const lexer::Token Op = advance();
    const NodeIndex Rhs = parseExpr(RightBP);

    Lhs = Pool.alloc(
        NodeKind::BinaryExpr,
        Span{Pool.spanOf(Lhs).Start, Pool.spanOf(Rhs).End}, {Lhs, Rhs},
        NodePool::makeOpPayload(static_cast<uint16_t>(Op.TokenKind)));
  }

  return Lhs;
}

NodeIndex Parser::parseLitExpr(const lexer::Token &Tok) {
  return Pool.allocLeaf(
      NodeKind::LitExpr, Tok.TokenSpan,
      NodePool::makePayload(Interner.intern(textOf(Tok.TokenSpan)),
                            Regime::None));
}

NodeIndex Parser::parsePrefixExpr() {
  ETER_DEBUG(llvm::dbgs() << "[" DEBUG_TYPE "] parsePrefixExpr\n");

  using Kind = lexer::Token::Kind;

  const lexer::Token Tok = Stream.peekToken();

  switch (Tok.TokenKind) {
  case Kind::integer_literal:
  case Kind::float_literal:
  case Kind::char_literal:
  case Kind::string_literal:
  case Kind::kw_true:
  case Kind::kw_false:
    advance();
    return parseLitExpr(Tok);
  case Kind::identifier: {
    advance();
    return Pool.allocLeaf(NodeKind::IdentExpr, Tok.TokenSpan,
                          Interner.intern(textOf(Tok.TokenSpan)));
  }
  case Kind::l_paren: {
    advance();
    NodeIndex Inner = parseExpr(0);
    expect(Kind::r_paren, DiagID::ExpectedParenExprClose);
    return Inner;
  }
  case Kind::bang:
  case Kind::minus:
  case Kind::amp: {
    const int RhsBP = prefixBindingPower(Tok.TokenKind);
    advance();
    const NodeIndex Operand = parseExpr(RhsBP);
    return Pool.alloc(
        NodeKind::UnaryExpr,
        Span{Tok.TokenSpan.Start, Pool.spanOf(Operand).End}, {Operand},
        NodePool::makeOpPayload(static_cast<uint16_t>(Tok.TokenKind)));
  }
  default:
    advance();
    addError(Tok.TokenSpan, DiagID::ExpectedExpr);
    return makeErrorNode(Tok.TokenSpan);
  }
}

NodeIndex Parser::parsePostfixOrCallExpr(NodeIndex Lhs) {
  ETER_DEBUG(llvm::dbgs() << "[" DEBUG_TYPE "] parsePostfixOrCallExpr\n");
  // TODO: handle .field, (args), [index], ?
  return Lhs;
}

NodeIndex Parser::parseArgList() {
  ETER_DEBUG(llvm::dbgs() << "[" DEBUG_TYPE "] parseArgList\n");
  llvm::report_fatal_error("TODO: implement Parser::parseArgList");
}

std::pair<int, int> Parser::infixBindingPower(lexer::Token::Kind K) {
  // Binding-power table for the Pratt parser.
  // {left_bp, right_bp}: left_bp < right_bp → left-associative,
  //                       left_bp > right_bp → right-associative.
  // Postfix operators (. [ ( ?) and path separator :: are NOT here,
  // they are handled by parsePostfixOrCallExpr.
  using Kind = lexer::Token::Kind;
  switch (K) {
  // Assignment, right-associative
  case Kind::eq:
  case Kind::plus_eq:
  case Kind::minus_eq:
  case Kind::star_eq:
  case Kind::slash_eq:
  case Kind::percent_eq:
  case Kind::amp_eq:
  case Kind::pipe_eq:
  case Kind::caret_eq:
  case Kind::less_less_eq:
  case Kind::greater_greater_eq:
    return {10, 9};
  case Kind::pipe_pipe:
    return {20, 21}; // ||
  case Kind::amp_amp:
    return {30, 31}; // &&
  case Kind::eq_eq:
  case Kind::bang_eq:
  case Kind::less:
  case Kind::greater:
  case Kind::less_eq:
  case Kind::greater_eq:
    return {40, 41}; // comparisons (semantic pass enforces non-assoc)
  case Kind::pipe:
    return {50, 51}; // |
  case Kind::caret:
    return {60, 61}; // ^
  case Kind::amp:
    return {70, 71}; // & (bitwise AND in infix pos)
  case Kind::less_less:
  case Kind::greater_greater:
    return {80, 81}; // << >>
  case Kind::plus:
  case Kind::minus:
    return {90, 91}; // + -
  case Kind::star:
  case Kind::slash:
  case Kind::percent:
    return {100, 101}; // * / %
  default:
    return {-1, -1};
  }
}

int Parser::prefixBindingPower(lexer::Token::Kind K) {
  // Prefix operators bind tighter than all binary operators (right_bp = 110).
  // & in prefix position means "take a projection of a mut value".
  using Kind = lexer::Token::Kind;
  switch (K) {
  case Kind::bang:
  case Kind::minus:
  case Kind::amp:
    return 110;
  default:
    return -1;
  }
}

} // namespace eter::parser
