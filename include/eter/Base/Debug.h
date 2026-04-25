//===----------------------------------------------------------------------===//
//
// Part of the Eter Project, under the Apache License v2.0 with LLVM Exceptions.
// See /LICENSE for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef ETER_BASE_DEBUG_H
#define ETER_BASE_DEBUG_H

#include <llvm/ADT/StringRef.h>
#include <llvm/Support/Debug.h>

namespace eter {

/// Global flag controlling whether debug output is enabled.
/// Set via --debug or --debug-only on the command line.
extern bool DebugFlag;

/// Current debug type filter. When non-empty, only debug output for this
/// type is emitted. Set via --debug-only=<type> on the command line.
extern llvm::StringRef CurrentDebugType;

} // namespace eter

#ifdef ETER_ENABLE_DEBUG
#define ETER_DEBUG_WITH_TYPE(TYPE, X)                                          \
  do {                                                                         \
    if (eter::DebugFlag && eter::CurrentDebugType.empty()) {                   \
      X;                                                                       \
    } else if (eter::DebugFlag &&                                              \
               llvm::StringRef(TYPE) == eter::CurrentDebugType) {              \
      X;                                                                       \
    }                                                                          \
  } while (0)
#define ETER_DEBUG(X) ETER_DEBUG_WITH_TYPE(DEBUG_TYPE, X)
#else
#define ETER_DEBUG_WITH_TYPE(TYPE, X)                                          \
  do {                                                                         \
  } while (0)
#define ETER_DEBUG(X)                                                          \
  do {                                                                         \
  } while (0)
#endif

#endif // ETER_BASE_DEBUG_H
