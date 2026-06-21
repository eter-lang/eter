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

NodeIndex Parser::parseTopLevelDecl() {
  ETER_DEBUG(llvm::dbgs() << "[" DEBUG_TYPE "] parseTopLevelDecl\n");
  using Kind = lexer::Token::Kind;

  const llvm::SmallVector<NodeIndex, 4> Docs = parseDocComments();

  uint32_t Flags = 0;
  if (consume(Kind::kw_pub))
    Flags |= NodePool::PubFlag;

  switch (peek()) {
  case Kind::kw_fn:
    return parseFnDecl(Docs, Flags);
  case Kind::kw_unsafe:
    if (peek(1) == Kind::kw_fn) {
      advance(); // consume 'unsafe'
      return parseFnDecl(Docs, Flags | NodePool::UnsafeFlag);
    }
    [[fallthrough]];
  case Kind::kw_const:
    return parseConstDecl(Docs, Flags);
  case Kind::kw_mod:
    return parseModDecl(Docs, Flags);
  case Kind::kw_struct:
    return parseStructDecl(Docs, Flags);
  case Kind::kw_enum:
    return parseEnumDecl(Docs, Flags);
  case Kind::kw_union:
    return parseUnionDecl(Docs, Flags);
  case Kind::kw_use:
    return parseUseDecl(Docs, Flags);
  case Kind::kw_trait:
    return parseTraitDecl(Docs, Flags);
  case Kind::kw_impl:
    return parseImplDecl(Docs, Flags);
  default: {
    const lexer::Token Tok = peekToken();
    if (!Docs.empty()) {
      addError(Tok.TokenSpan, DiagID::DocCommentNotAllowed);
    }
    addError(Tok.TokenSpan, DiagID::ExpectedTopLevelDecl);
    synchronize();
    return makeErrorNode(Tok.TokenSpan);
  }
  }
}

NodeIndex Parser::parseFnDecl(llvm::ArrayRef<NodeIndex> Docs, uint32_t Flags,
                              bool ExpectBody) {
  ETER_DEBUG(llvm::dbgs() << "[" DEBUG_TYPE "] parseFnDecl\n");
  using Kind = lexer::Token::Kind;

  const uint32_t StartPos = peekToken().TokenSpan.Start;
  if (consume(Kind::kw_unsafe))
    Flags |= NodePool::UnsafeFlag;
  const Span Start{StartPos,
                   expect(Kind::kw_fn, DiagID::ExpectedKeyword).TokenSpan.End};

  const Regime ReturnRegime = parseRegime();

  const InternedStr Name = expectName(Kind::identifier, "function");

  const NodeIndex GenericParams = parseGenericParams();

  llvm::SmallVector<NodeIndex, 8> Children(Docs.begin(), Docs.end());
  if (GenericParams != NullNode)
    Children.push_back(GenericParams);
  Children.push_back(parseParamList());

  if (consume(Kind::colon)) {
    Children.push_back(parseType());
  } else if (check(Kind::arrow)) {
    const lexer::Token Arrow = peekToken();
    const auto Found = lexer::Token::getTokenName(Arrow.TokenKind);
    diag(Arrow.TokenSpan, DiagID::ExpectedReturnTypeColon)
        .arg("found", Found.str());
    advance();
    Children.push_back(parseType());
  }

  if (ExpectBody) {
    Children.push_back(parseBlockExpr());
  } else {
    expect(Kind::semi, DiagID::ExpectedTraitFnSemi);
    Children.push_back(NullNode);
  }

  return Pool.alloc(NodeKind::FnDecl,
                    Span{Start.Start, Stream.previous().TokenSpan.End},
                    Children, NodePool::makePayload(Name, ReturnRegime), Flags);
}

