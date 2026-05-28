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

#include <llvm/Support/ErrorHandling.h>
#include <llvm/Support/raw_ostream.h>

#include <cstdint>

#define DEBUG_TYPE "parser"

namespace eter::parser {

//===----------------------------------------------------------------------===//
// Public entry point
//===----------------------------------------------------------------------===//

ParseResult Parser::parse(TokenStream Tokens, StringInterner &Interner) {
  ETER_DEBUG(llvm::dbgs() << "[" DEBUG_TYPE "] parse()\n");
  ParseResult Result;
  Parser P(std::move(Tokens), Result.Pool, Interner, Result.Errors);
  Result.Root = P.parseSourceFile();
  return Result;
}

//===----------------------------------------------------------------------===//
// Private constructor
//===----------------------------------------------------------------------===//

Parser::Parser(TokenStream Tokens, NodePool &Pool, StringInterner &Interner,
               std::vector<diag::PhaseDiagnostic> &Errors)
    : Stream(std::move(Tokens)), Pool(Pool), Interner(Interner),
      Errors(Errors) {}

//===----------------------------------------------------------------------===//
// Top-level
//===----------------------------------------------------------------------===//

NodeIndex Parser::parseSourceFile() {
  ETER_DEBUG(llvm::dbgs() << "[" DEBUG_TYPE "] parseSourceFile()\n");
  const Span StartOfFile = peekToken().TokenSpan;

  std::vector<NodeIndex> Children;
  for (const NodeIndex N : parseFileDocComments())
    Children.push_back(N);

  while (!atEof()) {
    Children.push_back(parseTopLevelDecl({}));
  }

  const Span EndOfFile = Stream.previous().TokenSpan;
  return Pool.alloc(NodeKind::SourceFile,
                    Span{StartOfFile.Start, EndOfFile.End}, Children);
}

//===----------------------------------------------------------------------===//
// Attributes
//===----------------------------------------------------------------------===//

llvm::SmallVector<NodeIndex, 4> Parser::parseAttributes() {
  llvm::report_fatal_error("TODO: implement Parser::parseAttributes");
}

NodeIndex Parser::parseAttribute() {
  llvm::report_fatal_error("TODO: implement Parser::parseAttribute");
}

llvm::SmallVector<NodeIndex, 4> Parser::parseDocComments() {
  using Kind = lexer::Token::Kind;
  llvm::SmallVector<NodeIndex, 4> Out;
  while (check(Kind::doc_comment)) {
    const lexer::Token Tok = advance();
    const InternedStr Text = Interner.intern(textOf(Tok.TokenSpan));
    Out.push_back(Pool.allocLeaf(NodeKind::DocComment, Tok.TokenSpan, Text));
  }
  return Out;
}

llvm::SmallVector<NodeIndex, 4> Parser::parseFileDocComments() {
  using Kind = lexer::Token::Kind;
  llvm::SmallVector<NodeIndex, 4> Out;
  while (check(Kind::file_doc_comment)) {
    const lexer::Token Tok = advance();
    const InternedStr Text = Interner.intern(textOf(Tok.TokenSpan));
    Out.push_back(
        Pool.allocLeaf(NodeKind::FileDocComment, Tok.TokenSpan, Text));
  }
  return Out;
}

//===----------------------------------------------------------------------===//
// Token stream helpers
//===----------------------------------------------------------------------===//

lexer::Token::Kind Parser::peek(uint32_t Offset) const {
  return Stream.peek(Offset);
}

lexer::Token Parser::peekToken(uint32_t Offset) const {
  return Stream.peekToken(Offset);
}

bool Parser::check(lexer::Token::Kind K) const { return Stream.check(K); }

bool Parser::atEof() const { return Stream.atEof(); }

lexer::Token Parser::advance() { return Stream.advance(); }

bool Parser::consume(lexer::Token::Kind K) { return Stream.consume(K); }

lexer::Token Parser::expect(lexer::Token::Kind K, DiagID D) {
  if (consume(K))
    return Stream.previous();
  auto T = Stream.peekToken();
  addError(T.TokenSpan, D);
  return lexer::Token(lexer::Token::Kind::unknown, T.TokenSpan);
}

InternedStr Parser::expectAndIntern(lexer::Token::Kind K, DiagID D) {
  return Interner.intern(textOf(expect(K, D).TokenSpan));
}

//===----------------------------------------------------------------------===//
// Error recovery
//===----------------------------------------------------------------------===//

void Parser::addError(Span S, DiagID D) {
  diag::PhaseDiagnostic PD;
  PD.Ph = diag::Phase::Parser;
  PD.LocalID = static_cast<uint16_t>(D);
  PD.Loc = S;
  Errors.push_back(std::move(PD));
}

NodeIndex Parser::makeErrorNode(Span S) {
  return Pool.allocLeaf(NodeKind::Error, S);
}

void Parser::synchronize() {
  using Kind = lexer::Token::Kind;

  // Consume at least one token to guarantee forward progress; the caller has
  // already recorded a parse error for the offending token.
  if (!atEof())
    advance();

  while (!atEof()) {
    // Just-consumed `;` closed a statement — recovery is complete.
    if (Stream.previous().TokenKind == Kind::semi)
      return;

    // The next token starts a fresh top-level item or closes the enclosing
    // block — stop without consuming so the caller can resume there.
    switch (peek()) {
    case Kind::r_brace:
    case Kind::kw_fn:
    case Kind::kw_struct:
    case Kind::kw_enum:
    case Kind::kw_mod:
    case Kind::kw_use:
    case Kind::kw_const:
      return;
    default:
      advance();
    }
  }
}

//===----------------------------------------------------------------------===//
// Regime
//===----------------------------------------------------------------------===//

Regime Parser::parseRegime() {
  switch (peek()) {
  case lexer::Token::Kind::kw_imm:
    advance();
    return Regime::Imm;
  case lexer::Token::Kind::kw_mut:
    advance();
    return Regime::Mut;
  case lexer::Token::Kind::kw_proj:
    advance();
    return Regime::Proj;
  default:
    return Regime::None;
  }
}

} // namespace eter::parser
