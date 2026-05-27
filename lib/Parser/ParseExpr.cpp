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
#include <iostream>
#include <ostream>

#define DEBUG_TYPE "parser-expr"

namespace eter::parser {

NodeIndex Parser::parseExpr([[maybe_unused]] int MinBP) {
  ETER_DEBUG(llvm::dbgs() << "[" DEBUG_TYPE "] parseExpr minBP=" << MinBP
                          << "\n");
  // llvm::report_fatal_error("TODO: implement Parser::parseExpr");

  std::vector<NodeIndex> Childrens;
  uint32_t Oper;
  const lexer::Token Start = Stream.peekToken();

  while (!Stream.check(lexer::Token::Kind::semi)) {
    const lexer::Token Tok = Stream.advance();

    if (Tok.TokenKind == lexer::Token::Kind::plus) {
      Oper = NodePool::makeOpPayload(static_cast<uint16_t>(Tok.TokenKind));

    } else {
      const NodeIndex Operand = Pool.allocLeaf(
          NodeKind::LitExpr, Tok.TokenSpan,
          NodePool::makePayload(Interner.intern(textOf(Tok.TokenSpan)),
                                Regime::None));
      Childrens.push_back(Operand);
    }
  }

  Stream.advance();

  if (Childrens.size() == 1) { // Es. " let <name> : i32 = 10"
    return Childrens[0];
  }

  return Pool.alloc(
      NodeKind::BinaryExpr,
      Span{Start.TokenSpan.Start, Stream.previous().TokenSpan.End}, Childrens,
      Oper);
}

NodeIndex Parser::parsePrefixExpr() {
  ETER_DEBUG(llvm::dbgs() << "[" DEBUG_TYPE "] parsePrefixExpr\n");
  llvm::report_fatal_error("TODO: implement Parser::parsePrefixExpr");
}

NodeIndex Parser::parsePostfixOrCallExpr(NodeIndex Lhs) {
  ETER_DEBUG(llvm::dbgs() << "[" DEBUG_TYPE "] parsePostfixOrCallExpr\n");
  (void)Lhs;
  llvm::report_fatal_error("TODO: implement Parser::parsePostfixOrCallExpr");
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
