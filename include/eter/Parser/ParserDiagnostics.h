//===----------------------------------------------------------------------===//
//
// Part of the Eter Project, under the Apache License v2.0 with LLVM Exceptions.
// See /LICENSE for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef ETER_PARSER_PARSERDIAGNOSTICS_H
#define ETER_PARSER_PARSERDIAGNOSTICS_H

#include <llvm/ADT/StringRef.h>

#include <cstdint>

namespace eter::parser {

/// Stable parser diagnostic identifiers. Populated from
/// `eter/Parser/ParserDiagnostics.def` via the X-Macro pattern; the
/// underlying type is `uint16_t` so values are storable in the
/// `PhaseDiagnostic::LocalID` slot.
enum class DiagID : uint16_t {
#define ETER_PARSE_DIAG(X, Y) X,
#include "eter/Parser/ParserDiagnostics.def"
};

/// Return the static message template for `D`. May contain `{name}`
/// placeholders to be resolved against `PhaseDiagnostic::Args` via
/// `diag::renderMessage`.
inline llvm::StringRef messageFor(DiagID D) {
  switch (D) {
#define ETER_PARSE_DIAG(X, Y)                                                  \
  case DiagID::X:                                                              \
    return Y;
#include "eter/Parser/ParserDiagnostics.def"
  }
  return "unknown";
}

} // namespace eter::parser

#endif // ETER_PARSER_PARSERDIAGNOSTICS_H
