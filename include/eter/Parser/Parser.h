//===----------------------------------------------------------------------===//
//
// Part of the Eter Project, under the Apache License v2.0 with LLVM Exceptions.
// See /LICENSE for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef ETER_PARSER_PARSER_H
#define ETER_PARSER_PARSER_H

#include "eter/Base/PhaseDiagnostic.h"
#include "eter/Base/Span.h"
#include "eter/Base/StringInterner.h"
#include "eter/Lexer/Token.h"
#include "eter/Parser/NodePool.h"
#include "eter/Parser/ParserDiagnostics.h"
#include "eter/Parser/Regime.h"
#include "eter/Parser/TokenStream.h"

#include <llvm/ADT/ArrayRef.h>
#include <llvm/ADT/STLFunctionalExtras.h>
#include <llvm/ADT/SmallVector.h>
#include <llvm/ADT/StringRef.h>

#include <utility>
#include <vector>

namespace eter::parser {

/// The result of a complete parse run. Owns the entire AST.
///
/// ## Lifetime
/// `ParseResult` is a value type that owns its `NodePool` directly (no
/// heap indirection). When a `ParseResult` goes out of scope, typically
/// after the Driver has lowered the AST to IR, the `NodePool` destructor
/// frees all nodes in a single bulk deallocation (two `std::vector` frees).
/// There are no individual per-node `delete` calls.
///
/// `StringInterner` is NOT owned here. It lives in the Driver for the
/// duration of the whole compilation, because `InternedStr` IDs must remain
/// valid and comparable across all source files parsed in the same session.
/// A `ParseResult` may therefore be destroyed before the `StringInterner`
/// without any issue; the reverse (destroying the interner while a
/// `ParseResult` still holds IDs) is safe as long as the raw `StringRef`
/// text is not accessed via `StringInterner::get` after destruction.
struct ParseResult {
  NodePool Pool;  ///< Owns all AST nodes.
  NodeIndex Root; ///< Index of the SourceFile node.
  std::vector<diag::PhaseDiagnostic>
      Errors; ///< Collected parse errors.
              ///< May be non-empty even when Root is valid
              ///< — the parser is error-resilient.

  /// Return true if no parse errors were recorded.
  [[nodiscard]] bool ok() const { return Errors.empty(); }
};

/// A pure, recursive-descent parser for the Eter language.
///
/// ## Purity contract
/// `Parser::parse` is a pure function: given the same `TokenStream` it always
/// produces the same `ParseResult`. It has no global state and emits no
/// diagnostics during parsing; all errors are accumulated in
/// `ParseResult::Errors`. Callers feed those errors to `DiagnosticEngine`
/// after the parse completes.
///
/// ## Error recovery
/// The parser is error-resilient: it always produces a structurally complete
/// AST, even on malformed input. Malformed subtrees are represented by
/// `NodeKind::Error` nodes. On cascading errors the parser synchronises to the
/// nearest statement boundary (e.g., `;`, `}`) or top-level declaration keyword
/// (e.g., `fn`, `struct`, `enum`, `union`, `mod`, `use`).
///
/// ## Expression parsing
/// Expressions are parsed with a Pratt (top-down operator precedence) parser
/// for O(n) time and minimal stack depth regardless of expression complexity.
///
/// `Parser` is non-copyable and non-movable and may only be instantiated via
/// `Parser::parse`. All sub-parse methods are `private`. Every unimplemented
/// stub
// must call `llvm::report_fatal_error("TODO: implement <method>")` so that
// missing
/// implementations are caught immediately at runtime.
class Parser {
public:
  /// Parse the entire token stream and return the result.
  ///
  /// \param Tokens   A flattened token stream produced from a `SourceBuffer`.
  ///                 Must carry the source text in `TokenStream::textOf`.
  /// \param Interner String interner; identifiers are interned during parsing.
  ///                 The caller must ensure `Interner` outlives the returned
  ///                 `ParseResult` if `StringRef`s from the interner are used
  ///                 after this call.
  [[nodiscard]] static ParseResult parse(TokenStream Tokens,
                                         StringInterner &Interner);