NodeIndex Parser::parseStructDecl(llvm::ArrayRef<NodeIndex> Docs,
                                  uint32_t Flags) {
  ETER_DEBUG(llvm::dbgs() << "[" DEBUG_TYPE "] parseStructDecl\n");
  using Kind = lexer::Token::Kind;

  const Span Start = expect(Kind::kw_struct, DiagID::ExpectedKeyword).TokenSpan;

  const InternedStr Name = expectName(Kind::identifier, "struct");

  const NodeIndex GenericParams = parseGenericParams();

  expect(Kind::l_brace, DiagID::ExpectedOpen, "to start struct body");

  llvm::SmallVector<NodeIndex, 8> Children(Docs.begin(), Docs.end());
  if (GenericParams != NullNode)
    Children.push_back(GenericParams);
  parseCommaSeparated(Children, Kind::r_brace,
                      [this] { return parseStructField(); });

  const Span End =
      expect(Kind::r_brace, DiagID::ExpectedClose, "to close struct body")
          .TokenSpan;
  return Pool.alloc(NodeKind::StructDecl, Span{Start.Start, End.End}, Children,
                    Name, Flags);
}

NodeIndex Parser::parseEnumDecl(llvm::ArrayRef<NodeIndex> Docs,
                                uint32_t Flags) {
  ETER_DEBUG(llvm::dbgs() << "[" DEBUG_TYPE "] parseEnumDecl\n");
  using Kind = lexer::Token::Kind;

  const Span Start = expect(Kind::kw_enum, DiagID::ExpectedKeyword).TokenSpan;

  const InternedStr Name = expectName(Kind::identifier, "enum");

  const NodeIndex GenericParams = parseGenericParams();

  expect(Kind::l_brace, DiagID::ExpectedOpen, "to start enum body");

  llvm::SmallVector<NodeIndex, 8> Children(Docs.begin(), Docs.end());
  if (GenericParams != NullNode)
    Children.push_back(GenericParams);
  parseCommaSeparated(Children, Kind::r_brace,
                      [this] { return parseEnumVariant(); });

  const Span End =
      expect(Kind::r_brace, DiagID::ExpectedClose, "to close enum body")
          .TokenSpan;
  return Pool.alloc(NodeKind::EnumDecl, Span{Start.Start, End.End}, Children,
                    Name, Flags);
}

NodeIndex Parser::parseUnionDecl(llvm::ArrayRef<NodeIndex> Docs,
                                 uint32_t Flags) {
  ETER_DEBUG(llvm::dbgs() << "[" DEBUG_TYPE "] parseUnionDecl\n");
  using Kind = lexer::Token::Kind;

  const Span Start = expect(Kind::kw_union, DiagID::ExpectedKeyword).TokenSpan;

  const InternedStr Name = expectName(Kind::identifier, "union");

  const NodeIndex GenericParams = parseGenericParams();

  expect(Kind::l_brace, DiagID::ExpectedOpen, "to start union body");

  llvm::SmallVector<NodeIndex, 8> Children(Docs.begin(), Docs.end());
  if (GenericParams != NullNode)
    Children.push_back(GenericParams);
  parseCommaSeparated(Children, Kind::r_brace,
                      [this] { return parseStructField(); });

  const Span End =
      expect(Kind::r_brace, DiagID::ExpectedClose, "to close union body")
          .TokenSpan;
  return Pool.alloc(NodeKind::UnionDecl, Span{Start.Start, End.End}, Children,
                    Name, Flags);
}

NodeIndex Parser::parseModDecl(llvm::ArrayRef<NodeIndex> Docs, uint32_t Flags) {
  ETER_DEBUG(llvm::dbgs() << "[" DEBUG_TYPE "] parseModDecl\n");
  using Kind = lexer::Token::Kind;

  const Span Start = expect(Kind::kw_mod, DiagID::ExpectedKeyword).TokenSpan;

  const InternedStr Name = expectName(Kind::identifier, "module");

  if (consume(Kind::l_brace)) {
    // Inline module: mod name { TopLevelDecl* }
    llvm::SmallVector<NodeIndex, 8> Children(Docs.begin(), Docs.end());
    while (!check(Kind::r_brace) && !atEof())
      Children.push_back(parseTopLevelDecl());
    const Span End =
        expect(Kind::r_brace, DiagID::ExpectedClose, "to close module body")
            .TokenSpan;
    return Pool.alloc(NodeKind::ModDecl, Span{Start.Start, End.End}, Children,
                      Name, Flags);
  }

  if (check(Kind::semi)) {
    const Span End = peekToken().TokenSpan;
    advance(); // consume ';'
    const llvm::SmallVector<NodeIndex, 4> Children(Docs.begin(), Docs.end());
    return Pool.alloc(NodeKind::ModDeclFile, Span{Start.Start, End.End},
                      Children, Name, Flags);
  }

  addError(peekToken().TokenSpan, DiagID::ExpectedModOpenOrSemi);
  return makeErrorNode(peekToken().TokenSpan);
}

