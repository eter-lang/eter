//===----------------------------------------------------------------------===//
//
// Part of the Eter Project, under the Apache License v2.0 with LLVM Exceptions.
// See /LICENSE for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#define DEBUG_TYPE "driver"

#include "eter/Base/Debug.h"
#include "eter/Base/DiagnosticEngine.h"
#include "eter/Base/SourceBuffer.h"
#include "eter/Base/SourceManager.h"
#include "eter/Driver/Driver.h"
#include "eter/Driver/Version.h"
#include "eter/Driver/DumpTokens.h"

#include <llvm/Support/Error.h>
#include <llvm/Support/VirtualFileSystem.h>
#include <llvm/Support/raw_ostream.h>

#include <cstddef>
#include <iostream>
#include <string>

namespace eter {

Driver::Driver() = default;

bool Driver::parseCommandLine(int Argc, char **Argv) {
  if (Argc < 2) {
    printHelp();
    return false;
  }

  for (int I = 1; I < Argc; ++I) {
    const llvm::StringRef Arg(Argv[I]);

    if (Arg == "--version") {
      Options.ShowVersion = true;
      return true;
    }
    if (Arg == "--help" || Arg == "-h") {
      Options.ShowHelp = true;
      return true;
    }

    if (Arg == "--debug") {
      eter::DebugFlag = true;
    } else if (Arg.starts_with("--debug-only=")) {
      eter::DebugFlag = true;
      eter::CurrentDebugType =
          llvm::StringRef(Arg.data() + sizeof("--debug-only=") - 1);
    } else if (Arg == "-O0") {
      Options.OptimizationLevel = 0;
    } else if (Arg == "-o" && I + 1 < Argc) {
      Options.OutputFile = Argv[++I];
    } else if (Arg[0] != '-') {
      // If an argument does not start with a '-', treat it as an input file.
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
  for (const auto &InputFilename : Options.InputFiles) {
    const int Result = compileFile(InputFilename);
    if (Result != 0) {
      return Result;
    }
  }

  return 0;
}

int Driver::compileFile(const std::string &InputFilename) {
  ETER_DEBUG(llvm::dbgs() << "eter: remark: Compiling: " << InputFilename
                          << "\n");

  const llvm::IntrusiveRefCntPtr<llvm::vfs::FileSystem> FS =
      llvm::vfs::getRealFileSystem();
  SimpleDiagnosticEngine SDE;
  auto ExpectedBuffer = SourceBuffer::makeFromFileName(*FS, InputFilename, SDE);
  if (!ExpectedBuffer) {
    llvm::errs() << "eter: error: Failed to load source file '" << InputFilename
                 << "': " << llvm::toString(ExpectedBuffer.takeError()) << "\n";
    return 1;
  }

  ETER_DEBUG(llvm::dbgs() << "eter: remark: Loaded source file: "
                          << InputFilename << "\n");

  const DiagnosticEngine DE =
      std::move(SDE).withSourceManager(SourceManager(*ExpectedBuffer));
  
  // Dump tokens if requested (for now we always dump tokens to satisfy requirement)
  // Or we can just call dumpTokens and comment it out or keep it simple.
  dumpTokens(*ExpectedBuffer);

  return 0;

  // TODO: Implement the actual compilation pipeline:
  // 1. Lexical analysis (tokenization)
  // 2. Parsing (build AST)
  // 3. Semantic analysis
  // 4. Lowering to MLIR
  // 5. Optimization passes
  // 6. Code generation

  ETER_DEBUG(llvm::dbgs() << "eter: remark: Optimization level: -O "
                          << Options.OptimizationLevel << "\n");

  if (!Options.OutputFile.empty()) {
    ETER_DEBUG(llvm::dbgs()
               << "eter: remark: Output file: " << Options.OutputFile << "\n");
  }

  return 0;
}

void Driver::printHelp() const {
  std::cout << "Eter Compiler " << eter::version().str() << "\n"
            << "\nUsage: eter [options] <input-files>\n"
            << "\nOptions:\n"
            << "  -h, --help              Show this help message\n"
            << "  --version               Show version information\n"
            << "  -o <output>             Specify output file\n"
            << "  -O0                     No optimization (default)\n"
            << "  --debug                 Enable debug output\n"
            << "  --debug-only=<type>     Enable debug output only for <type>\n"
            << "\nExamples:\n"
            << "  eterc --version\n"
            << "  eterc program.eter\n"
            << "  eterc program.eter -o program\n";
}

void Driver::printVersion() const {
  std::cout << "Eter compiler version " << eter::version().str() << '\n';
}

} // namespace eter