  Parser(const Parser &) = delete;
  Parser &operator=(const Parser &) = delete;
  Parser(Parser &&) = delete;
  Parser &operator=(Parser &&) = delete;

private:
  /// The only way to construct a `Parser` is via `Parser::parse()`.
  explicit Parser(TokenStream Tokens, NodePool &Pool, StringInterner &Interner,
                  std::vector<diag::PhaseDiagnostic> &Errors);

  //===----------------------------------------------------------------------===//
  // Top-level dispatch (cf. Parser.cpp)
  //===----------------------------------------------------------------------===//
  NodeIndex parseSourceFile();

  // Doc comments: `///` outer (attached to following decl) and `//!` inner
  // (attached to enclosing scope). Both reach the parser as plain tokens —
  // regular `//` comments are stripped earlier by `TokenStream`.
  llvm::SmallVector<NodeIndex, 4> parseDocComments();
  llvm::SmallVector<NodeIndex, 4> parseFileDocComments();

  //===----------------------------------------------------------------------===//
  // Declarations (cf. ParseDecl.cpp)
  //===----------------------------------------------------------------------===//

  // Each function is responsible for a single grammar production. Functions
  // that accept `Docs` receive already-parsed DocComment nodes. `Flags`
  // carries per-node boolean properties (pub, unsafe, etc.) encoded as
  // `NodePool::PubFlag`, `NodePool::UnsafeFlag`.
  NodeIndex parseTopLevelDecl();
  NodeIndex parseFnDecl(llvm::ArrayRef<NodeIndex> Docs, uint32_t Flags = 0,
                        bool ExpectBody = true);
  NodeIndex parseStructDecl(llvm::ArrayRef<NodeIndex> Docs, uint32_t Flags = 0);
  NodeIndex parseEnumDecl(llvm::ArrayRef<NodeIndex> Docs, uint32_t Flags = 0);
  NodeIndex parseUnionDecl(llvm::ArrayRef<NodeIndex> Docs, uint32_t Flags = 0);
  NodeIndex parseModDecl(llvm::ArrayRef<NodeIndex> Docs, uint32_t Flags = 0);
  NodeIndex parseUseDecl(llvm::ArrayRef<NodeIndex> Docs, uint32_t Flags = 0);
  NodeIndex parseTraitDecl(llvm::ArrayRef<NodeIndex> Docs, uint32_t Flags = 0);
  NodeIndex parseImplDecl(llvm::ArrayRef<NodeIndex> Docs, uint32_t Flags = 0);
  NodeIndex parseParamList();
  NodeIndex parseParam();
  NodeIndex parseStructField();
  NodeIndex parseEnumVariant();

  /// Parse a comma-separated list of elements produced by `ParseElement` into
  /// `Children`, allowing a trailing comma. Stops before the `Close` delimiter
  /// (or at EOF) without consuming it; the caller `expect`s the delimiter with
  /// its own diagnostic.
  ///
  /// If an element records a diagnostic, skips ahead to the next `,` or
  /// `Close` so that one malformed element produces a single diagnostic
  /// instead of a cascade. Used for struct/union bodies, enum variant lists,
  /// and tuple variant type lists.
  void parseCommaSeparated(llvm::SmallVectorImpl<NodeIndex> &Children,
                           lexer::Token::Kind Close,
                           llvm::function_ref<NodeIndex()> ParseElement);

  /// Consume `:: identifier` segments following an already-consumed leading
  /// identifier, widening `PathSpan` to cover them. Returns true if at least
  /// one segment was consumed (i.e. the name is a path, not a plain
  /// identifier). Shared by path expressions, qualified type names, and
  /// variant patterns.
  bool parsePathSegments(Span &PathSpan);
  // Const Declarations (cf. ParseConst.cpp)
  NodeIndex parseConstDecl(llvm::ArrayRef<NodeIndex> Docs, uint32_t Flags = 0);

  // Statements (cf. ParseStmt.cpp)
  NodeIndex parseStmt();
  NodeIndex parseLetStmt();
  NodeIndex parseRetStmt();
  /// Parses an `if` expression (used in both expression and statement
  /// position).
  NodeIndex parseIfExpr();
  NodeIndex parseForStmt();
  NodeIndex parseWhileStmt();
  NodeIndex parseBreakStmt();
  NodeIndex parseContinueStmt();
  NodeIndex parseUnsafeBlock();
  /// Parses a `match` expression (used in both expression and statement
  /// position).
  NodeIndex parseMatchExpr();
  NodeIndex parseBlockExpr();
  NodeIndex parseMatchArm();

