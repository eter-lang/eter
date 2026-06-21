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

#include <llvm/ADT/STLExtras.h>
#include <llvm/ADT/StringRef.h>
#include <llvm/Support/ErrorHandling.h>
#include <llvm/Support/SaveAndRestore.h>
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
    // Path expression: name :: name (:: name)*  (e.g. Animals::Cat).
    Span PathSpan = Tok.TokenSpan;
    const bool IsPath = parsePathSegments(PathSpan);
    const InternedStr Name = Interner.intern(textOf(PathSpan));
    // Struct literal: Name { FieldInit* }, possibly path-qualified
    // (Animal::Spider{ … }). Suppressed in if/while/match headers, where
    // the `{` opens the construct's block instead.
    if (StructLitAllowed && check(Kind::l_brace))
      return parseStructLitExpr(Name, PathSpan);
    return Pool.allocLeaf(IsPath ? NodeKind::PathExpr : NodeKind::IdentExpr,
                          PathSpan, Name);
  }
  case Kind::l_paren: {
    advance();
    // Parentheses disambiguate, so struct literals are legal again even
    // inside an if/while/match header.
    const llvm::SaveAndRestore<bool> AllowStructLit(StructLitAllowed, true);

    // Unit value: ()
    if (check(Kind::r_paren)) {
      const Span End = advance().TokenSpan;
      return Pool.alloc(NodeKind::TupleExpr, Span{Tok.TokenSpan.Start, End.End},
                        {});
    }

    const NodeIndex Inner = parseExpr(0);

    // Tuple expression: ( Expr, Expr, ... ). The trailing comma is what
    // distinguishes a one-element tuple `(x,)` from a parenthesised
    // expression `(x)`.
    if (check(Kind::comma)) {
      llvm::SmallVector<NodeIndex, 4> Elems;
      Elems.push_back(Inner);
      while (consume(Kind::comma)) {
        if (check(Kind::r_paren)) // trailing comma
          break;
        Elems.push_back(parseExpr(0));
      }
      const Span End =
          expect(Kind::r_paren, DiagID::ExpectedTupleExprClose).TokenSpan;
      return Pool.alloc(NodeKind::TupleExpr, Span{Tok.TokenSpan.Start, End.End},
                        Elems);
    }

    expect(Kind::r_paren, DiagID::ExpectedParenExprClose);
    return Inner;
  }
  case Kind::l_square:
    return parseArrayLitExpr();
  case Kind::kw_if:
    return parseIfExpr();
  case Kind::kw_while:
    return parseWhileStmt();
  case Kind::kw_match:
    return parseMatchExpr();
  case Kind::kw_unsafe:
    return parseUnsafeBlock();
  case Kind::l_brace:
    return parseBlockExpr();
  case Kind::kw_tensor:
    return parseTensorLitExpr();
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
  case Kind::star: {
    const int RhsBP = prefixBindingPower(Tok.TokenKind);
    advance();
    const NodeIndex Operand = parseExpr(RhsBP);
    // `*lhs = rhs` is a projection assignment.
    if (consume(Kind::eq)) {
      const NodeIndex Rhs = parseExpr(0);
      return Pool.alloc(NodeKind::ProjAssignExpr,
                        Span{Tok.TokenSpan.Start, Pool.spanOf(Rhs).End},
                        {Operand, Rhs});
    }
    return Pool.alloc(
        NodeKind::UnaryExpr,
        Span{Tok.TokenSpan.Start, Pool.spanOf(Operand).End}, {Operand},
        NodePool::makeOpPayload(static_cast<uint16_t>(Tok.TokenKind)));
  }
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
      if (check(Kind::integer_literal)) {
        const lexer::Token IdxTok = advance();
        Lhs = Pool.alloc(NodeKind::TupleIndexExpr,
                         Span{Pool.spanOf(Lhs).Start, IdxTok.TokenSpan.End},
                         {Lhs}, Interner.intern(textOf(IdxTok.TokenSpan)));
        continue;
      }
      // Chained tuple indexing: `t.0.1` lexes `.` `0.1` (float_literal).
      // Extract each integer part as a separate tuple index.
      if (check(Kind::float_literal)) {
        const lexer::Token FloatTok = peekToken();
        const llvm::StringRef Text = textOf(FloatTok.TokenSpan);
        const size_t DotPos = Text.find('.');
        if (DotPos != llvm::StringRef::npos && DotPos + 1 < Text.size()) {
          const llvm::StringRef IntPart = Text.substr(0, DotPos);
          llvm::StringRef FracPart = Text.substr(DotPos + 1);
          // Strip optional 'f' suffix from the fractional part.
          if (!FracPart.empty() && FracPart.back() == 'f')
            FracPart = FracPart.drop_back();
          advance(); // consume float_literal
          Lhs = Pool.alloc(NodeKind::TupleIndexExpr,
                           Span{Pool.spanOf(Lhs).Start, FloatTok.TokenSpan.End},
                           {Lhs}, Interner.intern(IntPart));
          // If the fractional part is pure digits, chain another index.
          if (!FracPart.empty() && llvm::all_of(FracPart, [](char C) {
                return C >= '0' && C <= '9';
              })) {
            Lhs =
                Pool.alloc(NodeKind::TupleIndexExpr,
                           Span{Pool.spanOf(Lhs).Start, FloatTok.TokenSpan.End},
                           {Lhs}, Interner.intern(FracPart));
          }
          continue;
        }
      }
      const InternedStr Field =
          expectAndIntern(Kind::identifier, DiagID::ExpectedFieldName);
      Lhs = Pool.alloc(
          NodeKind::FieldExpr,
          Span{Pool.spanOf(Lhs).Start, Stream.previous().TokenSpan.End}, {Lhs},
          NodePool::makePayload(Field, Regime::None));
      continue;
    }
    case Kind::l_square:
      Lhs = parsePostfixIndex(Lhs);
      continue;
    case Kind::question: {
      advance();
      Lhs = Pool.alloc(
          NodeKind::PropagateExpr,
          Span{Pool.spanOf(Lhs).Start, Stream.previous().TokenSpan.End}, {Lhs});
      continue;
    }
    case Kind::kw_as: {
      advance();
      const NodeIndex Ty = parseType();
      Lhs = Pool.alloc(NodeKind::CastExpr,
                       Span{Pool.spanOf(Lhs).Start, Pool.spanOf(Ty).End},
                       {Lhs, Ty});
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
      expect(Kind::l_paren, DiagID::ExpectedOpen, "to start argument list")
          .TokenSpan;

  // Parentheses disambiguate (cf. the parenthesised-expression case).
  const llvm::SaveAndRestore<bool> AllowStructLit(StructLitAllowed, true);

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

NodeIndex Parser::parseStructLitExpr(InternedStr Name, Span Start) {
  ETER_DEBUG(llvm::dbgs() << "[" DEBUG_TYPE "] parseStructLitExpr\n");
  using Kind = lexer::Token::Kind;

  advance(); // consume '{' (guaranteed by the caller's check)

  // Inside the braces there is no block ambiguity, so nested struct literals
  // are legal again even when the outer context suppressed them.
  const llvm::SaveAndRestore<bool> AllowStructLit(StructLitAllowed, true);

  llvm::SmallVector<NodeIndex, 8> Fields;
  parseCommaSeparated(Fields, Kind::r_brace,
                      [this] { return parseFieldInit(); });

  const Span End =
      expect(Kind::r_brace, DiagID::ExpectedStructLitClose).TokenSpan;
  return Pool.alloc(NodeKind::StructLitExpr, Span{Start.Start, End.End}, Fields,
                    Name);
}

NodeIndex Parser::parseFieldInit() {
  ETER_DEBUG(llvm::dbgs() << "[" DEBUG_TYPE "] parseFieldInit\n");
  using Kind = lexer::Token::Kind;

  // Bail out at the first error; parseCommaSeparated resynchronises to the
  // next field initialiser.
  if (!check(Kind::identifier)) {
    const Span S = peekToken().TokenSpan;
    addError(S, DiagID::ExpectedFieldInitName);
    return makeErrorNode(S);
  }
  const lexer::Token NameTok = advance();
  const InternedStr Name = Interner.intern(textOf(NameTok.TokenSpan));

  if (consume(Kind::colon)) {
    const NodeIndex Init = parseExpr();
    return Pool.alloc(NodeKind::FieldInit,
                      Span{NameTok.TokenSpan.Start, Pool.spanOf(Init).End},
                      {Init}, Name);
  }

  // Shorthand `name` ≡ `name: name`: synthesise the IdentExpr child.
  const NodeIndex Ident =
      Pool.allocLeaf(NodeKind::IdentExpr, NameTok.TokenSpan, Name);
  return Pool.alloc(NodeKind::FieldInit, NameTok.TokenSpan, {Ident}, Name);
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

NodeIndex Parser::parseArrayLitExpr() {
  ETER_DEBUG(llvm::dbgs() << "[" DEBUG_TYPE "] parseArrayLitExpr\n");
  using Kind = lexer::Token::Kind;

  const Span Start = advance().TokenSpan; // consume '['
  const llvm::SaveAndRestore<bool> AllowStructLit(StructLitAllowed, true);

  // Empty array literal: []
  if (check(Kind::r_square)) {
    const Span End = advance().TokenSpan;
    return Pool.alloc(NodeKind::ArrayLitExpr, Span{Start.Start, End.End}, {});
  }

  const NodeIndex First = parseExpr(0);

  // Repeat form: [ value ; count ]  (e.g. [0; 5])
  if (consume(Kind::semi)) {
    const NodeIndex Count = parseConstExpr();
    const Span End =
        expect(Kind::r_square, DiagID::ExpectedArrayLitClose).TokenSpan;
    return Pool.alloc(NodeKind::ArrayRepeatExpr, Span{Start.Start, End.End},
                      {First, Count});
  }

  // List form: [ Expr (, Expr)* ,? ]
  llvm::SmallVector<NodeIndex, 8> Elems;
  Elems.push_back(First);
  while (consume(Kind::comma)) {
    if (check(Kind::r_square)) // trailing comma
      break;
    Elems.push_back(parseExpr(0));
  }
  const Span End =
      expect(Kind::r_square, DiagID::ExpectedArrayLitClose).TokenSpan;
  return Pool.alloc(NodeKind::ArrayLitExpr, Span{Start.Start, End.End}, Elems);
}

NodeIndex Parser::parseTensorLitExpr() {
  ETER_DEBUG(llvm::dbgs() << "[" DEBUG_TYPE "] parseTensorLitExpr\n");
  using Kind = lexer::Token::Kind;

  const Span Start = advance().TokenSpan; // consume 'tensor'
  expect(Kind::l_square, DiagID::ExpectedTensorOpen);
  llvm::SmallVector<NodeIndex, 8> Children;
  {
    const llvm::SaveAndRestore<bool> AllowStructLit(StructLitAllowed, true);
    // Parse value(s) — comma-separated until ';'
    Children.push_back(parseExpr(0));
    while (consume(Kind::comma)) {
      if (check(Kind::semi) || check(Kind::r_square))
        break; // trailing comma guard
      Children.push_back(parseExpr(0));
    }
  }
  const auto NumValues = static_cast<uint16_t>(Children.size());
  expect(Kind::semi, DiagID::ExpectedSemi, "in tensor literal");
  // Parse dims (comma-separated const exprs)
  parseCommaSeparated(Children, Kind::r_square,
                      [this] { return parseConstExpr(); });
  if (static_cast<uint16_t>(Children.size()) == NumValues)
    addError(peekToken().TokenSpan, DiagID::ExpectedTensorLitDim);
  const Span End =
      expect(Kind::r_square, DiagID::ExpectedTensorLitClose).TokenSpan;
  return Pool.alloc(NodeKind::TensorLitExpr, Span{Start.Start, End.End},
                    Children, NodePool::makeOpPayload(NumValues));
}

NodeIndex Parser::parsePostfixIndex(NodeIndex Lhs) {
  ETER_DEBUG(llvm::dbgs() << "[" DEBUG_TYPE "] parsePostfixIndex\n");
  using Kind = lexer::Token::Kind;

  advance(); // consume '['
  const Span LhsSpan = Pool.spanOf(Lhs);

  // Collect all comma-separated index expressions.
  // [base, idx0]      → IndexExpr       (array / 1D-tensor single index)
  // [base, idx0, ...] → TensorIndexExpr (2+ indices for ND tensors)
  llvm::SmallVector<NodeIndex, 4> Children;
  Children.push_back(Lhs);
  {
    const llvm::SaveAndRestore<bool> AllowStructLit(StructLitAllowed, true);
    Children.push_back(parseExpr());
    while (consume(Kind::comma)) {
      if (check(Kind::r_square))
        break; // trailing comma
      Children.push_back(parseExpr());
    }
  }
  const bool IsTensor = Children.size() > 2;
  const Span Close =
      expect(Kind::r_square, IsTensor ? DiagID::ExpectedTensorIndexClose
                                      : DiagID::ExpectedRSquare)
          .TokenSpan;
  if (IsTensor)
    return Pool.alloc(NodeKind::TensorIndexExpr, Span{LhsSpan.Start, Close.End},
                      Children);
  return Pool.alloc(NodeKind::IndexExpr, Span{LhsSpan.Start, Close.End},
                    {Children[0], Children[1]});
}

int Parser::prefixBindingPower(lexer::Token::Kind K) {
  // Prefix operators bind tighter than all binary operators (right_bp = 110).
  // & in prefix position means "take a projection of a mut value".
  using Kind = lexer::Token::Kind;
  switch (K) {
  case Kind::bang:
  case Kind::minus:
  case Kind::amp:
  case Kind::star:
    return 110;
  default:
    return -1;
  }
}

} // namespace eter::parser
