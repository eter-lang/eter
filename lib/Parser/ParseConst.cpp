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

#define DEBUG_TYPE "parser-const-expr"

namespace eter::parser {

NodeIndex Parser::parseConstDecl() {
  const uint32_t StartSpan = advance().TokenSpan.Start; // Discard "const"
  std::vector<NodeIndex> Children;

  const InternedStr NameRef = Interner.intern(
      Stream.textOf(advance().TokenSpan)); // Get the variable name
  advance();                               //  Discard ":"

  const NodeIndex Type = parseType(); // Parse the type. Note that for `const`
                                      // declarations the type is mandatory.
  Children.push_back(Type);

  Stream.advance(); // Discard "="
  const NodeIndex RValue = parseConstExpr();
  Stream.advance(); // Discard ";"
  Children.push_back(RValue);

  return Pool.alloc(NodeKind::ConstDecl,
                    Span{StartSpan, Stream.previous().TokenSpan.End}, Children,
                    NameRef);
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
    addError(Tok.TokenSpan, "expected literal expression in const expression");
    return makeErrorNode(Tok.TokenSpan);
  }
}

} // namespace eter::parser
