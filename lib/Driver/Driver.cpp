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
#include "eter/Base/PhaseDiagnostic.h"
#include "eter/Base/SourceBuffer.h"
#include "eter/Base/SourceManager.h"
#include "eter/Driver/Driver.h"
#include "eter/Driver/PackSession.h"
#include "eter/Driver/Version.h"
#include "eter/Lexer/Lexer.h"
#include "eter/Parser/NodePool.h"
#include "eter/Parser/Parser.h"
#include "eter/Parser/ParserDiagnostics.h"
#include "eter/Parser/TokenStream.h"

#include <llvm/ADT/ScopeExit.h>
#include <llvm/ADT/StringRef.h>
#include <llvm/Support/Error.h>
#include <llvm/Support/VirtualFileSystem.h>
#include <llvm/Support/raw_ostream.h>

#include <cstddef>
#include <filesystem>
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
      // If an argument doesn't start with '-', it's an input file.
      Options.InputFiles.push_back(std::string(Arg));
    } else {
      llvm::errs() << "error: Unknown option: " << Arg << "\n\n";
      printHelp();
      return false;
    }
  }

  if (Options.InputFiles.empty() && !Options.ShowVersion && !Options.ShowHelp) {
    llvm::errs() << "error: No input files specified\n\n";
    printHelp();
    return false;
  }

  // For now, we support only one input file.
  if (Options.InputFiles.size() != 1) {
    llvm::errs() << "error: only one input file is supported\n\n";
    printHelp();
    return 1;
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

  for (const auto &InputFilename : Options.InputFiles) {
    const int Result = compilePack(InputFilename);
    if (Result != 0)
      return Result;
  }

  return 0;
}

int Driver::compilePack(const std::string &RootFile) {
  ETER_DEBUG(llvm::dbgs() << "[" DEBUG_TYPE "] compilePack root=" << RootFile
                          << "\n");

  PackSession Session;
  llvm::StringSet<> InProgress;

  const int Rc = parseFile(RootFile, Session, InProgress);
  if (Rc != 0)
    return Rc;

  ETER_DEBUG(llvm::dbgs() << "[" DEBUG_TYPE "] pack parsed: "
                          << Session.Results.size() << " file(s)\n");

  // TODO: semantic analysis, lowering to MLIR, optimisation, codegen.
  return 0;
}

int Driver::parseFile(const std::string &Path, PackSession &Session,
                      llvm::StringSet<> &InProgress) {
  // Already parsedm, nothing to do.
  if (Session.Results.count(Path))
    return 0;

  // Cycle detection.
  if (InProgress.count(Path)) {
    llvm::errs() << "error: circular mod dependency detected for '" << Path
                 << "'\n";
    return 1;
  }

  ETER_DEBUG(llvm::dbgs() << "[" DEBUG_TYPE "] parseFile " << Path << "\n");

  // Load the source file.
  const llvm::IntrusiveRefCntPtr<llvm::vfs::FileSystem> FS =
      llvm::vfs::getRealFileSystem();
  SimpleDiagnosticEngine SDE;
  auto ExpectedBuf = SourceBuffer::makeFromFileName(*FS, Path, SDE);
  if (!ExpectedBuf) {
    llvm::errs() << "error: cannot open '" << Path
                 << "': " << llvm::toString(ExpectedBuf.takeError()) << "\n";
    return 1;
  }
  // Lex.
  lexer::Lexer L;
  // Lex errors must be captured before TokenStream is moved into parse().
  parser::TokenStream TS(L.lex(*ExpectedBuf), ExpectedBuf->getBuffer());

  // Parse.
  parser::ParseResult Result =
      parser::Parser::parse(std::move(TS), Session.Interner);

  // Emit diagnostics.
  DiagnosticEngine DE =
      std::move(SDE).withSourceManager(SourceManager(*ExpectedBuf));
  for (const auto &LErr : TS.lexErrors())
    DE.error(LErr.ErrorSpan)
        .message(lexer::LexerError::getErrorString(LErr.ErrorKind))
        .emit();
  for (const auto &PErr : Result.Errors) {
    const std::string Rendered = diag::renderMessage(
        parser::messageFor(static_cast<parser::DiagID>(PErr.LocalID)),
        PErr.Args);
    auto B = DE.error(PErr.Loc).message(Rendered);
    for (const auto &L : PErr.Labels)
      B.label(L.DiagSpan, L.Message);
    for (const auto &N : PErr.Notes)
      B.note(N);
    B.emit();
  }
  DE.printAll();

  // Walk ModDeclFile nodes to discover child files.
  const parser::NodePool &Pool = Result.Pool;
  const std::filesystem::path Dir = std::filesystem::path(Path).parent_path();

  InProgress.insert(Path);
  auto Cleanup = llvm::scope_exit([&] { InProgress.erase(Path); });

  for (auto Child : Pool.childrenOf(Result.Root)) {
    if (Pool.kindOf(Child) != parser::NodeKind::ModDeclFile)
      continue;

    const llvm::StringRef ModName =
        Session.Interner.get(parser::NodePool::payloadStr(Pool[Child].Payload));

    // Try foo.et, then foo/mod.et (similar to Rust 2018 convention).
    const std::string FileDotEt = (Dir / (ModName.str() + ".et")).string();
    const std::string ModDotEt = (Dir / ModName.str() / "mod.et").string();

    std::string ChildPath;
    if (std::filesystem::exists(FileDotEt))
      ChildPath = FileDotEt;
    else if (std::filesystem::exists(ModDotEt))
      ChildPath = ModDotEt;
    else {
      llvm::errs() << "cannot find file for 'mod " << ModName << ";' (tried '"
                   << FileDotEt << "' and '" << ModDotEt << "')\n";
      return 1;
    }

    const int Rc = parseFile(ChildPath, Session, InProgress);
    if (Rc != 0)
      return Rc;
  }

  // Store the result only after all children are processed.
  Session.Results.insert({Path, std::move(Result)});
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
