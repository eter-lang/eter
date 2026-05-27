//===----------------------------------------------------------------------===//
//
// Part of the Eter Project, under the Apache License v2.0 with LLVM Exceptions.
// See /LICENSE for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef ETER_PARSER_ASTNODES_H
#define ETER_PARSER_ASTNODES_H

#include "eter/Base/StringInterner.h"
#include "eter/Lexer/Token.h"
#include "eter/Parser/NodePool.h"
#include "eter/Parser/Regime.h"

#include <llvm/ADT/ArrayRef.h>

namespace eter::parser {

//===----------------------------------------------------------------------===//
// Helpers
//===----------------------------------------------------------------------===//

/// Assert (in debug builds) that node `I` in pool `P` has kind `Expected`.
/// Call this at the top of every typed-view method that accesses children or
/// payload, so misuse is caught immediately rather than silently producing
/// wrong results.
void assertKind(const NodePool &P, NodeIndex I, NodeKind Expected);

/// Base for typed AST node views.
/// Each concrete view inherits from ASTNode<corresponding NodeKind> to get the
/// Idx member and a shared classof() implementation without vtables.
template <NodeKind K> struct ASTNode {
  NodeIndex Idx;

  [[nodiscard]] static bool classof(const NodePool &P, NodeIndex I) {
    return P.kindOf(I) == K;
  }
};

//===----------------------------------------------------------------------===//
// Declarations
//===----------------------------------------------------------------------===//

/// Typed view over a `FnDecl` node.
///
/// Child layout (childrenOf indices):
///   [0 .. nAttrs-1] : Attribute nodes (zero or more, in source order)
///   [nAttrs]        : ParamList
///   [nAttrs+1]      : ReturnType (NamedType / ArrayType / AttrType), or
///                     NullNode when the function returns nothing
///   [nAttrs+2]      : BlockExpr (function body)
///
/// Payload: makePayload(functionName, returnRegime)
struct FnDecl : ASTNode<NodeKind::FnDecl> {
  /// Interned name of the function.
  [[nodiscard]] InternedStr getName(const NodePool &P) const;

  /// Regime of the return value (Imm / Mut / Proj / None for void functions).
  [[nodiscard]] Regime getReturnRegime(const NodePool &P) const;

  /// Declaration-level @-attributes, in source order.
  [[nodiscard]] llvm::ArrayRef<NodeIndex>
  getAttributes(const NodePool &P) const;

  /// The ParamList node.
  [[nodiscard]] NodeIndex getParamList(const NodePool &P) const;

  /// The return-type node, or NullNode if the function is void.
  [[nodiscard]] NodeIndex getReturnType(const NodePool &P) const;

