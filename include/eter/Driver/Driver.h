//===----------------------------------------------------------------------===//
//
// Part of the Eter Project, under the Apache License v2.0 with LLVM Exceptions.
// See /LICENSE for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef ETER_DRIVER_DRIVER_H
#define ETER_DRIVER_DRIVER_H

#include <vector>

namespace eter {

/// Represents command-line options for the Eter compiler.
struct CompilerOptions {
  std::vector<std::string> InputFiles;
  std::string OutputFile;
  bool ShowVersion = false;
  bool ShowHelp = false;
  int OptimizationLevel = 0; // -O0
};

/// Main compiler driver that orchestrates the compilation pipeline.
class Driver {
public:
  Driver();

  /// Parse command-line arguments.
  /// \returns true if parsing was successful, false otherwise.
  bool parseCommandLine(int Argc, char **Argv);

  /// Run the compilation pipeline with the parsed options.
  /// \returns 0 on success, non-zero error code on failure.
  int run();

  /// Get the parsed compiler options.
  const CompilerOptions &getOptions() const { return Options; }

  /// Print help message.
  void printHelp() const;

  /// Print version information.
  void printVersion() const;

private:
  CompilerOptions Options;

  /// Process a single source file through the compilation pipeline.
  /// \returns 0 on success, non-zero error code on failure.
  int compileFile(const std::string &InputFile);
};

} // namespace eter

#endif // ETER_DRIVER_DRIVER_H
