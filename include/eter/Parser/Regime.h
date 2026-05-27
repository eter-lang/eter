//===----------------------------------------------------------------------===//
//
// Part of the Eter Project, under the Apache License v2.0 with LLVM Exceptions.
// See /LICENSE for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef ETER_PARSER_REGIME_H
#define ETER_PARSER_REGIME_H

#include <cstdint>

namespace eter::parser {

/// Ownership / access regime of a binding, parameter, function return, or
/// struct field in the Eter language.
///
/// The three-regime model guarantees memory safety without a garbage collector:
///   - Imm:  immutable, shared. O(1) copy semantics for all types.
///   - Mut:  uniquely owned, mutable. Move semantics; invalidates the source.
///   - Proj: a live projection (view) of a Mut value. Reinitializable.
///   - None: no regime annotation (e.g., unit functions).
enum class Regime : uint8_t { Imm, Mut, Proj, None };

/// Return a human-readable name for a regime (useful for diagnostics).
[[nodiscard]] const char *regimeName(Regime R);

} // namespace eter::parser

#endif // ETER_PARSER_REGIME_H
