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
                                 uint32_t Flags) {
  ETER_DEBUG(llvm::dbgs() << "[" DEBUG_TYPE "] parseConstDecl\n");
  using Kind = lexer::Token::Kind;

  const Span Start = expect(Kind::kw_const, DiagID::ExpectedKeyword).TokenSpan;

  const InternedStr Name = expectName(Kind::identifier, "constant");

  expect(Kind::colon, DiagID::ExpectedColon, "after constant name");
  const NodeIndex Type = parseType();

  expect(Kind::eq, DiagID::ExpectedEq, "in constant declaration");
  const NodeIndex Val = parseConstExpr();

  expect(Kind::semi, DiagID::ExpectedSemi, "after constant declaration");

  llvm::SmallVector<NodeIndex, 8> Children(Docs.begin(), Docs.end());
  Children.push_back(Type);
  Children.push_back(Val);
  return Pool.alloc(NodeKind::ConstDecl,
                    Span{Start.Start, Stream.previous().TokenSpan.End},
                    Children, Name, Flags);
}

NodeIndex Parser::parseConstExpr(int MinBP) {
  ETER_DEBUG(llvm::dbgs() << "[" DEBUG_TYPE "] parseConstExpr minBP=" << MinBP
                          << "\n");

  return parseExpr(MinBP);
}

} // namespace eter::parser
