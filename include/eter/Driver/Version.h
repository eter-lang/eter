//===----------------------------------------------------------------------===//
//
// Part of the Eter Project, under the Apache License v2.0 with LLVM Exceptions.
// See /LICENSE for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef ETER_DRIVER_VERSION_H
#define ETER_DRIVER_VERSION_H

#include <string_view>

namespace eter {

/// The current version of the Eter compiler.
[[nodiscard]] std::string_view version() noexcept;

} // namespace eter

#endif // ETER_DRIVER_VERSION_H
