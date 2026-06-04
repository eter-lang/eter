//===----------------------------------------------------------------------===//
//
// Part of the Eter Project, under the Apache License v2.0 with LLVM Exceptions.
// See /LICENSE for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "eter/Base/Debug.h"
#include "eter/Parser/Parser.h"

#include <llvm/ADT/StringRef.h>
#include <llvm/Support/ErrorHandling.h>
#include <llvm/Support/raw_ostream.h>

#define DEBUG_TYPE "parser-pat"

namespace eter::parser {

NodeIndex Parser::parsePat() {
  ETER_DEBUG(llvm::dbgs() << "[" DEBUG_TYPE "] parsePat\n");

  using Kind = lexer::Token::Kind;

  const lexer::Token Tok = Stream.peekToken();

  switch (Tok.TokenKind) {
  case Kind::identifier: {
    const llvm::StringRef Text = textOf(Tok.TokenSpan);
    if (Text == "_") {
      advance();
      return Pool.allocLeaf(NodeKind::WildcardPat, Tok.TokenSpan);
    }
    // FIXME: Check for ( or { after identifier to dispatch to
    // parseTuplePat/parseStructPat.
    advance();
    return Pool.allocLeaf(
        NodeKind::IdentPat, Tok.TokenSpan,
        NodePool::makePayload(Interner.intern(Text), Regime::None));
  }
  case Kind::integer_literal:
  case Kind::float_literal:
  case Kind::char_literal:
  case Kind::string_literal:
  case Kind::kw_true:
  case Kind::kw_false: {
    advance();
    return Pool.allocLeaf(NodeKind::LiteralPat, Tok.TokenSpan,
                          Interner.intern(textOf(Tok.TokenSpan)));
  }
  default:
    addError(Tok.TokenSpan, DiagID::ExpectedPattern);
    advance();
    return makeErrorNode(Tok.TokenSpan);
  }
}

NodeIndex Parser::parseStructPat(InternedStr Name, Span Start) {
  ETER_DEBUG(llvm::dbgs() << "[" DEBUG_TYPE "] parseStructPat\n");
  (void)Name;
  (void)Start;
  llvm::report_fatal_error("TODO: implement Parser::parseStructPat");
}

NodeIndex Parser::parseTuplePat(InternedStr Name, Span Start) {
  ETER_DEBUG(llvm::dbgs() << "[" DEBUG_TYPE "] parseTuplePat\n");
  (void)Name;
  (void)Start;
  llvm::report_fatal_error("TODO: implement Parser::parseTuplePat");
}

} // namespace eter::parser
