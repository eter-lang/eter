//===----------------------------------------------------------------------===//
//
// Part of the Eter Project, under the Apache License v2.0 with LLVM Exceptions.
// See /LICENSE for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "eter/Driver/Version.h"

#include <string>

namespace eter {

namespace {

constexpr Version CurrentVersion(0, 1, 0);

// We store the formatted string in a static buffer so that the
// std::string_view returned by version() remains valid for the lifetime
// of the program.
const std::string CurrentVersionStr =
    std::to_string(CurrentVersion.Major) + "." +
    std::to_string(CurrentVersion.Minor) + "." +
    std::to_string(CurrentVersion.Patch);

} // namespace

[[nodiscard]] std::string_view version() noexcept {
  // Returning a string_view is O(1) and avoids any heap allocation
  // during the function call.
  return CurrentVersionStr;
}

} // namespace eter