  //===----------------------------------------------------------------------===//
  // Const Expressions (cf. ParseConst.cpp)
  //===----------------------------------------------------------------------===//

  /// Pratt const expression entry point.
  /// \param MinBP Minimum binding power; controls the precedence level at
  ///              which the current call will stop consuming operators.
  ///
  /// FIXME(bruzzone): At this state of the implementation, `parseConstExpr`
  /// will accept only literals.
  NodeIndex parseConstExpr(int MinBP = 0);

  //===----------------------------------------------------------------------===//
  // Expressions (cf. ParseExpr.cpp)
  //===----------------------------------------------------------------------===//

  /// Pratt expression entry point.
  /// \param MinBP Minimum binding power; controls the precedence level at
  ///              which the current call will stop consuming operators.
  NodeIndex parseExpr(int MinBP = 0);
  NodeIndex parsePrefixExpr();
  NodeIndex parseLitExpr(const lexer::Token &Tok);
  NodeIndex parsePostfixOrCallExpr(NodeIndex Lhs);
  NodeIndex parseArgList();

  /// Struct literal: Name { FieldInit* }. `Name` and `Start` come from the
  /// already-consumed identifier; the current token must be `{`.
  NodeIndex parseStructLitExpr(InternedStr Name, Span Start);
  /// A field initialiser: name : Expr, or the shorthand `name` (equivalent
  /// to `name: name`, with a synthesised IdentExpr child).
  NodeIndex parseFieldInit();

  NodeIndex parseArrayLitExpr();
  NodeIndex parseTensorLitExpr();
  NodeIndex parsePostfixIndex(NodeIndex Lhs);

  /// Infix operator precedence table (Pratt parser).
  ///
  /// Returns {left_bp, right_bp}.  left_bp < right_bp → left-associative;
  /// left_bp > right_bp → right-associative; {-1,-1} → not an infix operator.
  /// Postfix operators (`.`  `[`  `(`  `?`) are handled by
  /// parsePostfixOrCallExpr and are NOT in this table.
  ///
  /// | Prec | Operator(s)                                      | Assoc |
  /// |------|--------------------------------------------------|-------|
  /// |  10  | `=` `+=` `-=` `*=` `/=` `%=` `&=` `|=` `^=` `<<=` `>>=` | right |
  /// |  20  | `\|\|`                                           | left  |
  /// |  30  | `&&`                                             | left  |
  /// |  40  | `==` `!=` `<` `>` `<=` `>=`                     | left* |
  /// |  50  | `\|` (bitwise OR)                                | left  |
  /// |  60  | `^` (bitwise XOR)                                | left  |
  /// |  70  | `&` (bitwise AND — prefix `&` handled separately)| left  |
  /// |  80  | `<<` `>>`                                        | left  |
  /// |  90  | `+` `-`                                          | left  |
  /// | 100  | `*` `/` `%`                                      | left  |
  ///
  /// *Comparisons are left-assoc in the parser; the semantic pass enforces
  ///  non-associativity (same strategy as rustc).
  [[nodiscard]] static std::pair<int, int>
  infixBindingPower(lexer::Token::Kind K);

  /// Prefix operator binding powers.
  ///
  /// Returns the right_bp passed to the recursive `parseExpr` call for the
  /// operand; -1 if `K` is not a valid prefix operator.
  /// All prefix operators bind tighter than every infix operator (max = 101).
  ///
  /// | right_bp | Operator                            |
  /// |----------|-------------------------------------|
  /// |   110    | `!` (logical NOT)                   |
  /// |   110    | `-` (unary negation)                |
  /// |   110    | `&` (take projection of a mut value)|
  [[nodiscard]] static int prefixBindingPower(lexer::Token::Kind K);

  //===----------------------------------------------------------------------===//
  // Types (cf. ParseType.cpp)
  //===----------------------------------------------------------------------===//

