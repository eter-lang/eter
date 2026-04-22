//===----------------------------------------------------------------------===//
//
// Part of the Eter Project, under the Apache License v2.0 with LLVM Exceptions.
// See /LICENSE for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "eter/Driver/Driver.h"
#include "eter/Driver/Version.h"

#include "llvm/Support/raw_ostream.h"

#include <iostream>
#include <string>
#include <string_view>

namespace eter {

Driver::Driver() = default;

bool Driver::parseCommandLine(int Argc, char **Argv) {
  if (Argc < 2) {
    printHelp();
    return false;
  }

  for (int I = 1; I < Argc; ++I) {
    std::string_view Arg(Argv[I]);

    if (Arg == "--version") {
      Options.ShowVersion = true;
      return true;
    }
    if (Arg == "--help" || Arg == "-h") {
      Options.ShowHelp = true;
      return true;
    }

    if (Arg == "-O0") {
      Options.OptimizationLevel = 0;
    } else if (Arg == "-o" && I + 1 < Argc) {
      Options.OutputFile = Argv[++I];
    } else if (Arg[0] != '-') {
      // Treat as input file
      Options.InputFiles.push_back(std::string(Arg));
    } else {
      llvm::errs() << "eter: error: Unknown option: " << Arg << "\n\n";
      printHelp();
      return false;
    }
  }

  if (Options.InputFiles.empty() && !Options.ShowVersion && !Options.ShowHelp) {
    llvm::errs() << "eter: error: No input files specified\n\n";
    printHelp();
    return false;
  }

  return true;
}

int Driver::run() {
  if (Options.ShowVersion) {
    printVersion();
    return 0;
  }

  if (Options.ShowHelp) {
    printHelp();
    return 0;
  }

  // Compile each input file
  for (const auto &InputFile : Options.InputFiles) {
    int Result = compileFile(InputFile);
    if (Result != 0) {
      return Result;
    }
  }

  return 0;
}

int Driver::compileFile(const std::string &InputFile) {
  llvm::outs() << "eter: remark: Compiling: " << InputFile << "\n";

  // TODO: Implement the actual compilation pipeline:
  // 1. Lexical analysis (tokenization)
  // 2. Parsing (build AST)
  // 3. Semantic analysis
  // 4. Lowering to MLIR
  // 5. Optimization passes
  // 6. Code generation

  llvm::outs() << "eter: remark: Optimization level: -O"
               << Options.OptimizationLevel << "\n";

  if (!Options.OutputFile.empty()) {
    llvm::outs() << "eter: remark: Output file: " << Options.OutputFile << "\n";
  }

  return 0;
}

void Driver::printHelp() const {
  std::cout << "Eter Compiler " << eter::version() << "\n"
            << "\nUsage: eter [options] <input-files>\n"
            << "\nOptions:\n"
            << "  -h, --help              Show this help message\n"
            << "  --version               Show version information\n"
            << "  -o <output>             Specify output file\n"
            << "  -O0                     No optimization (default)\n"
            << "\nExamples:\n"
            << "  eterc --version\n"
            << "  eterc program.eter\n"
            << "  eterc program.eter -o program\n";
}

void Driver::printVersion() const {
  std::cout << "Eter compiler version " << eter::version() << '\n';
}

} // namespace eter
