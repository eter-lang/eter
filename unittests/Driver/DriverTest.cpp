//===----------------------------------------------------------------------===//
//
// Part of the Eter Project, under the Apache License v2.0 with LLVM Exceptions.
// See /LICENSE for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "eter/Driver/Driver.h"

#include "gtest/gtest.h"

using namespace eter;

//============================================================================//
// Test: Driver parseCommandLine
//============================================================================//

TEST(DriverTest, ParseVersionFlag) {
  Driver D;
  const char *Args[] = { "eter", "--version" };
  EXPECT_TRUE(D.parseCommandLine(2, const_cast<char**>(Args)));
  EXPECT_TRUE(D.getOptions().ShowVersion);
}

TEST(DriverTest, ParseHelpFlag) {
  Driver D;
  const char *Args[] = { "eter", "--help" };
  EXPECT_TRUE(D.parseCommandLine(2, const_cast<char**>(Args)));
  EXPECT_TRUE(D.getOptions().ShowHelp);
}

TEST(DriverTest, ParseOutputFile) {
  Driver D;
  const char *Args[] = { "eter", "-o", "output.et", "input.et" };
  EXPECT_TRUE(D.parseCommandLine(4, const_cast<char**>(Args)));
  EXPECT_EQ(D.getOptions().OutputFile, "output.et");
  EXPECT_EQ(D.getOptions().InputFiles.size(), 1u);
  EXPECT_EQ(D.getOptions().InputFiles[0], "input.et");
}

TEST(DriverTest, ParseOptimizationLevel) {
  Driver D;
  const char *Args[] = { "eter", "-O0", "input.et" };
  EXPECT_TRUE(D.parseCommandLine(3, const_cast<char**>(Args)));
  EXPECT_EQ(D.getOptions().OptimizationLevel, 0);
}

TEST(DriverTest, ParseDebugFlag) {
  Driver D;
  const char *Args[] = { "eter", "--debug", "input.et" };
  EXPECT_TRUE(D.parseCommandLine(3, const_cast<char**>(Args)));
}

TEST(DriverTest, ParseDebugOnly) {
  Driver D;
  const char *Args[] = { "eter", "--debug-only=lexer", "input.et" };
  EXPECT_TRUE(D.parseCommandLine(3, const_cast<char**>(Args)));
}

TEST(DriverTest, ParseMultipleInputFiles) {
  Driver D;
  const char *Args[] = { "eter", "file1.et", "file2.et" };
  EXPECT_TRUE(D.parseCommandLine(3, const_cast<char**>(Args)));
  EXPECT_EQ(D.getOptions().InputFiles.size(), 2u);
  EXPECT_EQ(D.getOptions().InputFiles[0], "file1.et");
  EXPECT_EQ(D.getOptions().InputFiles[1], "file2.et");
}

TEST(DriverTest, ParseNoArguments) {
  Driver D;
  const char *Args[] = { "eter" };
  EXPECT_FALSE(D.parseCommandLine(1, const_cast<char**>(Args)));
}

TEST(DriverTest, GetOptionsDefaults) {
  Driver D;
  const char *Args[] = { "eter", "input.et" };
  EXPECT_TRUE(D.parseCommandLine(2, const_cast<char**>(Args)));

  const auto &Opts = D.getOptions();
  EXPECT_FALSE(Opts.ShowVersion);
  EXPECT_FALSE(Opts.ShowHelp);
  EXPECT_EQ(Opts.OptimizationLevel, 0);
  EXPECT_TRUE(Opts.OutputFile.empty());
  EXPECT_EQ(Opts.InputFiles.size(), 1u);
}