  NodeIndex parseType();
  NodeIndex parseTupleType();
  NodeIndex parseNamedType();
  NodeIndex parseArrayType();
  NodeIndex parseTensorType();

  //===----------------------------------------------------------------------===//
  // Generics (cf. ParseGenerics.cpp)
  //===----------------------------------------------------------------------===//

  /// Parse generic parameters: < T, U: Bound, V > (or empty for non-generic)
  /// Returns NullNode if no `<` found; returns GenericParamList if present.
  NodeIndex parseGenericParams();

  /// Parse a single generic parameter: T or T: Bound
  NodeIndex parseGenericParam();

  /// Parse generic arguments in type position: < Type, Type, ... >
  /// Assumes current token is `<`; consumes up to and including `>`.
  NodeIndex parseGenericArgs();

  //===----------------------------------------------------------------------===//
  // Patterns (cf. ParsePat.cpp)
  //===----------------------------------------------------------------------===//

  NodeIndex parsePat();
  /// Struct pattern: Name { FieldPat* }. `Name` and `Start` come from the
  /// already-consumed (possibly `::`-qualified) path; the current token must
  /// be `{`.
  NodeIndex parseStructPat(InternedStr Name, Span Start);
  /// Tuple pattern: Name ( Pat* ). `Name` is `NullStr` for anonymous tuple
  /// patterns; the current token must be `(`.
  NodeIndex parseTuplePat(InternedStr Name, Span Start);
  /// A field binding inside a struct pattern: name [: Pat].
  NodeIndex parseFieldPat();

  //===----------------------------------------------------------------------===//
  // Regime
  //===----------------------------------------------------------------------===//

  /// Consume `imm`, `mut`, or `proj` if the current token matches, and return
  /// the corresponding `Regime`. Returns `Regime::None` without consuming any
  /// token if none of the regime keywords is present.
  Regime parseRegime();

  //===----------------------------------------------------------------------===//
  // Token stream helpers
  //===----------------------------------------------------------------------===//

  [[nodiscard]] lexer::Token::Kind peek(uint32_t Offset = 0) const;
  [[nodiscard]] lexer::Token peekToken(uint32_t Offset = 0) const;
  [[nodiscard]] bool check(lexer::Token::Kind K) const;
  [[nodiscard]] bool atEof() const;
  lexer::Token advance();
  bool consume(lexer::Token::Kind K);

  /// Consume a token of kind `K`. If the current token does not match, record
  /// a parse error with diagnostic `D` and return a synthetic token with
  /// `Kind::unknown`.
  ///
  /// The `{tok}` and `{keyword}` placeholders are automatically set to the
  /// human-readable name of `K` (e.g. "fn", ";", "{"). If `Context` is
  /// non-empty, the `{context}` placeholder is also set.
  lexer::Token expect(lexer::Token::Kind K, DiagID D,
                      llvm::StringRef Context = "");

  /// Consume a token of kind `K`, intern its source text, and return the
  /// resulting `InternedStr`.
  ///
  /// Intended for tokens whose textual representation *is* their semantic
  /// value (e.g., `identifier`, path/import segments). String and character
  /// literals carry quoting and escapes that must be processed before
  /// interning, and numeric literals are parsed into typed values — neither
  /// should be routed through this helper.
  ///
  /// On a kind mismatch, `expect` records a parse error and returns a
  /// synthetic `unknown` token; this helper still returns a well-formed
  /// `InternedStr` (over the empty text of that synthetic token), so the
  /// caller can keep building the AST without branching on the error.
  ///
  /// The consumed token remains accessible via `Stream.previous()` if the
  /// caller needs its `Span` (e.g., to widen a node's span).
  InternedStr expectAndIntern(lexer::Token::Kind K, DiagID D);

  /// Consume an identifier, intern it, and return the `InternedStr`.
  /// On failure, emit `ExpectedName` with `{construct}` set to `Construct`.
  /// Convenience wrapper around `expectAndIntern` for the common
  /// "expected {construct} name" pattern.
  InternedStr expectName(lexer::Token::Kind K, llvm::StringRef Construct);

  //===----------------------------------------------------------------------===//
  // Diagnostics
  //===----------------------------------------------------------------------===//

