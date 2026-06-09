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

  NodeIndex LHS = parsePrefixExpr();

  while (true) {
    LHS = parsePostfixOrCallExpr(LHS);

    const auto [LeftBP, RightBP] = infixBindingPower(peek());
    if (LeftBP < MinBP)
      break;

    const lexer::Token Op = advance();
    const NodeIndex RHS = parseExpr(RightBP);

    if (Pool.kindOf(RHS) == NodeKind::Error)
      return RHS;

    LHS = Pool.alloc(
        NodeKind::BinaryExpr,
        Span{Pool.spanOf(LHS).Start, Pool.spanOf(RHS).End}, {LHS, RHS},
        NodePool::makeOpPayload(static_cast<uint16_t>(Op.TokenKind)));
  }

  return LHS;
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
  // NOTE: Add case Kind::l_brace: return parseBlockExpr(); to allow blocks
  // in expression position (e.g. let x = { 42 }).
  default: {
    // Don't consume structural boundaries (sync tokens): leaving them in the
    // stream lets the enclosing block / statement list recover to the next `;`
    // or `}` without a cascade of misleading errors.
    const Kind K = Tok.TokenKind;
    const bool IsSync = K == Kind::semi || K == Kind::r_brace ||
                        K == Kind::r_paren || K == Kind::r_square ||
                        K == Kind::comma;
    if (!IsSync)
      advance();
    addError(Tok.TokenSpan, DiagID::ExpectedExpr);
    return makeErrorNode(Tok.TokenSpan);
  }
  }
}

NodeIndex Parser::parsePostfixOrCallExpr(NodeIndex Lhs) {
  ETER_DEBUG(llvm::dbgs() << "[" DEBUG_TYPE "] parsePostfixOrCallExpr\n");

  using Kind = lexer::Token::Kind;

  while (true) {
    switch (peek()) {
    case Kind::l_paren: {
      const NodeIndex Args = parseArgList();
      Lhs = Pool.alloc(NodeKind::CallExpr,
                       Span{Pool.spanOf(Lhs).Start, Pool.spanOf(Args).End},
                       {Lhs, Args});
      continue;
    }
    case Kind::dot: {
      advance();
      const InternedStr Field =
          expectAndIntern(Kind::identifier, DiagID::ExpectedFieldName);
      Lhs = Pool.alloc(
          NodeKind::FieldExpr,
          Span{Pool.spanOf(Lhs).Start, Stream.previous().TokenSpan.End}, {Lhs},
          NodePool::makePayload(Field, Regime::None));
      continue;
    }
    case Kind::l_square: {
      advance();
      const NodeIndex Index = parseExpr();
      const Span Close =
          expect(Kind::r_square, DiagID::ExpectedRSquare).TokenSpan;
      Lhs = Pool.alloc(NodeKind::IndexExpr,
                       Span{Pool.spanOf(Lhs).Start, Close.End}, {Lhs, Index});
      continue;
    }
    case Kind::question: {
      advance();
      Lhs = Pool.alloc(
          NodeKind::PropagateExpr,
          Span{Pool.spanOf(Lhs).Start, Stream.previous().TokenSpan.End}, {Lhs});
      continue;
    }
    default:
      return Lhs;
    }
  }
}

NodeIndex Parser::parseArgList() {
  ETER_DEBUG(llvm::dbgs() << "[" DEBUG_TYPE "] parseArgList\n");

  using Kind = lexer::Token::Kind;

  const Span Start =
      expect(Kind::l_paren, DiagID::ExpectedArgListOpen).TokenSpan;

  llvm::SmallVector<NodeIndex, 8> Args;
  if (!check(Kind::r_paren)) {
    Args.push_back(parseExpr());
    while (consume(Kind::comma))
      Args.push_back(parseExpr());
  }

  const Span End =
      expect(Kind::r_paren, DiagID::ExpectedArgListClose).TokenSpan;

  return Pool.alloc(NodeKind::ArgList, Span{Start.Start, End.End}, Args);
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