  /// The function body (a BlockExpr node).
  [[nodiscard]] NodeIndex getBody(const NodePool &P) const;
};

/// Typed view over a `StructDecl` node.
///
/// Child layout: [Attribute*, StructField*]
/// Payload: InternedStr (struct name)
struct StructDecl : ASTNode<NodeKind::StructDecl> {
  [[nodiscard]] InternedStr getName(const NodePool &P) const;
  [[nodiscard]] llvm::ArrayRef<NodeIndex>
  getAttributes(const NodePool &P) const;
  [[nodiscard]] llvm::ArrayRef<NodeIndex> getFields(const NodePool &P) const;
};

/// Typed view over a `StructField` node.
///
/// Child layout: [Type]
/// Payload: makePayload(fieldName, regime)
struct StructField : ASTNode<NodeKind::StructField> {
  [[nodiscard]] InternedStr getName(const NodePool &P) const;
  [[nodiscard]] Regime getRegime(const NodePool &P) const;
  /// The declared type of this field.
  [[nodiscard]] NodeIndex getType(const NodePool &P) const;
};

/// Typed view over an `EnumDecl` node.
///
/// Child layout: [Attribute*, (EnumVariantUnit | EnumVariantTuple |
///                             EnumVariantStruct)*]
/// Payload: InternedStr (enum name)
struct EnumDecl : ASTNode<NodeKind::EnumDecl> {
  [[nodiscard]] InternedStr getName(const NodePool &P) const;
  [[nodiscard]] llvm::ArrayRef<NodeIndex>
  getAttributes(const NodePool &P) const;
  [[nodiscard]] llvm::ArrayRef<NodeIndex> getVariants(const NodePool &P) const;
};

/// Typed view over a `Param` node.
///
/// Child layout: [Type]
/// Payload: makePayload(paramName, regime)
struct Param : ASTNode<NodeKind::Param> {
  [[nodiscard]] InternedStr getName(const NodePool &P) const;
  [[nodiscard]] Regime getRegime(const NodePool &P) const;
  [[nodiscard]] NodeIndex getType(const NodePool &P) const;
};

/// Typed view over a `ParamList` node.
///
/// Child layout: [Param*]
/// Payload: 0
struct ParamList : ASTNode<NodeKind::ParamList> {
  [[nodiscard]] llvm::ArrayRef<NodeIndex> getParams(const NodePool &P) const;
};

//===----------------------------------------------------------------------===//
// Statements
//===----------------------------------------------------------------------===//

/// Typed view over a `LetStmt` node.
///
/// Child layout: [Type?, Expr]
///   ChildCount == 1: no type annotation; child[0] = Expr (initialiser).
///   ChildCount == 2: child[0] = Type annotation, child[1] = Expr.
/// Payload: makePayload(bindingName, regime)
struct LetStmt : ASTNode<NodeKind::LetStmt> {
  [[nodiscard]] InternedStr getName(const NodePool &P) const;
  [[nodiscard]] Regime getRegime(const NodePool &P) const;
  /// Explicit type annotation, or NullNode if absent.
  [[nodiscard]] NodeIndex getTypeAnnotation(const NodePool &P) const;
  /// The initialiser expression (always present; may be a NodeKind::Error).
  [[nodiscard]] NodeIndex getInitExpr(const NodePool &P) const;
};

/// Typed view over a `RetStmt` node.
///
/// Child layout: [Expr?]  (zero children = bare `ret;`)
/// Payload: 0
struct RetStmt : ASTNode<NodeKind::RetStmt> {
  /// The return value expression, or NullNode for a bare `ret;`.
  [[nodiscard]] NodeIndex getExpr(const NodePool &P) const;
};

/// Typed view over a `BlockExpr` node.
///
/// Child layout: [Stmt*, Expr?]
///   The optional final child without a trailing `;` is the block's yield
///   value.  All preceding children are statements.
/// Payload: 0
struct BlockExpr : ASTNode<NodeKind::BlockExpr> {
  [[nodiscard]] llvm::ArrayRef<NodeIndex> getStmts(const NodePool &P) const;
  /// Tail expression (block yield value), or NullNode if absent.
  [[nodiscard]] NodeIndex getTailExpr(const NodePool &P) const;
};

/// Typed view over an `IfExpr` node.
///
/// Child layout: [Expr (cond), BlockExpr (then), BlockExpr|IfExpr? (else)]
///   ChildCount == 2: no else branch.
///   ChildCount == 3: child[2] is a BlockExpr or a nested IfExpr.
/// Payload: 0
struct IfExpr : ASTNode<NodeKind::IfExpr> {
  [[nodiscard]] NodeIndex getCondition(const NodePool &P) const;
  [[nodiscard]] NodeIndex getThenBranch(const NodePool &P) const;
  /// The else branch, or NullNode if absent.
  [[nodiscard]] NodeIndex getElseBranch(const NodePool &P) const;
};

//===----------------------------------------------------------------------===//
// Expressions
//===----------------------------------------------------------------------===//

/// Typed view over a `BinaryExpr` node.
///
/// Child layout: [Expr (lhs), Expr (rhs)]
/// Payload: makeOpPayload(static_cast<uint16_t>(Token::Kind))
struct BinaryExpr : ASTNode<NodeKind::BinaryExpr> {
  [[nodiscard]] NodeIndex getLhs(const NodePool &P) const;
  [[nodiscard]] NodeIndex getRhs(const NodePool &P) const;
  [[nodiscard]] lexer::Token::Kind getOp(const NodePool &P) const;
};

/// Typed view over a `UnaryExpr` node.
///
/// Child layout: [Expr]
/// Payload: makeOpPayload(static_cast<uint16_t>(Token::Kind))
///
/// Note: op == Token::Kind::amp represents `&expr` — taking a projection of a
/// Mut value (the rhs use of `&`). The distinct `ProjAssignExpr` covers the
/// lhs use `&lhs = rhs`.
struct UnaryExpr : ASTNode<NodeKind::UnaryExpr> {
  [[nodiscard]] NodeIndex getOperand(const NodePool &P) const;
  [[nodiscard]] lexer::Token::Kind getOp(const NodePool &P) const;
};

/// Typed view over a `ProjAssignExpr` node (`&lhs = rhs`).
///
/// Write a value through a projection variable or projected field path.
/// This is semantically distinct from a regular assignment (BinaryExpr with
/// op = Token::Kind::eq).
///
/// Child layout: [Expr (projection target), Expr (rhs)]
/// Payload: 0
struct ProjAssignExpr : ASTNode<NodeKind::ProjAssignExpr> {
  /// The projection target (an IdentExpr, or a FieldExpr / IndexExpr chain).
  [[nodiscard]] NodeIndex getTarget(const NodePool &P) const;
  [[nodiscard]] NodeIndex getRhs(const NodePool &P) const;
};

/// Typed view over a `CallExpr` node.
///
/// Child layout: [Expr (callee), ArgList]
/// Payload: 0
struct CallExpr : ASTNode<NodeKind::CallExpr> {
  [[nodiscard]] NodeIndex getCallee(const NodePool &P) const;
  [[nodiscard]] llvm::ArrayRef<NodeIndex> getArgs(const NodePool &P) const;
};

/// Typed view over a `FieldExpr` node.
///
/// Child layout: [Expr (base)]
/// Payload: InternedStr (field name)
struct FieldExpr : ASTNode<NodeKind::FieldExpr> {
  [[nodiscard]] NodeIndex getBase(const NodePool &P) const;
  [[nodiscard]] InternedStr getFieldName(const NodePool &P) const;
};

/// Typed view over an `IdentExpr` node.
///
/// Child layout: []
/// Payload: InternedStr (identifier name)
struct IdentExpr : ASTNode<NodeKind::IdentExpr> {
  [[nodiscard]] InternedStr getName(const NodePool &P) const;
};

/// Typed view over a `LitExpr` node.
///
/// Child layout: []
/// Payload: InternedStr (source text of the literal as written, e.g. "42",
/// "3.14f", "true", "'a'"). The semantic type is determined by the type
/// checker, not the parser.
struct LitExpr : ASTNode<NodeKind::LitExpr> {
  [[nodiscard]] InternedStr getText(const NodePool &P) const;
};

//===----------------------------------------------------------------------===//
// Types
//===----------------------------------------------------------------------===//

/// Typed view over a `NamedType` node (e.g. i32, f32, Point).
///
/// Child layout: []
/// Payload: InternedStr (type name)
struct NamedType : ASTNode<NodeKind::NamedType> {
  [[nodiscard]] InternedStr getName(const NodePool &P) const;
};

/// Typed view over an `ArrayType` node: [ Type ; Expr ]
///
/// Child layout: [Type (element type), Expr (size)]
/// Payload: 0
struct ArrayType : ASTNode<NodeKind::ArrayType> {
  [[nodiscard]] NodeIndex getElementType(const NodePool &P) const;
  [[nodiscard]] NodeIndex getSizeExpr(const NodePool &P) const;
};

/// Typed view over an `AttrType` node: Type @qualifier
///
/// Child layout: [Type (inner type)]
/// Payload: InternedStr (qualifier name, e.g. "global", "shared", "host",
/// "constant")
struct AttrType : ASTNode<NodeKind::AttrType> {
  [[nodiscard]] NodeIndex getInnerType(const NodePool &P) const;
  [[nodiscard]] InternedStr getQualifier(const NodePool &P) const;
};

//===----------------------------------------------------------------------===//
// Match
//===----------------------------------------------------------------------===//

/// Typed view over a `MatchExpr` node.
///
/// Child layout: [Expr (scrutinee), MatchArm*]
/// Payload: 0
struct MatchExpr : ASTNode<NodeKind::MatchExpr> {
  [[nodiscard]] NodeIndex getScrutinee(const NodePool &P) const;
  [[nodiscard]] llvm::ArrayRef<NodeIndex> getArms(const NodePool &P) const;
};

/// Typed view over a `MatchArm` node: Pat => Expr
///
/// Child layout: [Pat, Expr]
/// Payload: 0
struct MatchArm : ASTNode<NodeKind::MatchArm> {
  [[nodiscard]] NodeIndex getPattern(const NodePool &P) const;
  [[nodiscard]] NodeIndex getBody(const NodePool &P) const;
};

} // namespace eter::parser

#endif // ETER_PARSER_ASTNODES_H
