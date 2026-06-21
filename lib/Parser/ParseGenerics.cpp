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

#define DEBUG_TYPE "parser-generics"

namespace eter::parser {

NodeIndex Parser::parseGenericParams() {
  ETER_DEBUG(llvm::dbgs() << "[" DEBUG_TYPE "] parseGenericParams\n");
  using Kind = lexer::Token::Kind;

  if (!check(Kind::less))
    return NullNode;

  const Span Start = advance().TokenSpan;

  llvm::SmallVector<NodeIndex, 4> Params;
  if (!check(Kind::greater)) {
    Params.push_back(parseGenericParam());
    while (consume(Kind::comma)) {
      if (check(Kind::greater))
        break;
      Params.push_back(parseGenericParam());
    }
  }

  const Span End = expect(Kind::greater, DiagID::ExpectedGreaterThan).TokenSpan;
  return Pool.alloc(NodeKind::GenericParamList, Span{Start.Start, End.End},
                    Params);
}

NodeIndex Parser::parseGenericParam() {
  ETER_DEBUG(llvm::dbgs() << "[" DEBUG_TYPE "] parseGenericParam\n");
  using Kind = lexer::Token::Kind;

  const uint32_t StartPos = peekToken().TokenSpan.Start;

  if (!check(Kind::identifier)) {
    addError(peekToken().TokenSpan, DiagID::ExpectedTypeParam);
    return makeErrorNode(peekToken().TokenSpan);
  }

  const InternedStr Name = Interner.intern(textOf(advance().TokenSpan));

  llvm::SmallVector<NodeIndex, 1> Children;
  if (consume(Kind::colon)) {
    Children.push_back(parseType());
  } else {
    Children.push_back(NullNode);
  }

  return Pool.alloc(NodeKind::GenericParam,
                    Span{StartPos, Stream.previous().TokenSpan.End}, Children,
                    Name);
}

NodeIndex Parser::parseGenericArgs() {
  ETER_DEBUG(llvm::dbgs() << "[" DEBUG_TYPE "] parseGenericArgs\n");
  using Kind = lexer::Token::Kind;

  const Span Start = advance().TokenSpan;

  llvm::SmallVector<NodeIndex, 4> Args;
  if (!check(Kind::greater)) {
    Args.push_back(parseType());
    while (consume(Kind::comma)) {
      if (check(Kind::greater))
        break;
      Args.push_back(parseType());
    }
  }

  const Span End = expect(Kind::greater, DiagID::ExpectedGreaterThan).TokenSpan;
  return Pool.alloc(NodeKind::GenericArgList, Span{Start.Start, End.End}, Args);
}

} // namespace eter::parser