NodeIndex Parser::parseUseDecl(llvm::ArrayRef<NodeIndex> Docs, uint32_t Flags) {
  ETER_DEBUG(llvm::dbgs() << "[" DEBUG_TYPE "] parseUseDecl\n");
  using Kind = lexer::Token::Kind;

  const Span Start = expect(Kind::kw_use, DiagID::ExpectedKeyword).TokenSpan;

  // Parse path segments manually so we can stop before ::* or alias 'as'.
  // Each iteration: consume an identifier, then peek at what follows.
  expect(Kind::identifier, DiagID::ExpectedIdentifier, "in use path");
  Span PathSpan = Stream.previous().TokenSpan;

  // Consume intermediate :: identifier segments, stopping before ::* or end.
  while (check(Kind::colon_colon)) {
    // Peek ahead: if next is *, stop here and let the glob handling below run.
    if (peek(1) == Kind::star)
      break;
    // If next is not an identifier, stop and let the error surface below.
    if (peek(1) != Kind::identifier)
      break;
    advance(); // consume '::'
    advance(); // consume identifier
    PathSpan.End = Stream.previous().TokenSpan.End;
  }

  Span EndSpan = PathSpan;
  InternedStr FullPath = Interner.intern(textOf(PathSpan));

  if (check(Kind::colon_colon)) {
    advance(); // consume '::'
    if (consume(Kind::star)) {
      EndSpan = Stream.previous().TokenSpan;
      FullPath = Interner.intern(textOf(Span{PathSpan.Start, EndSpan.End}));
    } else {
      addError(peekToken().TokenSpan, DiagID::ExpectedUseItemOrGlob);
    }
  } else if (consume(Kind::kw_as)) {
    expectAndIntern(Kind::identifier, DiagID::ExpectedAsAlias);
    EndSpan = Stream.previous().TokenSpan;
    FullPath = Interner.intern(textOf(Span{PathSpan.Start, EndSpan.End}));
  }

  expect(Kind::semi, DiagID::ExpectedSemi, "after use declaration");

  const llvm::SmallVector<NodeIndex, 4> Children(Docs.begin(), Docs.end());
  return Pool.alloc(NodeKind::UseDecl, Span{Start.Start, EndSpan.End}, Children,
                    FullPath, Flags);
}

NodeIndex Parser::parseParamList() {
  ETER_DEBUG(llvm::dbgs() << "[" DEBUG_TYPE "] parseParamList\n");
  using Kind = lexer::Token::Kind;

  const Span Start =
      expect(Kind::l_paren, DiagID::ExpectedOpen, "to start parameter list")
          .TokenSpan;

  llvm::SmallVector<NodeIndex, 8> Children;
  if (!check(Kind::r_paren)) {
    Children.push_back(parseParam());
    while (consume(Kind::comma))
      Children.push_back(parseParam());
  }

  const Span End =
      expect(Kind::r_paren, DiagID::ExpectedClose, "to close parameter list")
          .TokenSpan;
  return Pool.alloc(NodeKind::ParamList, Span{Start.Start, End.End}, Children);
}

NodeIndex Parser::parseParam() {
  ETER_DEBUG(llvm::dbgs() << "[" DEBUG_TYPE "] parseParam\n");
  using Kind = lexer::Token::Kind;

  const uint32_t StartPos = peekToken().TokenSpan.Start;

  const Regime R = parseRegime();

  const InternedStr Name = expectName(Kind::identifier, "parameter");

  // In methods, `self` may omit the type annotation (defaults to `Self`).
  const bool IsSelfParam = Interner.get(Name) == "self" &&
                           !check(Kind::colon) &&
                           (check(Kind::r_paren) || check(Kind::comma));

  if (IsSelfParam) {
    // Synthesise a `Self` type node.
    const Span SelfSpan = Stream.previous().TokenSpan;
    const NodeIndex Type =
        Pool.allocLeaf(NodeKind::NamedType, SelfSpan, Interner.intern("Self"));
    const NodeIndex Children[] = {Type};
    return Pool.alloc(NodeKind::Param,
                      Span{StartPos, Stream.previous().TokenSpan.End}, Children,
                      NodePool::makePayload(Name, R));
  }

  expect(Kind::colon, DiagID::ExpectedColon, "after parameter name");

  const NodeIndex Type = parseType();

  const NodeIndex Children[] = {Type};
  return Pool.alloc(NodeKind::Param,
                    Span{StartPos, Stream.previous().TokenSpan.End}, Children,
                    NodePool::makePayload(Name, R));
}

