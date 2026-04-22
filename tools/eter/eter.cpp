//===----------------------------------------------------------------------===//
//
// Part of the Eter Project, under the Apache License v2.0 with LLVM Exceptions.
// See /LICENSE for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
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
