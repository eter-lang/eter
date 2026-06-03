//===----------------------------------------------------------------------===//
//
// Part of the Eter Project, under the Apache License v2.0 with LLVM Exceptions.
// See /LICENSE for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "eter/Base/Debug.h"
#include "eter/Base/Span.h"
#include "eter/Base/StringInterner.h"
#include "eter/Lexer/Token.h"
#include "eter/Parser/NodePool.h"
#include "eter/Parser/Parser.h"
#include "eter/Parser/Regime.h"

#include <llvm/ADT/SmallVector.h>
#include <llvm/Support/ErrorHandling.h>
#include <llvm/Support/raw_ostream.h>

#define DEBUG_TYPE "parser-stmt"

namespace eter::parser {

NodeIndex Parser::parseStmt() {
  ETER_DEBUG(llvm::dbgs() << "[" DEBUG_TYPE "] parseStmt\n");

  using Kind = lexer::Token::Kind;

  const lexer::Token Tok = peekToken();

  switch (Tok.TokenKind) {
  case Kind::kw_let:
    return parseLetStmt();
  case Kind::kw_if:
    return parseIfExpr();
  case Kind::kw_for:
    return parseForStmt();
  case Kind::kw_while:
    return parseWhileStmt();
  case Kind::kw_match:
    return parseMatchExpr();
  case Kind::l_brace:
    return parseBlockExpr();
  case Kind::kw_ret:
    return parseRetStmt();
  // Expression statements — tokens that can start an expression followed by ';'
  case Kind::identifier:
  case Kind::integer_literal:
  case Kind::float_literal:
  case Kind::char_literal:
  case Kind::string_literal:
  case Kind::kw_true:
  case Kind::kw_false:
  case Kind::l_paren:
  case Kind::bang:
  case Kind::minus:
  case Kind::amp: {
    NodeIndex Expr = parseExpr(0);
    expect(Kind::semi, DiagID::ExpectedSemiAfterExpr);
    return Expr;
  }
  default:
    addError(Tok.TokenSpan, DiagID::ExpectedStmt);
    synchronize();
    return makeErrorNode(Tok.TokenSpan);
  }
}

NodeIndex Parser::parseLetStmt() {
  ETER_DEBUG(llvm::dbgs() << "[" DEBUG_TYPE "] parseLetStmt\n");

  using Kind = lexer::Token::Kind;

  const Span Start = expect(Kind::kw_let, DiagID::ExpectedLetKeyword).TokenSpan;

  const Regime LetRegime = parseRegime();

  if (LetRegime == Regime::None)
    addError(Stream.previous().TokenSpan, DiagID::ExpectedRegimeAfterLet);

  const InternedStr Name =
      expectAndIntern(Kind::identifier, DiagID::ExpectedLetName);

  expect(Kind::colon, DiagID::ExpectedColonAfterName);

  llvm::SmallVector<NodeIndex, 2> Children;
  Children.push_back(parseType());

  expect(Kind::eq, DiagID::ExpectedEqAfterType);

  Children.push_back(parseExpr());

  expect(Kind::semi, DiagID::ExpectedLetSemi);

  return Pool.alloc(NodeKind::LetStmt,
                    Span{Start.Start, Stream.previous().TokenSpan.End},
                    Children, NodePool::makePayload(Name, LetRegime));
}

NodeIndex Parser::parseRetStmt() {
  ETER_DEBUG(llvm::dbgs() << "[" DEBUG_TYPE "] parseRetStmt\n");
  llvm::report_fatal_error("TODO: implement Parser::parseRetStmt");
}

NodeIndex Parser::parseIfExpr() {
  ETER_DEBUG(llvm::dbgs() << "[" DEBUG_TYPE "] parseIfExpr\n");
  llvm::report_fatal_error("TODO: implement Parser::parseIfExpr");
}

NodeIndex Parser::parseForStmt() {
  ETER_DEBUG(llvm::dbgs() << "[" DEBUG_TYPE "] parseForStmt\n");
  llvm::report_fatal_error("TODO: implement Parser::parseForStmt");
}

NodeIndex Parser::parseWhileStmt() {
  ETER_DEBUG(llvm::dbgs() << "[" DEBUG_TYPE "] parseWhileStmt\n");
  llvm::report_fatal_error("TODO: implement Parser::parseWhileStmt");
}

NodeIndex Parser::parseMatchExpr() {
  ETER_DEBUG(llvm::dbgs() << "[" DEBUG_TYPE "] parseMatchExpr\n");
  llvm::report_fatal_error("TODO: implement Parser::parseMatchExpr");
}

NodeIndex Parser::parseBlockExpr() {
  ETER_DEBUG(llvm::dbgs() << "[" DEBUG_TYPE "] parseBlockExpr\n");
  using Kind = lexer::Token::Kind;

  const Span Start = expect(Kind::l_brace, DiagID::ExpectedBlockOpen).TokenSpan;

  llvm::SmallVector<NodeIndex, 8> Children;

  while (!check(Kind::r_brace) && !atEof()) {
    Children.push_back(parseStmt());
  }

  const Span End = expect(Kind::r_brace, DiagID::ExpectedBlockClose).TokenSpan;
  return Pool.alloc(NodeKind::BlockExpr, Span{Start.Start, End.End}, Children);
}

NodeIndex Parser::parseMatchArm() {
  ETER_DEBUG(llvm::dbgs() << "[" DEBUG_TYPE "] parseMatchArm\n");
  llvm::report_fatal_error("TODO: implement Parser::parseMatchArm");
}

} // namespace eter::parser
