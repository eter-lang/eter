//===----------------------------------------------------------------------===//
//
// Part of the Eter Project, under the Apache License v2.0 with LLVM Exceptions.
// See /LICENSE for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "eter/Base/DiagnosticEngine.h"

#include <llvm/ADT/StringRef.h>
#include <llvm/Support/Compiler.h>
#include <llvm/Support/raw_ostream.h>

#include <algorithm>
#include <cstdint>

namespace eter {

static unsigned digitWidth(uint32_t N) {
  unsigned W = 1;
  while (N >= 10) {
    N /= 10;
    ++W;
  }
  return W;
}

static llvm::raw_ostream::Colors levelColor(DiagnosticLevel L,
                                            DiagnosticKind K) {
  if (K == DiagnosticKind::InternalCompilerError)
    return llvm::raw_ostream::MAGENTA;
  switch (L) {
  case DiagnosticLevel::Error:
    return llvm::raw_ostream::RED;
  case DiagnosticLevel::Warning:
    return llvm::raw_ostream::YELLOW;
  case DiagnosticLevel::Note:
    return llvm::raw_ostream::CYAN;
  }
  return llvm::raw_ostream::SAVEDCOLOR;
}

static const char *levelTitle(DiagnosticLevel L, DiagnosticKind K) {
  if (K == DiagnosticKind::InternalCompilerError)
    return "internal compiler error";
  switch (L) {
  case DiagnosticLevel::Error:
    return "error";
  case DiagnosticLevel::Warning:
    return "warning";
  case DiagnosticLevel::Note:
    return "note";
  }
  return "diagnostic";
}

static size_t findLineStart(llvm::StringRef B, size_t Off) {
  if (Off == 0)
    return 0;
  if (Off > B.size())
    Off = B.size();
  // StringRef::rfind treats `From` as exclusive (it searches positions
  // [0, From)), so passing `Off` here looks at positions [0, Off-1].
  const size_t Nl = B.rfind('\n', Off);
  return Nl == llvm::StringRef::npos ? 0 : Nl + 1;
}

static size_t findLineEnd(llvm::StringRef B, size_t Off) {
  const size_t Nl = B.find('\n', Off);
  return Nl == llvm::StringRef::npos ? B.size() : Nl;
}

static uint32_t countLines(llvm::StringRef B) {
  if (B.empty())
    return 0;
  uint32_t N = 1;
  for (const char C : B)
    if (C == '\n')
      ++N;
  return N;
}

static void writeRepeated(llvm::raw_ostream &OS, unsigned Count, char C) {
  for (unsigned I = 0; I < Count; ++I)
    OS << C;
}

// Render a snippet block: 1 line of context before, the line(s) covered by
// `S`, then 1 line of context after; with a gutter `  N | `, carets under
// the spanned columns, and an optional caption next to the carets.
static void printSnippet(llvm::raw_ostream &OS, const SourceManager &SM, Span S,
                         llvm::StringRef Caption,
                         llvm::raw_ostream::Colors Color) {
  const llvm::StringRef Buf = SM.getBuffer();
  if (Buf.empty())
    return;

  const size_t StartOff = std::min<size_t>(S.Start, Buf.size());
  size_t EndOff = std::min<size_t>(S.End, Buf.size());
  if (EndOff < StartOff)
    EndOff = StartOff;

  const SourceLocation StartLoc = SM.getLocation(StartOff);
  const SourceLocation EndLoc = SM.getLocation(EndOff);
  const uint32_t SpanStartLine = StartLoc.Line;
  const uint32_t SpanEndLine = EndLoc.Line;
  const uint32_t StartCol = StartLoc.Column;
  const uint32_t EndCol = EndLoc.Column;

  const uint32_t TotalLines = countLines(Buf);
  const uint32_t CtxStartLine = SpanStartLine > 1 ? SpanStartLine - 1 : 1;
  const uint32_t CtxEndLine = std::min(TotalLines, SpanEndLine + 1);
  const unsigned GutterW = digitWidth(CtxEndLine);

  size_t LineOff = findLineStart(Buf, StartOff);
  for (uint32_t Step = 0; Step < SpanStartLine - CtxStartLine; ++Step) {
    if (LineOff == 0)
      break;
    LineOff = findLineStart(Buf, LineOff - 1);
  }

  // Top border: "    |"
  OS << " ";
  OS.changeColor(llvm::raw_ostream::CYAN, /*Bold=*/true);
  writeRepeated(OS, GutterW, ' ');
  OS << " |\n";
  OS.resetColor();

  uint32_t CurLine = CtxStartLine;
  while (CurLine <= CtxEndLine) {
    const size_t LineEndOff = findLineEnd(Buf, LineOff);
    const llvm::StringRef LineText = Buf.slice(LineOff, LineEndOff);
    const bool InSpan = (CurLine >= SpanStartLine && CurLine <= SpanEndLine);

    OS << " ";
    OS.changeColor(llvm::raw_ostream::CYAN, /*Bold=*/true);
    writeRepeated(OS, GutterW - digitWidth(CurLine), ' ');
    OS << CurLine << " | ";
    OS.resetColor();
    OS << LineText << "\n";

    if (InSpan) {
      const uint32_t LineLen = static_cast<uint32_t>(LineEndOff - LineOff);
      const uint32_t FromCol = (CurLine == SpanStartLine) ? StartCol : 1;
      uint32_t ToCol = (CurLine == SpanEndLine) ? EndCol : LineLen + 1;
      if (ToCol <= FromCol)
        ToCol = FromCol + 1;
      const uint32_t Width = ToCol - FromCol;

      OS << " ";
      OS.changeColor(llvm::raw_ostream::CYAN, /*Bold=*/true);
      writeRepeated(OS, GutterW, ' ');
      OS << " | ";
      OS.resetColor();
      writeRepeated(OS, FromCol - 1, ' ');
      OS.changeColor(Color, /*Bold=*/true);
      writeRepeated(OS, Width, '^');
      if (!Caption.empty() && CurLine == SpanEndLine)
        OS << ' ' << Caption;
      OS.resetColor();
      OS << "\n";
    }

    if (LineEndOff >= Buf.size())
      break;
    LineOff = LineEndOff + 1;
    ++CurLine;
  }
}

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
SimpleDiagnosticEngine::withSourceManager(SourceManager SM) && {
  DiagnosticEngine DE(std::move(SM));

  for (auto &D : Diagnostics)
    DE.emit(std::move(D));

  return DE;
}

DiagnosticEngine::DiagnosticEngine(SourceManager SM) : SM(std::move(SM)) {}

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

DiagnosticBuilder<DiagnosticEngine> DiagnosticEngine::ice() {
  return DiagnosticBuilder(*this, DiagnosticLevel::Error,
                           DiagnosticKind::InternalCompilerError);
}

DiagnosticBuilder<DiagnosticEngine> DiagnosticEngine::ice(Span Span) {
  return DiagnosticBuilder(*this, DiagnosticLevel::Error,
                           DiagnosticKind::InternalCompilerError)
      .at(Span);
}

void DiagnosticEngine::print(const Diagnostic &Diag) const {
  llvm::raw_ostream &OS = llvm::outs();
  const auto Color = levelColor(Diag.Level, Diag.Kind);
  const char *Title = levelTitle(Diag.Level, Diag.Kind);

  OS.changeColor(Color, /*Bold=*/true);
  OS << Title;
  OS.resetColor();
  OS.changeColor(llvm::raw_ostream::SAVEDCOLOR, /*Bold=*/true);
  OS << ": " << Diag.Message << "\n";
  OS.resetColor();

  switch (Diag.Location.kind()) {
  case DiagnosticLocation::Kind::None:
    break;
  case DiagnosticLocation::Kind::File:
    OS.changeColor(llvm::raw_ostream::CYAN, /*Bold=*/true);
    OS << " --> ";
    OS.resetColor();
    OS << Diag.Location.filename() << "\n";
    break;
  case DiagnosticLocation::Kind::Span: {
    const Span Sp = Diag.Location.span();
    const SourceLocation Start = SM.getLocation(Sp.Start);
    OS.changeColor(llvm::raw_ostream::CYAN, /*Bold=*/true);
    OS << " --> ";
    OS.resetColor();
    OS << SM.getFilename() << ":" << Start.Line << ":" << Start.Column << "\n";
    printSnippet(OS, SM, Sp, /*Caption=*/"", Color);
    break;
  }
  }

  for (const auto &Label : Diag.Labels)
    printSnippet(OS, SM, Label.DiagSpan, Label.getMessage(),
                 llvm::raw_ostream::BLUE);

  for (const auto &Note : Diag.Notes) {
    OS.changeColor(llvm::raw_ostream::CYAN, /*Bold=*/true);
    OS << " = note: ";
    OS.resetColor();
    OS << Note << "\n";
  }

  if (Diag.Kind == DiagnosticKind::InternalCompilerError) {
    OS << "\n";
    OS.changeColor(llvm::raw_ostream::MAGENTA, /*Bold=*/true);
    OS << "=== INTERNAL COMPILER ERROR ===\n";
    OS.resetColor();
    OS << "This is a bug in the compiler. Please report it to the "
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
