//===----------------------------------------------------------------------===//
//
// Part of the Eter Project, under the Apache License v2.0 with LLVM Exceptions.
// See /LICENSE for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef ETER_BASE_PHASEDIAGNOSTIC_H
#define ETER_BASE_PHASEDIAGNOSTIC_H

#include "eter/Base/DiagnosticEngine.h"
#include "eter/Base/Span.h"

#include <llvm/ADT/ArrayRef.h>
#include <llvm/ADT/SmallVector.h>
#include <llvm/ADT/StringRef.h>

#include <cstdint>
#include <string>
#include <utility>

namespace eter::diag {

/// Origin phase of a compiler diagnostic. The pair (Ph, LocalID) is the
/// stable identity of a diagnostic; LocalID is the integer value of the
/// phase's local enum (e.g. `parser::DiagID`, `lexer::LexerError::Kind`).
enum class Phase : uint8_t { Lexer, Parser };

/// A discriminated diagnostic produced by a compiler phase. Phases collect
/// these in a vector and the Driver sinks them into `DiagnosticEngine`.
///
/// `Args` carries named replacements for `{name}` placeholders in the message
/// template returned by the phase's `messageFor` function; `Labels` and
/// `Notes` mirror the corresponding fields in `Diagnostic`.
struct PhaseDiagnostic {
  Phase Ph;
  uint16_t LocalID;
  Span Loc;
  llvm::SmallVector<DiagnosticLabel, 2> Labels;
  llvm::SmallVector<std::string, 2> Notes;
  llvm::SmallVector<std::pair<llvm::StringRef, std::string>, 0> Args;
};

/// Render `Template` by substituting every `{name}` placeholder with the
/// matching entry in `Args`. Placeholders without a matching arg are left
/// verbatim, so a missing arg never silently swallows information.
std::string
renderMessage(llvm::StringRef Template,
              llvm::ArrayRef<std::pair<llvm::StringRef, std::string>> Args);

} // namespace eter::diag

#endif // ETER_BASE_PHASEDIAGNOSTIC_H
