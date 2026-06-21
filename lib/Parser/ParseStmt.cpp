//===----------------------------------------------------------------------===//
//
// Part of the Eter Project, under the Apache License v2.0 with LLVM Exceptions.
// See /LICENSE for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "eter/Base/Debug.h"
#include "eter/Base/Span.h"
#include "eter/Lexer/Token.h"
#include "eter/Parser/NodePool.h"
#include "eter/Parser/Parser.h"
#include "eter/Parser/Regime.h"

#include <llvm/ADT/SmallVector.h>
#include <llvm/Support/ErrorHandling.h>
#include <llvm/Support/SaveAndRestore.h>
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
  case Kind::l_brace:
    return parseBlockExpr();
  case Kind::kw_ret:
    return parseRetStmt();
  case Kind::kw_break:
    return parseBreakStmt();
  case Kind::kw_continue:
    return parseContinueStmt();
  case Kind::kw_unsafe:
    return parseUnsafeBlock();
  // Expression statements — tokens that can start an expression followed by ';'
  case Kind::identifier:
  case Kind::integer_literal:
  case Kind::float_literal:
  case Kind::char_literal:
  case Kind::string_literal:
  case Kind::kw_true:
  case Kind::kw_false:
  case Kind::l_paren:
  case Kind::l_square:
  case Kind::bang:
  case Kind::minus:
  case Kind::amp:
  case Kind::star:
  case Kind::kw_tensor:
  case Kind::kw_match: {
    NodeIndex Expr = parseExpr(0);
    // If expression parsing failed, resync to the next statement boundary
    // before continuing. Otherwise still emit a missing-`;` diagnostic if
    // needed and let the surrounding block handle the resulting position.
    if (Pool.kindOf(Expr) == NodeKind::Error) {
      synchronize();
      return Expr;
    }
    if (!consume(Kind::semi)) {
      const Span PrevEnd = Stream.previous().TokenSpan;
      const lexer::Token Next = peekToken();
      const auto Found = lexer::Token::getTokenName(Next.TokenKind);
      const Span Insert = {PrevEnd.End, PrevEnd.End};
      diag(Insert, DiagID::ExpectedSemi)
          .arg("context", "after expression")
          .arg("found", Found.str())
          .help("add `;` here")
          .note("in Eter, expression statements must end with `;`")
          .label(Next.TokenSpan, "unexpected token");
    }
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

  const Span Start = expect(Kind::kw_let, DiagID::ExpectedKeyword).TokenSpan;

  const Regime LetRegime = parseRegime();

  const InternedStr Name = expectName(Kind::identifier, "variable");

  llvm::SmallVector<NodeIndex, 2> Children;
  if (consume(Kind::colon))
    Children.push_back(parseType());

  if (consume(Kind::eq))
    Children.push_back(parseExpr());

  if (!Children.empty() && Pool.kindOf(Children.back()) == NodeKind::Error) {
    // The initialiser already produced a diagnostic; resynchronise to the
    // end of the statement without emitting the follow-up ';' diagnostic.
    // Stop at '}' as well so a missing ';' does not swallow the block close.
    while (!check(Kind::semi) && !check(Kind::r_brace) && !atEof())
      advance();
    consume(Kind::semi);
  } else if (!consume(Kind::semi)) {
    const Span PrevEnd = Stream.previous().TokenSpan;
    const lexer::Token Next = peekToken();
    const auto Found = lexer::Token::getTokenName(Next.TokenKind);
    diag(PrevEnd, DiagID::ExpectedSemi)
        .arg("context", "in let statement")
        .arg("found", Found.str())
        .help("add `;` here")
        .note("all statements must end with `;` in Eter")
        .label(Next.TokenSpan, "unexpected token");
  }

  return Pool.alloc(NodeKind::LetStmt,
                    Span{Start.Start, Stream.previous().TokenSpan.End},
                    Children, NodePool::makePayload(Name, LetRegime));
}

