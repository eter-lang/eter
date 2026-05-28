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

#include <llvm/Support/ErrorHandling.h>
#include <llvm/Support/raw_ostream.h>

#define DEBUG_TYPE "parser-type"

namespace eter::parser {

// FIXME First check for primitive types, then custom types
NodeIndex Parser::parseType() {
  ETER_DEBUG(llvm::dbgs() << "[" DEBUG_TYPE "] parseType\n");
  using Kind = lexer::Token::Kind;

  const Span NameSpan = peekToken().TokenSpan;
  const InternedStr Name =
      expectAndIntern(Kind::identifier, DiagID::ExpectedTypeName);

  return Pool.allocLeaf(NodeKind::NamedType, NameSpan, Name);
}

NodeIndex Parser::parseNamedType() {
  ETER_DEBUG(llvm::dbgs() << "[" DEBUG_TYPE "] parseNamedType\n");
  llvm::report_fatal_error("TODO: implement Parser::parseNamedType");
}

NodeIndex Parser::parseArrayType() {
  ETER_DEBUG(llvm::dbgs() << "[" DEBUG_TYPE "] parseArrayType\n");
  llvm::report_fatal_error("TODO: implement Parser::parseArrayType");
}

} // namespace eter::parser