NodeIndex Parser::parseStructField() {
  ETER_DEBUG(llvm::dbgs() << "[" DEBUG_TYPE "] parseStructField\n");
  using Kind = lexer::Token::Kind;

  const uint32_t StartPos = peekToken().TokenSpan.Start;

  const Regime R = parseRegime();

  // Bail out at the first error: chaining the follow-up ':' and type
  // diagnostics for a field that never started would only repeat errors on
  // the same token. parseCommaSeparated resynchronises to the next field.
  if (!check(Kind::identifier)) {
    const Span S = peekToken().TokenSpan;
    const llvm::StringRef Found = lexer::Token::getTokenName(peek());
    diag(S, DiagID::ExpectedName)
        .arg("construct", "field")
        .arg("found", Found.str());
    return makeErrorNode(Span{StartPos, S.End});
  }
  const InternedStr Name = Interner.intern(textOf(advance().TokenSpan));

  if (!consume(Kind::colon)) {
    const lexer::Token FoundTok = peekToken();
    const llvm::StringRef Found =
        lexer::Token::getTokenName(FoundTok.TokenKind);
    diag(FoundTok.TokenSpan, DiagID::ExpectedColon)
        .arg("context", "after field name")
        .arg("found", Found.str());
    // Only attempt the type when the next token could actually start one;
    // otherwise the missing ':' diagnostic already covers this field.
    if (!check(Kind::identifier))
      return makeErrorNode(Span{StartPos, Stream.previous().TokenSpan.End});
  }

  const NodeIndex Type = parseType();

  const NodeIndex Children[] = {Type};
  return Pool.alloc(NodeKind::StructField,
                    Span{StartPos, Stream.previous().TokenSpan.End}, Children,
                    NodePool::makePayload(Name, R));
}

NodeIndex Parser::parseEnumVariant() {
  ETER_DEBUG(llvm::dbgs() << "[" DEBUG_TYPE "] parseEnumVariant\n");
  using Kind = lexer::Token::Kind;

  const uint32_t StartPos = peekToken().TokenSpan.Start;

  // Bail out at the first error; parseCommaSeparated resynchronises to the
  // next variant, so a single bad variant yields a single diagnostic.
  if (!check(Kind::identifier)) {
    const Span S = peekToken().TokenSpan;
    const llvm::StringRef Found = lexer::Token::getTokenName(peek());
    diag(S, DiagID::ExpectedName)
        .arg("construct", "enum variant")
        .arg("found", Found.str());
    return makeErrorNode(S);
  }
  const InternedStr Name = Interner.intern(textOf(advance().TokenSpan));

  // Tuple variant: VariantName(Type*)
  if (consume(Kind::l_paren)) {
    llvm::SmallVector<NodeIndex, 4> Types;
    parseCommaSeparated(Types, Kind::r_paren, [this] { return parseType(); });
    const Span End =
        expect(Kind::r_paren, DiagID::ExpectedClose, "to close tuple variant")
            .TokenSpan;
    return Pool.alloc(NodeKind::EnumVariantTuple, Span{StartPos, End.End},
                      Types, NodePool::makePayload(Name, Regime::None));
  }

  // Struct variant: VariantName { StructField* }
  if (consume(Kind::l_brace)) {
    llvm::SmallVector<NodeIndex, 8> Fields;
    parseCommaSeparated(Fields, Kind::r_brace,
                        [this] { return parseStructField(); });
    const Span End =
        expect(Kind::r_brace, DiagID::ExpectedClose, "to close struct variant")
            .TokenSpan;
    return Pool.alloc(NodeKind::EnumVariantStruct, Span{StartPos, End.End},
                      Fields, NodePool::makePayload(Name, Regime::None));
  }

  // Unit variant with an explicit discriminant: VariantName = ConstExpr
  if (consume(Kind::eq)) {
    const NodeIndex Discriminant = parseConstExpr();
    const NodeIndex Children[] = {Discriminant};
    return Pool.alloc(NodeKind::EnumVariantUnit,
                      Span{StartPos, Stream.previous().TokenSpan.End}, Children,
                      NodePool::makePayload(Name, Regime::None));
  }

  // Unit variant: VariantName
  return Pool.allocLeaf(NodeKind::EnumVariantUnit,
                        Span{StartPos, Stream.previous().TokenSpan.End},
                        NodePool::makePayload(Name, Regime::None));
}

