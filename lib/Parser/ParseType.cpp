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

NodeIndex Parser::parseType() {
  ETER_DEBUG(llvm::dbgs() << "[" DEBUG_TYPE "] parseType\n");
  using Kind = lexer::Token::Kind;

  if (check(Kind::l_paren))
    return parseTupleType();

  if (check(Kind::l_square))
    return parseArrayType();

  if (check(Kind::kw_tensor))
    return parseTensorType();

  return parseNamedType();
}

NodeIndex Parser::parseTupleType() {
  ETER_DEBUG(llvm::dbgs() << "[" DEBUG_TYPE "] parseTupleType\n");
  using Kind = lexer::Token::Kind;

  const Span Start = advance().TokenSpan;
  llvm::SmallVector<NodeIndex, 4> Types;
  parseCommaSeparated(Types, Kind::r_paren, [this] { return parseType(); });
  const Span End =
      expect(Kind::r_paren, DiagID::ExpectedTupleTypeClose).TokenSpan;
  return Pool.alloc(NodeKind::TupleType, Span{Start.Start, End.End}, Types);
}

NodeIndex Parser::parseNamedType() {
  ETER_DEBUG(llvm::dbgs() << "[" DEBUG_TYPE "] parseNamedType\n");
  using Kind = lexer::Token::Kind;

  if (!check(Kind::identifier) && !check(Kind::kw_Self)) {
    addError(peekToken().TokenSpan, DiagID::ExpectedTypeName);
    return makeErrorNode(peekToken().TokenSpan);
  }
  Span Full = advance().TokenSpan;

  // Qualified name: name :: name (:: name)*  (e.g. math::Vec).
  parsePathSegments(Full);

  return Pool.allocLeaf(NodeKind::NamedType, Full,
                        Interner.intern(textOf(Full)));
}

NodeIndex Parser::parseArrayType() {
  ETER_DEBUG(llvm::dbgs() << "[" DEBUG_TYPE "] parseArrayType\n");
  using Kind = lexer::Token::Kind;

  const Span Start = advance().TokenSpan; // consume '['

  llvm::SmallVector<NodeIndex, 2> Children;
  Children.push_back(parseType());

  expect(Kind::semi, DiagID::ExpectedArrayTypeSemi);
  Children.push_back(parseConstExpr());

  const Span End =
      expect(Kind::r_square, DiagID::ExpectedArrayTypeClose).TokenSpan;
  return Pool.alloc(NodeKind::ArrayType, Span{Start.Start, End.End}, Children);
}

NodeIndex Parser::parseTensorType() {
  ETER_DEBUG(llvm::dbgs() << "[" DEBUG_TYPE "] parseTensorType\n");
  using Kind = lexer::Token::Kind;

  const Span Start = advance().TokenSpan; // consume 'tensor'

  expect(Kind::l_square, DiagID::ExpectedTensorOpen);

  llvm::SmallVector<NodeIndex, 4> Children;
  Children.push_back(parseType());

  expect(Kind::semi, DiagID::ExpectedTensorTypeSemi);

  // One size per dimension: tensor[i32; 10] is 1D, tensor[f32; 2, 4] is 2D.
  parseCommaSeparated(Children, Kind::r_square,
                      [this] { return parseConstExpr(); });
  if (Children.size() == 1)
    addError(peekToken().TokenSpan, DiagID::ExpectedTensorLitDim);

  const Span End =
      expect(Kind::r_square, DiagID::ExpectedTensorTypeClose).TokenSpan;
  return Pool.alloc(NodeKind::TensorType, Span{Start.Start, End.End}, Children);
}

} // namespace eter::parser
