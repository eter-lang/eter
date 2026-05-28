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

#include <algorithm>
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
               std::vector<ParseError> &Errors)
    : Stream(std::move(Tokens)), Pool(Pool), Interner(Interner),
      Errors(Errors) {}

//===----------------------------------------------------------------------===//
// Top-level
//===----------------------------------------------------------------------===//

NodeIndex Parser::parseSourceFile() {
  ETER_DEBUG(llvm::dbgs() << "[" DEBUG_TYPE "] parseSourceFile()\n");
  const Span StartOfFile = peekToken().TokenSpan;

  std::vector<NodeIndex> TopLevelDecls;
  while (!atEof()) {
    TopLevelDecls.push_back(parseTopLevelDecl({}));
  }

  const Span EndOfFile = Stream.previous().TokenSpan;
  return Pool.alloc(NodeKind::SourceFile,
                    Span{StartOfFile.Start, EndOfFile.End}, TopLevelDecls);
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

lexer::Token Parser::expect(lexer::Token::Kind K, llvm::StringRef Context) {
  if (consume(K))
    return Stream.previous();
  auto T = Stream.peekToken();
  addError(T.TokenSpan, Context);
  return lexer::Token(lexer::Token::Kind::unknown, T.TokenSpan);
}

//===----------------------------------------------------------------------===//
// Error recovery
//===----------------------------------------------------------------------===//

void Parser::addError(Span S, llvm::StringRef Msg) {
  Errors.push_back(ParseError{S, Msg.str()});
}

NodeIndex Parser::makeErrorNode(Span S) {
  return Pool.allocLeaf(NodeKind::Error, S);
}

void Parser::synchronize() {
  llvm::report_fatal_error("TODO: implement Parser::synchronize");
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
