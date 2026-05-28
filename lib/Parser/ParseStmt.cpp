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

#define DEBUG_TYPE "parser-stmt"

namespace eter::parser {

NodeIndex Parser::parseStmt() {
  ETER_DEBUG(llvm::dbgs() << "[" DEBUG_TYPE "] parseStmt\n");
  llvm::report_fatal_error("TODO: implement Parser::parseStmt");
}

NodeIndex Parser::parseLetStmt() {
  ETER_DEBUG(llvm::dbgs() << "[" DEBUG_TYPE "] parseLetStmt\n");
  llvm::report_fatal_error("TODO: implement Parser::parseLetStmt");
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

  // Statement parsing is not yet implemented
  if (!check(Kind::r_brace) && !atEof()) {
    llvm::report_fatal_error("TODO: implement Parser::parseBlockExpr");
    while (!check(Kind::r_brace) && !atEof())
      advance();
  }

  const Span End = expect(Kind::r_brace, DiagID::ExpectedBlockClose).TokenSpan;
  return Pool.allocLeaf(NodeKind::BlockExpr, Span{Start.Start, End.End});
}

NodeIndex Parser::parseMatchArm() {
  ETER_DEBUG(llvm::dbgs() << "[" DEBUG_TYPE "] parseMatchArm\n");
  llvm::report_fatal_error("TODO: implement Parser::parseMatchArm");
}

} // namespace eter::parser
