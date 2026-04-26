//===----------------------------------------------------------------------===//
//
// Part of the Eter Project, under the Apache License v2.0 with LLVM Exceptions.
// See /LICENSE for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "eter/Base/DiagnosticEngine.h"

#include <llvm/Support/Compiler.h>
#include <llvm/Support/raw_ostream.h>

namespace eter {

void SimpleDiagnosticEngine::emit(Diagnostic Diag) {
  Diagnostics.push_back(std::move(Diag));
}

DiagnosticBuilder<SimpleDiagnosticEngine> SimpleDiagnosticEngine::error() {
  return DiagnosticBuilder(*this, DiagnosticLevel::Error,
                           DiagnosticKind::UserError);
}
DiagnosticBuilder<SimpleDiagnosticEngine> SimpleDiagnosticEngine::warning() {
  return DiagnosticBuilder(*this, DiagnosticLevel::Warning,
                           DiagnosticKind::UserError);
}
DiagnosticBuilder<SimpleDiagnosticEngine> SimpleDiagnosticEngine::note() {
  return DiagnosticBuilder(*this, DiagnosticLevel::Note,
                           DiagnosticKind::UserError);
}

DiagnosticEngine
SimpleDiagnosticEngine::withSourceManager(const SourceManager &SM) && {
  DiagnosticEngine DE(SM);

  for (auto &D : Diagnostics)
    DE.emit(std::move(D));

  return DE;
}

DiagnosticEngine::DiagnosticEngine(const SourceManager &SM) : SM(SM) {}

void DiagnosticEngine::emit(Diagnostic Diag) {
  Diagnostics.push_back(std::move(Diag));
}

DiagnosticBuilder<DiagnosticEngine> DiagnosticEngine::error() {
  return DiagnosticBuilder(*this, DiagnosticLevel::Error,
                           DiagnosticKind::UserError);
}

DiagnosticBuilder<DiagnosticEngine> DiagnosticEngine::error(Span Span) {
  return error().at(Span);
}

DiagnosticBuilder<DiagnosticEngine> DiagnosticEngine::warning() {
  return DiagnosticBuilder(*this, DiagnosticLevel::Warning,
                           DiagnosticKind::UserError);
}

DiagnosticBuilder<DiagnosticEngine> DiagnosticEngine::warning(Span Span) {
  return warning().at(Span);
}

DiagnosticBuilder<DiagnosticEngine> DiagnosticEngine::note() {
  return DiagnosticBuilder(*this, DiagnosticLevel::Note,
                           DiagnosticKind::UserError);
}

DiagnosticBuilder<DiagnosticEngine> DiagnosticEngine::note(Span Span) {
  return note().at(Span);
}

DiagnosticBuilder<DiagnosticEngine> DiagnosticEngine::ice(Span Span) {
  return DiagnosticBuilder(*this, DiagnosticLevel::Error,
                           DiagnosticKind::InternalCompilerError)
      .at(Span);
}

void DiagnosticEngine::print(const Diagnostic &Diag) const {
  const char *LevelStr = (Diag.Kind == DiagnosticKind::InternalCompilerError)
                             ? "internal compiler error"
                         : (Diag.Level == DiagnosticLevel::Error)   ? "error"
                         : (Diag.Level == DiagnosticLevel::Warning) ? "warning"
                                                                    : "note";

  llvm::outs() << LevelStr << ": " << Diag.Message << "\n";

  switch (Diag.Location.kind()) {
  case DiagnosticLocation::Kind::None:
    break;
  case DiagnosticLocation::Kind::File:
    llvm::outs() << " --> " << Diag.Location.filename() << "\n";
    break;
  case DiagnosticLocation::Kind::Span: {
    auto Span = Diag.Location.span();

    auto Start = SM.getLocation(Span.Start);
    auto End = SM.getLocation(Span.End);

    llvm::outs() << " --> " << SM.getFilename() << ":" << Start.Line << ":"
                 << Start.Column << "\n";

    // Extract the line of source code corresponding to the span and print it
    const llvm::StringRef Buffer = SM.getBuffer();

    size_t LineStart = Buffer.rfind('\n', Span.Start);
    if (LineStart == llvm::StringRef::npos)
      LineStart = 0;
    else
      LineStart += 1;

    size_t LineEnd = Buffer.find('\n', Span.Start);
    if (LineEnd == llvm::StringRef::npos)
      LineEnd = Buffer.size();
    const llvm::StringRef Line = Buffer.slice(LineStart, LineEnd);
    llvm::outs() << Line << "\n";
    llvm::outs() << "     ";

    // Caret alignment: print spaces until the start column, then print carets
    // until the end column
    for (uint32_t I = 1; I < Start.Column; I++)
      llvm::outs() << " ";
    const uint32_t EndCol =
        (Span.End > LineEnd) ? (LineEnd - LineStart + 1) : End.Column;
    const uint32_t Width =
        (EndCol >= Start.Column) ? (EndCol - Start.Column + 1) : 1;

    for (uint32_t I = 0; I < Width; I++)
      llvm::outs() << "^";

    llvm::outs() << "\n";

    break;
  }
  }

  // Print labels
  for (const auto &Label : Diag.Labels) {
    auto Loc = SM.getLocation(Label.Span.Start);

    llvm::outs() << "  | " << Loc.Line << ":" << Loc.Column << " -> "
                 << Label.getMessage() << "\n";
  }

  // Print notes
  for (const auto &Note : Diag.Notes) {
    llvm::outs() << "  = note: " << Note << "\n";
  }

  // Print internal compiler error message if this is an ICE
  if (Diag.Kind == DiagnosticKind::InternalCompilerError) {
    llvm::outs() << "\n=== INTERNAL COMPILER ERROR ===\n"
                 << "This is a bug in the compiler. Please report it to the "
                    "developers.\n";
  }
}

void DiagnosticEngine::printAll() const {
  for (const auto &D : Diagnostics)
    print(D);
}

llvm::StringRef DiagnosticLabel::getMessage() const {
  return llvm::StringRef(Message);
}

} // namespace eter