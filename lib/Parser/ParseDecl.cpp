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

NodeIndex
Parser::parseTopLevelDecl([[maybe_unused]] llvm::ArrayRef<NodeIndex> Attrs) {
  ETER_DEBUG(llvm::dbgs() << "[" DEBUG_TYPE "] parseTopLevelDecl\n");
  // FIXME: Dispatch the correct Top Level Declarations
  return parseConstDecl();
}

NodeIndex
Parser::parseFnDecl([[maybe_unused]] llvm::ArrayRef<NodeIndex> Attrs) {
  ETER_DEBUG(llvm::dbgs() << "[" DEBUG_TYPE "] parseFnDecl\n");
  llvm::report_fatal_error("TODO: implement Parser::parseFnDecl");
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
  llvm::report_fatal_error("TODO: implement Parser::parseParamList");
}

NodeIndex Parser::parseParam() {
  ETER_DEBUG(llvm::dbgs() << "[" DEBUG_TYPE "] parseParam\n");
  llvm::report_fatal_error("TODO: implement Parser::parseParam");
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