NodeIndex Parser::parseTraitDecl(llvm::ArrayRef<NodeIndex> Docs,
                                 uint32_t Flags) {
  ETER_DEBUG(llvm::dbgs() << "[" DEBUG_TYPE "] parseTraitDecl\n");
  using Kind = lexer::Token::Kind;

  const Span Start = expect(Kind::kw_trait, DiagID::ExpectedKeyword).TokenSpan;

  const InternedStr Name = expectName(Kind::identifier, "trait");

  const NodeIndex GenericParams = parseGenericParams();

  expect(Kind::l_brace, DiagID::ExpectedOpen, "to start trait body");

  llvm::SmallVector<NodeIndex, 8> Children(Docs.begin(), Docs.end());
  if (GenericParams != NullNode)
    Children.push_back(GenericParams);
  while (!check(Kind::r_brace) && !atEof()) {
    const llvm::SmallVector<NodeIndex, 4> MethodDocs = parseDocComments();
    const size_t ErrorsBefore = Errors.size();
    Children.push_back(
        parseFnDecl(MethodDocs, /*Flags=*/0, /*ExpectBody=*/false));
    if (Errors.size() != ErrorsBefore)
      while (!check(Kind::r_brace) && !check(Kind::kw_fn) &&
             !check(Kind::kw_unsafe) && !atEof())
        advance();
  }

  const Span End =
      expect(Kind::r_brace, DiagID::ExpectedClose, "to close trait body")
          .TokenSpan;
  return Pool.alloc(NodeKind::TraitDecl, Span{Start.Start, End.End}, Children,
                    Name, Flags);
}

NodeIndex Parser::parseImplDecl(llvm::ArrayRef<NodeIndex> Docs,
                                uint32_t Flags) {
  ETER_DEBUG(llvm::dbgs() << "[" DEBUG_TYPE "] parseImplDecl\n");
  using Kind = lexer::Token::Kind;

  const Span Start = expect(Kind::kw_impl, DiagID::ExpectedKeyword).TokenSpan;

  const NodeIndex GenericParams = parseGenericParams();

  // First type: either the trait (if `for` follows) or the implementing type.
  const NodeIndex FirstType = parseType();

  llvm::SmallVector<NodeIndex, 8> Children(Docs.begin(), Docs.end());
  if (GenericParams != NullNode)
    Children.push_back(GenericParams);
  if (consume(Kind::kw_for)) {
    // Trait impl: impl Trait for Type { ... }
    Children.push_back(FirstType);
    Children.push_back(parseType());
  } else {
    // Inherent impl: impl Type { ... }
    Children.push_back(NullNode);
    Children.push_back(FirstType);
  }

  expect(Kind::l_brace, DiagID::ExpectedOpen, "to start impl body");

  while (!check(Kind::r_brace) && !atEof()) {
    const llvm::SmallVector<NodeIndex, 4> MethodDocs = parseDocComments();
    const size_t ErrorsBefore = Errors.size();
    Children.push_back(
        parseFnDecl(MethodDocs, /*Flags=*/0, /*ExpectBody=*/true));
    if (Errors.size() != ErrorsBefore)
      while (!check(Kind::r_brace) && !check(Kind::kw_fn) &&
             !check(Kind::kw_unsafe) && !atEof())
        advance();
  }

  const Span End =
      expect(Kind::r_brace, DiagID::ExpectedClose, "to close impl body")
          .TokenSpan;
  return Pool.alloc(NodeKind::ImplDecl, Span{Start.Start, End.End}, Children,
                    /*Payload=*/0u, Flags);
}

} // namespace eter::parser
