//===----------------------------------------------------------------------===//
//
// Part of the Eter Project, under the Apache License v2.0 with LLVM Exceptions.
// See /LICENSE for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef ETER_DRIVER_VERSION_H
#define ETER_DRIVER_VERSION_H

#include <string>

namespace eter {

// Provide a simple global function returning the version string. Keep this
// minimal so it can be used during startup and diagnostics.
std::string getVersionString();

} // namespace eter

#endif // ETER_DRIVER_VERSION_H
