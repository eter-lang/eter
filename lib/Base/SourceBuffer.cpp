//===----------------------------------------------------------------------===//
//
// Part of the Eter Project, under the Apache License v2.0 with LLVM Exceptions.
// See /LICENSE for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "eter/Base/DiagnosticEngine.h"
#include "eter/Base/SourceBuffer.h"

#include <llvm/Support/ErrorOr.h>
#include <llvm/Support/MemoryBuffer.h>
#include <llvm/Support/VirtualFileSystem.h>
#include <llvm/Support/raw_ostream.h>

namespace eter {

llvm::Expected<SourceBuffer>
SourceBuffer::makeFromFileName(llvm::vfs::FileSystem &FS,
                               llvm::StringRef Filename,
                               SimpleDiagnosticEngine &SDE) {
  llvm::ErrorOr<std::unique_ptr<llvm::MemoryBuffer>> FileOrErr =
      FS.getBufferForFile(Filename);

  if (!FileOrErr) {
    const std::error_code EC = FileOrErr.getError();

    SDE.error()
        .atFile(Filename)
        .message("failed to open file")
        .note("file: " + Filename.str())
        .note("reason: " + EC.message())
        .emit();

    return llvm::createStringError(EC, "failed to open file '%s': %s",
                                   Filename.str().c_str(),
                                   EC.message().c_str());
  }

  return makeFromMemoryBuffer(std::move(*FileOrErr), Filename, SDE);
}

llvm::Expected<SourceBuffer>
SourceBuffer::makeFromMemoryBuffer(std::unique_ptr<llvm::MemoryBuffer> Buffer,
                                   llvm::StringRef Filename,
                                   SimpleDiagnosticEngine &SDE) {
  if (!Buffer) {
    SDE.error()
        .atFile(Filename)
        .message("failed to read file into memory")
        .note("file: " + Filename.str())
        .emit();

    return llvm::createStringError(llvm::inconvertibleErrorCode(),
                                   "failed to read file '%s' into memory",
                                   Filename.str().c_str());
  }

  return SourceBuffer(Filename.str(), std::move(Buffer));
}

llvm::StringRef SourceBuffer::getFilename() const { return Filename; }

llvm::StringRef SourceBuffer::getBuffer() const { return Buffer->getBuffer(); }

SourceBuffer SourceBuffer::makeFromString(llvm::StringRef Content,
                                        llvm::StringRef Name) {
  std::unique_ptr<llvm::MemoryBuffer> Buffer =
      llvm::MemoryBuffer::getMemBuffer(Content, Name);
  return SourceBuffer(Name.str(), std::move(Buffer));
}

} // namespace eter
