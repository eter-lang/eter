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
///   [0] : ParamList
///   [1] : ReturnType (NamedType / ArrayType / AttrType), or
///         NullNode when the function returns nothing
///   [2] : BlockExpr (function body)
///
/// Payload: makePayload(functionName, returnRegime)
struct FnDecl : ASTNode<NodeKind::FnDecl> {
  /// Interned name of the function.
  [[nodiscard]] InternedStr getName(const NodePool &P) const;

  /// Regime of the return value (Imm / Mut / Proj / None for void functions).
  [[nodiscard]] Regime getReturnRegime(const NodePool &P) const;

  /// The ParamList node.
  [[nodiscard]] NodeIndex getParamList(const NodePool &P) const;

  /// The return-type node, or NullNode if the function is void.
  [[nodiscard]] NodeIndex getReturnType(const NodePool &P) const;

  /// The function body (a BlockExpr node).
  [[nodiscard]] NodeIndex getBody(const NodePool &P) const;
};

/// Typed view over a `StructDecl` node.
///
/// Child layout: [StructField*]
/// Payload: InternedStr (struct name)
struct StructDecl : ASTNode<NodeKind::StructDecl> {
  [[nodiscard]] InternedStr getName(const NodePool &P) const;
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
/// Child layout: [(EnumVariantUnit | EnumVariantTuple | EnumVariantStruct)*]
/// Payload: InternedStr (enum name)
struct EnumDecl : ASTNode<NodeKind::EnumDecl> {
  [[nodiscard]] InternedStr getName(const NodePool &P) const;
  [[nodiscard]] llvm::ArrayRef<NodeIndex> getVariants(const NodePool &P) const;
};

/// Typed view over a `UnionDecl` node.
///
/// Child layout: [StructField*]
/// Payload: InternedStr (union name)
struct UnionDecl : ASTNode<NodeKind::UnionDecl> {
  [[nodiscard]] InternedStr getName(const NodePool &P) const;
  [[nodiscard]] llvm::ArrayRef<NodeIndex> getFields(const NodePool &P) const;
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

/// Typed view over a `GenericParam` node.
///
/// Child layout: [Type?]  (bound type, NullNode if unbounded)
/// Payload: InternedStr (parameter name, e.g. "T")
struct GenericParam : ASTNode<NodeKind::GenericParam> {
  [[nodiscard]] InternedStr getName(const NodePool &P) const;
  /// The bound type, or NullNode if unbounded.
  [[nodiscard]] NodeIndex getBound(const NodePool &P) const;
};

/// Typed view over a `GenericParamList` node.
///
/// Child layout: [GenericParam*]
/// Payload: 0
struct GenericParamList : ASTNode<NodeKind::GenericParamList> {
  [[nodiscard]] llvm::ArrayRef<NodeIndex> getParams(const NodePool &P) const;
};

/// Typed view over a `GenericArgList` node.
///
/// Child layout: [Type*]
/// Payload: 0
struct GenericArgList : ASTNode<NodeKind::GenericArgList> {
  [[nodiscard]] llvm::ArrayRef<NodeIndex> getArgs(const NodePool &P) const;
};

/// Typed view over a `UseDecl` node.
///
/// Child layout: [DocComment*]
/// Payload: InternedStr (full path text, e.g. "foo::bar::Baz")
/// Flags: PubFlag (if `pub use`)
struct UseDecl : ASTNode<NodeKind::UseDecl> {
  /// The full use path (e.g. "foo::bar::Baz").
  [[nodiscard]] InternedStr getPath(const NodePool &P) const;
};

//===----------------------------------------------------------------------===//
// Statements
//===----------------------------------------------------------------------===//

/// Typed view over a `LetStmt` node.
///
/// Child layout: [Type?, Expr?]
///   Both the type annotation and the initialiser are optional. The children
///   that are present appear in that order and are distinguished by node
///   kind (type nodes vs expression nodes).
/// Payload: makePayload(bindingName, regime)
struct LetStmt : ASTNode<NodeKind::LetStmt> {
  [[nodiscard]] InternedStr getName(const NodePool &P) const;
  [[nodiscard]] Regime getRegime(const NodePool &P) const;
  /// Explicit type annotation, or NullNode if absent.
  [[nodiscard]] NodeIndex getTypeAnnotation(const NodePool &P) const;
  /// The initialiser expression, or NullNode if absent.
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
/// Mut value. op == Token::Kind::star represents `*expr` — dereference.
/// The distinct `ProjAssignExpr` covers `*lhs = rhs`.
struct UnaryExpr : ASTNode<NodeKind::UnaryExpr> {
  [[nodiscard]] NodeIndex getOperand(const NodePool &P) const;
  [[nodiscard]] lexer::Token::Kind getOp(const NodePool &P) const;
};

/// Typed view over a `ProjAssignExpr` node (`*lhs = rhs`).
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

/// Typed view over a `CastExpr` node (`expr as Type`).
///
/// Child layout: [Expr, Type]
/// Payload: 0
struct CastExpr : ASTNode<NodeKind::CastExpr> {
  /// The expression being cast.
  [[nodiscard]] NodeIndex getExpr(const NodePool &P) const;
  /// The target type.
  [[nodiscard]] NodeIndex getType(const NodePool &P) const;
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

/// Typed view over a `TupleExpr` node: ( Expr, Expr, ... )
///
/// Child layout: [Expr*] (empty = the unit value `()`)
/// Payload: 0
struct TupleExpr : ASTNode<NodeKind::TupleExpr> {
  [[nodiscard]] llvm::ArrayRef<NodeIndex> getElements(const NodePool &P) const;
};

/// Typed view over a `TensorIndexExpr` node: Expr [ Expr, Expr, ... ]
///
/// Used for multi-dimensional tensor access with 2+ comma-separated indices.
/// For single-index access (including 1D tensors) use IndexExpr.
/// Child layout: [Expr (base), Expr+ (indices, one per dimension)]
/// Payload: 0
struct TensorIndexExpr : ASTNode<NodeKind::TensorIndexExpr> {
  [[nodiscard]] NodeIndex getBase(const NodePool &P) const;
  [[nodiscard]] llvm::ArrayRef<NodeIndex> getIndices(const NodePool &P) const;
};

/// Typed view over a `TupleIndexExpr` node: Expr . IntegerLiteral
///
/// Child layout: [Expr (base)]
/// Payload: InternedStr (index literal text, e.g. "0")
struct TupleIndexExpr : ASTNode<NodeKind::TupleIndexExpr> {
  [[nodiscard]] NodeIndex getBase(const NodePool &P) const;
  [[nodiscard]] InternedStr getIndexText(const NodePool &P) const;
};

/// Typed view over a `StructLitExpr` node.
///
/// Child layout: [FieldInit*]
/// Payload: InternedStr (struct name)
struct StructLitExpr : ASTNode<NodeKind::StructLitExpr> {
  [[nodiscard]] InternedStr getName(const NodePool &P) const;
  [[nodiscard]] llvm::ArrayRef<NodeIndex>
  getFieldInits(const NodePool &P) const;
};

/// Typed view over a `FieldInit` node (`name: Expr`, or shorthand `name`).
///
/// Child layout: [Expr] (for the shorthand, a synthesised IdentExpr)
/// Payload: InternedStr (field name)
struct FieldInit : ASTNode<NodeKind::FieldInit> {
  [[nodiscard]] InternedStr getFieldName(const NodePool &P) const;
  [[nodiscard]] NodeIndex getExpr(const NodePool &P) const;
};

/// Typed view over an `IdentExpr` node.
///
/// Child layout: []
/// Payload: InternedStr (identifier name)
struct IdentExpr : ASTNode<NodeKind::IdentExpr> {
  [[nodiscard]] InternedStr getName(const NodePool &P) const;
};

/// Typed view over a `PathExpr` node (e.g. Animals::Cat, u32::MAX).
///
/// Child layout: []
/// Payload: InternedStr (full path text, `::`-separated)
struct PathExpr : ASTNode<NodeKind::PathExpr> {
  [[nodiscard]] InternedStr getPath(const NodePool &P) const;
};

/// Typed view over an `ArrayLitExpr` node: [ Expr* ]
///
/// Child layout: [Expr*]
/// Payload: 0
struct ArrayLitExpr : ASTNode<NodeKind::ArrayLitExpr> {
  [[nodiscard]] llvm::ArrayRef<NodeIndex> getElements(const NodePool &P) const;
};

/// Typed view over an `ArrayRepeatExpr` node: [ Expr ; ConstExpr ]
///
/// Child layout: [Expr (value), ConstExpr (count)]
/// Payload: 0
struct ArrayRepeatExpr : ASTNode<NodeKind::ArrayRepeatExpr> {
  [[nodiscard]] NodeIndex getValue(const NodePool &P) const;
  [[nodiscard]] NodeIndex getCount(const NodePool &P) const;
};

/// Typed view over a `TensorLitExpr` node.
///
/// Repeat form:  tensor [ Expr ; ConstExpr (, ConstExpr)* ]
/// Flat form:    tensor [ Expr (, Expr)* ; ConstExpr (, ConstExpr)* ]
/// Child layout: [Expr+ (values), ConstExpr+ (dims)]
/// Payload: number of value children (uint16, via NodePool::payloadOp)
struct TensorLitExpr : ASTNode<NodeKind::TensorLitExpr> {
  [[nodiscard]] uint16_t getValueCount(const NodePool &P) const;
  [[nodiscard]] llvm::ArrayRef<NodeIndex> getValues(const NodePool &P) const;
  [[nodiscard]] llvm::ArrayRef<NodeIndex> getDims(const NodePool &P) const;
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

/// Typed view over an `ArrayType` node: [ Type ; ConstExpr ]
///
/// 1D only. For multi-dimensional tensors use TensorType.
/// Child layout: [Type (element type), ConstExpr (size)]
/// Payload: 0
struct ArrayType : ASTNode<NodeKind::ArrayType> {
  [[nodiscard]] NodeIndex getElementType(const NodePool &P) const;
  [[nodiscard]] NodeIndex getSizeExpr(const NodePool &P) const;
};

/// Typed view over a `TensorType` node: tensor [ Type ; ConstExpr (,
/// ConstExpr)* ]
///
/// Stack-allocated mathematical value type with data/shape/stride/offset.
/// Supports O(1) slicing, transpose, and broadcasting.
/// Child layout: [Type (element type), ConstExpr+ (one size per dimension)]
/// Payload: 0
struct TensorType : ASTNode<NodeKind::TensorType> {
  [[nodiscard]] NodeIndex getElementType(const NodePool &P) const;
  [[nodiscard]] llvm::ArrayRef<NodeIndex> getSizeExprs(const NodePool &P) const;
};

/// Typed view over a `TupleType` node: ( Type, Type, ... )
///
/// Child layout: [Type*] (empty = the unit type `()`)
/// Payload: 0
struct TupleType : ASTNode<NodeKind::TupleType> {
  [[nodiscard]] llvm::ArrayRef<NodeIndex>
  getElementTypes(const NodePool &P) const;
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

//===----------------------------------------------------------------------===//
// Traits and Impls
//===----------------------------------------------------------------------===//

/// Typed view over a `TraitDecl` node.
///
/// Child layout: [DocComment*, FnDecl*]
/// Payload: InternedStr (trait name)
struct TraitDecl : ASTNode<NodeKind::TraitDecl> {
  [[nodiscard]] InternedStr getName(const NodePool &P) const;
  [[nodiscard]] llvm::ArrayRef<NodeIndex> getMethods(const NodePool &P) const;
};

/// Typed view over an `ImplDecl` node.
///
/// Child layout: [DocComment*, TraitType?, Type, FnDecl*]
///   TraitType is NullNode for inherent impls (impl Type { ... }).
///   For trait impls (impl Trait for Type { ... }), TraitType is the
///   trait's NamedType/PathExpr node.
/// Payload: 0
struct ImplDecl : ASTNode<NodeKind::ImplDecl> {
  /// The trait type, or NullNode for inherent impls.
  [[nodiscard]] NodeIndex getTrait(const NodePool &P) const;
  /// The implementing type.
  [[nodiscard]] NodeIndex getType(const NodePool &P) const;
  /// The method implementations.
  [[nodiscard]] llvm::ArrayRef<NodeIndex> getMethods(const NodePool &P) const;
};

/// Typed view over an `UnsafeBlock` node.
///
/// Child layout: [Stmt*, Expr?]  (same layout as BlockExpr)
///   The optional final child without a trailing `;` is the block's yield
///   value. All preceding children are statements.
/// Payload: 0
struct UnsafeBlock : ASTNode<NodeKind::UnsafeBlock> {
  [[nodiscard]] llvm::ArrayRef<NodeIndex> getStmts(const NodePool &P) const;
  [[nodiscard]] NodeIndex getTailExpr(const NodePool &P) const;
};

} // namespace eter::parser

#endif // ETER_PARSER_ASTNODES_H
