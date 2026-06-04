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
/// (e.g., `fn`, `struct`, `enum`, `mod`, `use`).
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

  // Attributes: @name [(ArgList)]
  llvm::SmallVector<NodeIndex, 4> parseAttributes();
  NodeIndex parseAttribute();

  // Doc comments: `///` outer (attached to following decl) and `//!` inner
  // (attached to enclosing scope). Both reach the parser as plain tokens —
  // regular `//` comments are stripped earlier by `TokenStream`.
  llvm::SmallVector<NodeIndex, 4> parseDocComments();
  llvm::SmallVector<NodeIndex, 4> parseFileDocComments();

  //===----------------------------------------------------------------------===//
  // Declarations (cf. ParseDecl.cpp)
  //===----------------------------------------------------------------------===//

  // Each function is responsible for a single grammar production. Functions
  // that accept `Docs` / `Attrs` receive already-parsed DocComment and
  // @-attribute nodes so that they are not re-parsed.
  NodeIndex parseTopLevelDecl(llvm::ArrayRef<NodeIndex> Attrs);
  NodeIndex parseFnDecl(llvm::ArrayRef<NodeIndex> Docs,
                        llvm::ArrayRef<NodeIndex> Attrs);
  NodeIndex parseStructDecl(llvm::ArrayRef<NodeIndex> Docs,
                            llvm::ArrayRef<NodeIndex> Attrs);
  NodeIndex parseEnumDecl(llvm::ArrayRef<NodeIndex> Docs,
                          llvm::ArrayRef<NodeIndex> Attrs);
  NodeIndex parseModDecl(llvm::ArrayRef<NodeIndex> Docs,
                         llvm::ArrayRef<NodeIndex> Attrs);
  NodeIndex parseUseDecl(llvm::ArrayRef<NodeIndex> Docs,
                         llvm::ArrayRef<NodeIndex> Attrs);
  NodeIndex parseParamList();
  NodeIndex parseParam();
  NodeIndex parseStructField();
  NodeIndex parseEnumVariant();
  // Const Declarations (cf. ParseConst.cpp)
  NodeIndex parseConstDecl(llvm::ArrayRef<NodeIndex> Docs,
                           llvm::ArrayRef<NodeIndex> Attrs);

  // Statements (cf. ParseStmt.cpp)
  NodeIndex parseStmt();
  NodeIndex parseLetStmt();
  NodeIndex parseRetStmt();
  /// Parses an `if` expression (used in both expression and statement
  /// position).
  NodeIndex parseIfExpr();
  NodeIndex parseForStmt();
  NodeIndex parseWhileStmt();
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
  /// will accept
  ///                  only literals.
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
  NodeIndex parseNamedType();
  NodeIndex parseArrayType();

  //===----------------------------------------------------------------------===//
  // Patterns (cf. ParsePat.cpp)
  //===----------------------------------------------------------------------===//

  NodeIndex parsePat();
  NodeIndex parseStructPat(InternedStr Name, Span Start);
  NodeIndex parseTuplePat(InternedStr Name, Span Start);

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
  lexer::Token expect(lexer::Token::Kind K, DiagID D);

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

  //===----------------------------------------------------------------------===//
  // Error recovery
  //===----------------------------------------------------------------------===//

  /// Record a parse error at span `S` with diagnostic `D`.
  void addError(Span S, DiagID D);

  /// Allocate a `NodeKind::Error` node covering span `S`.
  NodeIndex makeErrorNode(Span S);

  /// Advance past tokens until a synchronisation point:
  ///   - A `;` (end of the current statement), or
  ///   - A `}` (end of the current block), or
  ///   - A top-level declaration keyword: fn, struct, enum, mod, use, const, or
  ///   - A statement-introducing keyword: let, if, while, for, match, ret.
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
