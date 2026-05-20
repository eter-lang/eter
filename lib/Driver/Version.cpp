//===----------------------------------------------------------------------===//
//
// Part of the Eter Project, under the Apache License v2.0 with LLVM Exceptions.
// See /LICENSE for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "eter/Driver/Version.h"

#include <llvm/ADT/StringRef.h>
#include <string>

namespace eter {

namespace {

// Represents the version of the Eter compiler.
struct Version {
  const int Major;
  const int Minor;
  const int Patch;

  constexpr Version(int Major, int Minor, int Patch) noexcept
      : Major(Major), Minor(Minor), Patch(Patch) {}
};

constexpr Version CurrentVersion(0, 1, 0);

const std::string CurrentVersionStr =
    std::to_string(CurrentVersion.Major) + "." +
    std::to_string(CurrentVersion.Minor) + "." +
    std::to_string(CurrentVersion.Patch);

} // namespace

[[nodiscard]] llvm::StringRef version() noexcept { return CurrentVersionStr; }

} // namespace eter