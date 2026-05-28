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
#include "eter/Parser/NodePool.h"
#include "eter/Parser/Parser.h"

#include <llvm/ADT/StringRef.h>
#include <llvm/Support/ErrorHandling.h>
#include <llvm/Support/raw_ostream.h>

#define DEBUG_TYPE "parser-decl"

namespace eter::parser {

NodeIndex Parser::parseTopLevelDecl(llvm::ArrayRef<NodeIndex> Attrs) {
  ETER_DEBUG(llvm::dbgs() << "[" DEBUG_TYPE "] parseTopLevelDecl\n");
  using Kind = lexer::Token::Kind;

  switch (peek()) {
  case Kind::kw_fn:
    return parseFnDecl(Attrs);
  case Kind::kw_const:
    return parseConstDecl();
  case Kind::kw_mod:
    return parseModDecl();
  case Kind::kw_struct:
    return parseStructDecl(Attrs);
  case Kind::kw_enum:
    return parseEnumDecl(Attrs);
  case Kind::kw_use:
    return parseUseDecl();
  default: {
    const lexer::Token Tok = peekToken();
    addError(Tok.TokenSpan, "expected a top-level declaration");
    advance(); // avoid infinite loop until synchronize() is implemented
    return makeErrorNode(Tok.TokenSpan);
  }
  }
}

NodeIndex Parser::parseFnDecl(llvm::ArrayRef<NodeIndex> Attrs) {
  ETER_DEBUG(llvm::dbgs() << "[" DEBUG_TYPE "] parseFnDecl\n");
  using Kind = lexer::Token::Kind;

  const Span Start = expect(Kind::kw_fn, "expected 'fn'").TokenSpan;

  const Regime ReturnRegime = parseRegime();

  const lexer::Token NameTok =
      expect(Kind::identifier, "expected function name");
  const InternedStr Name = Interner.intern(textOf(NameTok.TokenSpan));

  llvm::SmallVector<NodeIndex, 8> Children(Attrs.begin(), Attrs.end());
  Children.push_back(parseParamList());

  if (consume(Kind::colon))
    Children.push_back(parseType());

  Children.push_back(parseBlockExpr());

  return Pool.alloc(NodeKind::FnDecl,
                    Span{Start.Start, Stream.previous().TokenSpan.End},
                    Children, NodePool::makePayload(Name, ReturnRegime));
}

NodeIndex
Parser::parseStructDecl([[maybe_unused]] llvm::ArrayRef<NodeIndex> Attrs) {
  ETER_DEBUG(llvm::dbgs() << "[" DEBUG_TYPE "] parseStructDecl\n");
  llvm::report_fatal_error("TODO: implement Parser::parseStructDecl");
}

NodeIndex
Parser::parseEnumDecl([[maybe_unused]] llvm::ArrayRef<NodeIndex> Attrs) {
  ETER_DEBUG(llvm::dbgs() << "[" DEBUG_TYPE "] parseEnumDecl\n");
  llvm::report_fatal_error("TODO: implement Parser::parseEnumDecl");
}

NodeIndex Parser::parseModDecl() {
  ETER_DEBUG(llvm::dbgs() << "[" DEBUG_TYPE "] parseModDecl\n");
  using Kind = lexer::Token::Kind;

  const Span Start = expect(Kind::kw_mod, "expected 'mod'").TokenSpan;

  const lexer::Token NameTok =
      expect(Kind::identifier, "expected module name after 'mod'");
  const InternedStr Name = Interner.intern(textOf(NameTok.TokenSpan));

  if (consume(Kind::l_brace)) {
    // Inline module: mod name { TopLevelDecl* }
    llvm::SmallVector<NodeIndex, 8> Children;
    while (!check(Kind::r_brace) && !atEof())
      Children.push_back(parseTopLevelDecl(parseAttributes()));
    const Span End =
        expect(Kind::r_brace, "expected '}' to close module body").TokenSpan;
    return Pool.alloc(NodeKind::ModDecl, Span{Start.Start, End.End}, Children,
                      Name);
  }

  if (check(Kind::semi)) {
    const Span End = peekToken().TokenSpan;
    advance(); // consume ';'
    return Pool.allocLeaf(NodeKind::ModDeclFile, Span{Start.Start, End.End},
                          Name);
  }

  addError(peekToken().TokenSpan, "expected '{' or ';' after module name");
  return makeErrorNode(peekToken().TokenSpan);
}

NodeIndex Parser::parseUseDecl() {
  ETER_DEBUG(llvm::dbgs() << "[" DEBUG_TYPE "] parseUseDecl\n");
  llvm::report_fatal_error("TODO: implement Parser::parseUseDecl");
}

NodeIndex Parser::parseParamList() {
  ETER_DEBUG(llvm::dbgs() << "[" DEBUG_TYPE "] parseParamList\n");
  using Kind = lexer::Token::Kind;

  const Span Start =
      expect(Kind::l_paren, "expected '(' to start parameter list").TokenSpan;

  llvm::SmallVector<NodeIndex, 8> Children;
  if (!check(Kind::r_paren)) {
    Children.push_back(parseParam());
    while (consume(Kind::comma))
      Children.push_back(parseParam());
  }

  const Span End =
      expect(Kind::r_paren, "expected ')' to close parameter list").TokenSpan;
  return Pool.alloc(NodeKind::ParamList, Span{Start.Start, End.End}, Children);
}

NodeIndex Parser::parseParam() {
  ETER_DEBUG(llvm::dbgs() << "[" DEBUG_TYPE "] parseParam\n");
  using Kind = lexer::Token::Kind;

  const uint32_t StartPos = peekToken().TokenSpan.Start;

  const Regime R = parseRegime();

  const lexer::Token NameTok =
      expect(Kind::identifier, "expected parameter name");
  const InternedStr Name = Interner.intern(textOf(NameTok.TokenSpan));

  expect(Kind::colon, "expected ':' after parameter name");

  const NodeIndex Type = parseType();

  const NodeIndex Children[] = {Type};
  return Pool.alloc(NodeKind::Param,
                    Span{StartPos, Stream.previous().TokenSpan.End}, Children,
                    NodePool::makePayload(Name, R));
}

NodeIndex Parser::parseStructField() {
  ETER_DEBUG(llvm::dbgs() << "[" DEBUG_TYPE "] parseStructField\n");
  llvm::report_fatal_error("TODO: implement Parser::parseStructField");
}

NodeIndex Parser::parseEnumVariant() {
  ETER_DEBUG(llvm::dbgs() << "[" DEBUG_TYPE "] parseEnumVariant\n");
  llvm::report_fatal_error("TODO: implement Parser::parseEnumVariant");
}

} // namespace eter::parser
