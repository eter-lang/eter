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

const std::string CurrentVersionStr =
    std::to_string(CurrentVersion.Major) + "." +
    std::to_string(CurrentVersion.Minor) + "." +
    std::to_string(CurrentVersion.Patch);

} // namespace

[[nodiscard]] std::string_view version() noexcept { return CurrentVersionStr; }

} // namespace eter