NodeIndex Parser::parseRetStmt() {
  ETER_DEBUG(llvm::dbgs() << "[" DEBUG_TYPE "] parseRetStmt\n");

  using Kind = lexer::Token::Kind;

  const Span Start = expect(Kind::kw_ret, DiagID::ExpectedKeyword).TokenSpan;

  const NodeIndex RetExpr = parseExpr();

  const Span End =
      expect(Kind::semi, DiagID::ExpectedSemi, "after expression").TokenSpan;

  return Pool.alloc(NodeKind::RetStmt, Span{Start.Start, End.End}, {RetExpr});
}

NodeIndex Parser::parseIfExpr() {
  ETER_DEBUG(llvm::dbgs() << "[" DEBUG_TYPE "] parseIfExpr\n");

  using Kind = lexer::Token::Kind;

  const Span Start = expect(Kind::kw_if, DiagID::ExpectedKeyword).TokenSpan;

  llvm::SmallVector<NodeIndex, 3> Children;
  {
    // A `{` after the condition opens the then-block, not a struct literal:
    // `if x { … }` must not parse the condition as `x { … }`.
    const llvm::SaveAndRestore<bool> NoStructLit(StructLitAllowed, false);
    Children.push_back(parseExpr());
  }

  Children.push_back(parseBlockExpr());

  if (check(Kind::kw_else)) {
    advance();
    if (check(Kind::kw_if))
      Children.push_back(parseIfExpr());
    else
      Children.push_back(parseBlockExpr());
  }

  return Pool.alloc(NodeKind::IfExpr,
                    Span{Start.Start, Stream.previous().TokenSpan.End},
                    Children);
}

NodeIndex Parser::parseWhileStmt() {
  ETER_DEBUG(llvm::dbgs() << "[" DEBUG_TYPE "] parseWhileStmt\n");

  using Kind = lexer::Token::Kind;

  const Span Start = expect(Kind::kw_while, DiagID::ExpectedKeyword).TokenSpan;

  llvm::SmallVector<NodeIndex, 2> Children;
  {
    // Same restriction as `if`: the `{` opens the loop body.
    const llvm::SaveAndRestore<bool> NoStructLit(StructLitAllowed, false);
    Children.push_back(parseExpr());
  }

  Children.push_back(parseBlockExpr());

  return Pool.alloc(NodeKind::WhileStmt,
                    Span{Start.Start, Stream.previous().TokenSpan.End},
                    Children);
}

NodeIndex Parser::parseForStmt() {
  ETER_DEBUG(llvm::dbgs() << "[" DEBUG_TYPE "] parseForStmt\n");
  using Kind = lexer::Token::Kind;

  const Span Start = expect(Kind::kw_for, DiagID::ExpectedKeyword).TokenSpan;

  const Regime LoopRegime = parseRegime();

  const InternedStr Name = expectName(Kind::identifier, "loop variable");

  expect(Kind::kw_in_, DiagID::ExpectedKeyword);

  NodeIndex Iterable;
  {
    const llvm::SaveAndRestore<bool> NoStructLit(StructLitAllowed, false);
    Iterable = parseExpr();
  }

  const NodeIndex Body = parseBlockExpr();

  return Pool.alloc(NodeKind::ForStmt, Span{Start.Start, Pool.spanOf(Body).End},
                    {Iterable, Body}, NodePool::makePayload(Name, LoopRegime));
}

NodeIndex Parser::parseBreakStmt() {
  ETER_DEBUG(llvm::dbgs() << "[" DEBUG_TYPE "] parseBreakStmt\n");
  using Kind = lexer::Token::Kind;

  const lexer::Token Tok = advance(); // consume 'break'
  expect(Kind::semi, DiagID::ExpectedBreakOrContinueSemi);
  return Pool.allocLeaf(NodeKind::BreakStmt, Tok.TokenSpan);
}

