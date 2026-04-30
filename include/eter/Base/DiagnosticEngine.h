//===----------------------------------------------------------------------===//
//
// Part of the Eter Project, under the Apache License v2.0 with LLVM Exceptions.
// See /LICENSE for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef ETER_BASE_DIAGNOSTICENGINE_H
#define ETER_BASE_DIAGNOSTICENGINE_H

#include "eter/Base/SourceManager.h"

#include <cstddef>

namespace eter {

/// Severity level of a diagnostic message.
enum class DiagnosticLevel { Error, Warning, Note };

/// Category indicating the origin of a diagnostic.
enum class DiagnosticKind { UserError, InternalCompilerError };

/// A labeled span attached to a diagnostic, providing additional context.
struct DiagnosticLabel {
  Span Span;
  std::string Message;

  /// Return the label message as a string reference.
  llvm::StringRef getMessage() const;
};

/// Represents the location associated with a diagnostic.
class DiagnosticLocation {
public:
  /// Kind of location represented.
  enum class Kind { None, File, Span };

  /// Create a location representing no specific source location.
  static DiagnosticLocation none() { return DiagnosticLocation(); }

  /// Create a location representing a file.
  static DiagnosticLocation file(llvm::StringRef Filename) {
    DiagnosticLocation Loc;
    Loc.Kind = Kind::File;
    Loc.Filename = Filename.str();
    return Loc;
  }

  /// Create a location representing a source span.
  static DiagnosticLocation span(Span Span) {
    DiagnosticLocation Loc;
    Loc.Kind = Kind::Span;
    Loc.Span = Span;
    return Loc;
  }

  /// Return the kind of this location.
  Kind kind() const { return Kind; }

  /// Return the filename associated with this location.
  const std::string &filename() const { return Filename; }

  /// Return the span associated with this location.
  const Span &span() const { return Span; }

  /// Return true if this location has a span.
  bool hasSpan() const { return Kind == Kind::Span; }
  /// Return true if this location has a file.
  bool hasFile() const { return Kind == Kind::File; }

private:
  Kind Kind = Kind::None;

  std::string Filename;
  Span Span;
};

/// A single diagnostic message with metadata, labels, and notes.
struct Diagnostic {
  DiagnosticLevel Level;
  DiagnosticKind Kind;

  std::string Message;
  DiagnosticLocation Location;

  llvm::SmallVector<DiagnosticLabel, 2> Labels;
  llvm::SmallVector<std::string, 2> Notes;
};

template <typename DiagnosticEngineType> class DiagnosticBuilder;

class DiagnosticEngine;
/// A simple diagnostic engine that buffers diagnostics without source manager
/// support.
class SimpleDiagnosticEngine {
public:
  SimpleDiagnosticEngine() = default;

  /// Emit a diagnostic.
  void emit(Diagnostic Diag);

  /// Create an error diagnostic builder.
  DiagnosticBuilder<SimpleDiagnosticEngine> error();
  /// Create a warning diagnostic builder.
  DiagnosticBuilder<SimpleDiagnosticEngine> warning();
  /// Create a note diagnostic builder.
  DiagnosticBuilder<SimpleDiagnosticEngine> note();

  /// Convert this simple engine to a full DiagnosticEngine with the given
  /// source manager. Calling this method when SimpleDiagnosticEngine is an
  /// rvalue causes the SimpleDiagnosticEngine instance to be invalidated.
  DiagnosticEngine withSourceManager(const SourceManager &SM) &&;

  SimpleDiagnosticEngine(SimpleDiagnosticEngine &&) = delete;
  SimpleDiagnosticEngine &operator=(SimpleDiagnosticEngine &&) = delete;

private:
  llvm::SmallVector<Diagnostic, 8> Diagnostics;
};

/// Main diagnostic engine with source manager support for emitting and printing
/// diagnostics.
class DiagnosticEngine {
public:
  /// Construct a diagnostic engine with a source manager.
  explicit DiagnosticEngine(const SourceManager &SM);

  /// Emit a diagnostic.
  void emit(Diagnostic Diag);

  /// Create an error diagnostic builder.
  DiagnosticBuilder<DiagnosticEngine> error();
  /// Create an error diagnostic builder at the given span.
  DiagnosticBuilder<DiagnosticEngine> error(Span Span);
  /// Create a warning diagnostic builder.
  DiagnosticBuilder<DiagnosticEngine> warning();
  /// Create a warning diagnostic builder at the given span.
  DiagnosticBuilder<DiagnosticEngine> warning(Span Span);
  /// Create a note diagnostic builder.
  DiagnosticBuilder<DiagnosticEngine> note();
  /// Create a note diagnostic builder at the given span.
  DiagnosticBuilder<DiagnosticEngine> note(Span Span);

  /// Create an internal compiler error diagnostic builder.
  DiagnosticBuilder<DiagnosticEngine> ice();
  /// Create an internal compiler error diagnostic builder at the given span.
  DiagnosticBuilder<DiagnosticEngine> ice(Span Span);

  /// Print all buffered diagnostics.
  void printAll() const;

private:
  /// Print a single diagnostic.
  void print(const Diagnostic &Diag) const;

  const SourceManager &SM;
  llvm::SmallVector<Diagnostic, 8> Diagnostics;
};

/// Fluent builder for constructing diagnostics before emitting them.
template <typename DiagnosticEngineType> class DiagnosticBuilder {
public:
  /// Construct a diagnostic builder for the given engine, level, and kind.
  DiagnosticBuilder(DiagnosticEngineType &DE, DiagnosticLevel Level,
                    DiagnosticKind Kind)
      : DE(DE) {
    Diag.Level = Level;
    Diag.Kind = Kind;
    Diag.Location = DiagnosticLocation::none();
  }

  /// Set the diagnostic location to a source span.
  DiagnosticBuilder<DiagnosticEngineType> &at(Span Span) {
    Diag.Location = DiagnosticLocation::span(Span);
    return *this;
  }

  /// Set the diagnostic location to a file.
  DiagnosticBuilder<DiagnosticEngineType> &atFile(llvm::StringRef Filename) {
    Diag.Location = DiagnosticLocation::file(Filename);
    return *this;
  }

  /// Set the diagnostic message.
  DiagnosticBuilder<DiagnosticEngineType> &message(const llvm::Twine &Msg) {
    Diag.Message = Msg.str();
    return *this;
  }

  /// Add a labeled span to the diagnostic.
  DiagnosticBuilder<DiagnosticEngineType> &label(Span Span,
                                                 const llvm::Twine &Msg) {
    Diag.Labels.push_back({Span, Msg.str()});
    return *this;
  }

  /// Add a note to the diagnostic.
  DiagnosticBuilder<DiagnosticEngineType> &note(const llvm::Twine &Msg) {
    Diag.Notes.push_back(Msg.str());
    return *this;
  }

  /// Emit the constructed diagnostic.
  void emit() { DE.emit(std::move(Diag)); }

private:
  DiagnosticEngineType &DE;
  Diagnostic Diag;
};

} // namespace eter

#endif // ETER_BASE_DIAGNOSTICENGINE_H