  /// Fluent builder for parser diagnostics. Supports adding labelled spans,
  /// notes, and `{name}` replacement arguments before the diagnostic is
  /// consumed by the caller (typically through `expect`, `addError`, or via
  /// the `Errors` vector stored in `ParseResult`).
  class DiagBuilder {
  public:
    /// Append a `{name}` → `Value` mapping for the message template.
    DiagBuilder &arg(llvm::StringRef Name, std::string Value) & {
      PD.Args.emplace_back(Name, std::move(Value));
      return *this;
    }
    DiagBuilder &&arg(llvm::StringRef Name, std::string Value) && {
      return std::move(this->arg(Name, std::move(Value)));
    }

    /// Add a labelled sub-span to the diagnostic.
    DiagBuilder &label(Span S, llvm::StringRef Msg) & {
      PD.Labels.push_back({S, Msg.str()});
      return *this;
    }
    DiagBuilder &&label(Span S, llvm::StringRef Msg) && {
      return std::move(this->label(S, Msg));
    }

    /// Add a note to the diagnostic.
    DiagBuilder &note(llvm::StringRef Msg) & {
      PD.Notes.push_back(Msg.str());
      return *this;
    }
    DiagBuilder &&note(llvm::StringRef Msg) && {
      return std::move(this->note(Msg));
    }

    /// Add a help suggestion to the diagnostic.
    DiagBuilder &help(llvm::StringRef Msg) & {
      PD.Helps.push_back(Msg.str());
      return *this;
    }
    DiagBuilder &&help(llvm::StringRef Msg) && {
      return std::move(this->help(Msg));
    }

  private:
    friend class Parser;
    explicit DiagBuilder(diag::PhaseDiagnostic &PD) : PD(PD) {}
    diag::PhaseDiagnostic &PD;
  };

  /// Create a new parse diagnostic at span `S` with id `D`.
  /// Returns a `DiagBuilder` for adding arguments, labels, or notes.
  ///
  /// Usage:
  ///   diag(SomeSpan, DiagID::ExpectedToken)
  ///     .arg("tok", "fn")
  ///     .label(SomeSpan, "while parsing this")
  ///     .note("try adding ';' here");
  DiagBuilder diag(Span S, DiagID D);

  /// Record a parse error at span `S` with diagnostic `D` (no extra args).
  void addError(Span S, DiagID D);

  /// Allocate a `NodeKind::Error` node covering span `S`.
  NodeIndex makeErrorNode(Span S);

  /// Advance past tokens until a synchronisation point:
  ///   - A `;` (end of the current statement), or
  ///   - A `}` (end of the current block), or
  ///   - A top-level declaration keyword: fn, struct, enum, union, mod, use,
  ///     const, trait, impl, unsafe, or
  ///   - A statement-introducing keyword: let, if, while, for, match, ret,
  ///     break, continue.
  ///
  /// The broader keyword set is a strict superset of the top-level set, so the
  /// same routine is correct for both top-level and block-internal recovery.
  void synchronize();

  //===----------------------------------------------------------------------===//
  // Source text
  //===----------------------------------------------------------------------===//

  /// Extract the source text covered by `S`. Delegates to `TokenStream::textOf`
  /// which holds the source buffer passed at construction time.
  [[nodiscard]] llvm::StringRef textOf(Span S) const {
    return Stream.textOf(S);
  }

  /// Whether `Name { … }` in expression position parses as a struct literal.
  /// Cleared while parsing if/while conditions and match scrutinees, where a
  /// following `{` opens the construct's block instead (same restriction as
  /// Rust); restored inside any bracketed sub-expression: `(…)`, `[…]`,
  /// argument lists, and struct-literal field initialisers.
  bool StructLitAllowed = true;

  /// The token stream.
  TokenStream Stream;
  /// The node pool owns all AST nodes.
  /// It is owned by the Parser and lives for the duration of the parse.
  NodePool &Pool;
  /// The string interner owns all identifiers.
  StringInterner &Interner;
  /// The parse errors are collected inside `ParseResult`.
  std::vector<diag::PhaseDiagnostic> &Errors;
};

} // namespace eter::parser

#endif // ETER_PARSER_PARSER_H