NodeIndex Parser::parseContinueStmt() {
  ETER_DEBUG(llvm::dbgs() << "[" DEBUG_TYPE "] parseContinueStmt\n");
  using Kind = lexer::Token::Kind;

  const lexer::Token Tok = advance(); // consume 'continue'
  expect(Kind::semi, DiagID::ExpectedBreakOrContinueSemi);
  return Pool.allocLeaf(NodeKind::ContinueStmt, Tok.TokenSpan);
}

NodeIndex Parser::parseUnsafeBlock() {
  ETER_DEBUG(llvm::dbgs() << "[" DEBUG_TYPE "] parseUnsafeBlock\n");
  using Kind = lexer::Token::Kind;

  const Span Start = expect(Kind::kw_unsafe, DiagID::ExpectedKeyword).TokenSpan;

  llvm::SmallVector<NodeIndex, 8> Children;
  if (!consume(Kind::l_brace))
    diag(peekToken().TokenSpan, DiagID::ExpectedOpen)
        .arg("tok", "{")
        .arg("context", "to start unsafe block body");

  while (!check(Kind::r_brace) && !atEof()) {
    Children.push_back(parseStmt());
  }

  const Span End =
      expect(Kind::r_brace, DiagID::ExpectedClose, "to close unsafe block")
          .TokenSpan;
  return Pool.alloc(NodeKind::UnsafeBlock, Span{Start.Start, End.End},
                    Children);
}

NodeIndex Parser::parseMatchExpr() {
  ETER_DEBUG(llvm::dbgs() << "[" DEBUG_TYPE "] parseMatchExpr\n");

  using Kind = lexer::Token::Kind;

  const Span Start = expect(Kind::kw_match, DiagID::ExpectedKeyword).TokenSpan;

  NodeIndex Scrutinee;
  {
    // Same restriction as `if`: the `{` opens the arm list.
    const llvm::SaveAndRestore<bool> NoStructLit(StructLitAllowed, false);
    Scrutinee = parseExpr();
  }

  expect(Kind::l_brace, DiagID::ExpectedOpen, "to start match body");

  llvm::SmallVector<NodeIndex, 8> Arms;
  while (!check(Kind::r_brace) && !atEof()) {
    Arms.push_back(parseMatchArm());
  }

  const Span End =
      expect(Kind::r_brace, DiagID::ExpectedClose, "to close match body")
          .TokenSpan;

  llvm::SmallVector<NodeIndex, 8> Children;
  Children.push_back(Scrutinee);
  Children.append(Arms.begin(), Arms.end());
  return Pool.alloc(NodeKind::MatchExpr, Span{Start.Start, End.End}, Children);
}

NodeIndex Parser::parseMatchArm() {
  ETER_DEBUG(llvm::dbgs() << "[" DEBUG_TYPE "] parseMatchArm\n");

  using Kind = lexer::Token::Kind;

  const NodeIndex Pat = parsePat();

  expect(Kind::fat_arrow, DiagID::ExpectedFatArrow);

  const NodeIndex Value = parseExpr();

  if (!consume(Kind::comma))
    consume(Kind::semi);

  return Pool.alloc(NodeKind::MatchArm,
                    Span{Pool.spanOf(Pat).Start, Pool.spanOf(Value).End},
                    {Pat, Value});
}

NodeIndex Parser::parseBlockExpr() {
  ETER_DEBUG(llvm::dbgs() << "[" DEBUG_TYPE "] parseBlockExpr\n");
  using Kind = lexer::Token::Kind;

  const Span Start =
      expect(Kind::l_brace, DiagID::ExpectedOpen, "to start block").TokenSpan;

  llvm::SmallVector<NodeIndex, 8> Children;

  while (!check(Kind::r_brace) && !atEof()) {
    Children.push_back(parseStmt());
  }

  const Span End =
      expect(Kind::r_brace, DiagID::ExpectedClose, "to close block").TokenSpan;
  return Pool.alloc(NodeKind::BlockExpr, Span{Start.Start, End.End}, Children);
}

} // namespace eter::parser
