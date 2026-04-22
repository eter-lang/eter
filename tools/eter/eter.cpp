//===----------------------------------------------------------------------===//
//
// Part of the Eter Project, under the Apache License v2.0 with LLVM Exceptions.
// See /LICENSE for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This is the main entry point for the Eter compiler. It initializes the
// compiler driver, parses command-line arguments, and starts the compilation
// process.
//
//===----------------------------------------------------------------------===//

#include "eter/Driver/Driver.h"

int main(int argc, char **argv) {
  eter::Driver Driver;

  if (!Driver.parseCommandLine(argc, argv)) {
    return 1;
  }

  return Driver.run();
}
