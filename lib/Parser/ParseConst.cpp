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

#include <llvm/ADT/StringRef.h>
#include <llvm/Support/ErrorHandling.h>
#include <llvm/Support/raw_ostream.h>

#define DEBUG_TYPE "parser-const-expr"

namespace eter::parser {

NodeIndex Parser::parseConstDecl(llvm::ArrayRef<NodeIndex> Docs,
                                 llvm::ArrayRef<NodeIndex> Attrs) {
  ETER_DEBUG(llvm::dbgs() << "[" DEBUG_TYPE "] parseConstDecl\n");
  using Kind = lexer::Token::Kind;

  const Span Start =
      expect(Kind::kw_const, DiagID::ExpectedConstKeyword).TokenSpan;

  const InternedStr Name =
      expectAndIntern(Kind::identifier, DiagID::ExpectedConstName);

  expect(Kind::colon, DiagID::ExpectedConstColon);
  const NodeIndex Type = parseType();

  expect(Kind::eq, DiagID::ExpectedConstEquals);
  const NodeIndex Value = parseConstExpr();

  const Span End = expect(Kind::semi, DiagID::ExpectedConstSemi).TokenSpan;

  llvm::SmallVector<NodeIndex, 8> Children(Docs.begin(), Docs.end());
  Children.append(Attrs.begin(), Attrs.end());
  Children.push_back(Type);
  Children.push_back(Value);

  return Pool.alloc(NodeKind::ConstDecl, Span{Start.Start, End.End}, Children,
                    Name);
}

NodeIndex Parser::parseConstExpr([[maybe_unused]] int MinBP) {
  ETER_DEBUG(llvm::dbgs() << "[" DEBUG_TYPE "] parseConstExpr minBP=" << MinBP
                          << "\n");

  const lexer::Token Tok = Stream.peekToken();

  switch (Tok.TokenKind) {
  case lexer::Token::Kind::integer_literal:
  case lexer::Token::Kind::float_literal:
  case lexer::Token::Kind::char_literal:
  case lexer::Token::Kind::string_literal:
  case lexer::Token::Kind::kw_true:
  case lexer::Token::Kind::kw_false:
    Stream.advance();
    return parseLitExpr(Tok);
  default:
    addError(Tok.TokenSpan, DiagID::ExpectedConstLiteral);
    return makeErrorNode(Tok.TokenSpan);
  }
}

} // namespace eter::parser
