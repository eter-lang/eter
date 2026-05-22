//===----------------------------------------------------------------------===//
//
// Part of the Eter Project, under the Apache License v2.0 with LLVM Exceptions.
// See /LICENSE for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "eter/Parser/ASTNodes.h"

#include <llvm/Support/VirtualFileSystem.h>

#include <variant>
#include <vector>

#include "gtest/gtest.h"

using namespace eter;
using namespace eter::parser;

// Testing the correct use of test suite. Remember to remove!
#include <iostream>
using namespace std;

TEST(ParserTest, TesterTest) { cout << "ASTNodesTest is running" << endl; }
