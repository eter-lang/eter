//===----------------------------------------------------------------------===//
//
// Part of the Eter Project, under the Apache License v2.0 with LLVM Exceptions.
// See /LICENSE for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "eter/Base/SourceBuffer.h"
#include <llvm/Support/ErrorOr.h>
#include <llvm/Support/MemoryBuffer.h>
#include <llvm/Support/VirtualFileSystem.h>
#include <llvm/Support/raw_ostream.h>

namespace eter {

llvm::Expected<SourceBuffer>
SourceBuffer::makeFromFileName(llvm::vfs::FileSystem &FS,
                               llvm::StringRef Filename) {
  llvm::ErrorOr<std::unique_ptr<llvm::MemoryBuffer>> FileOrErr =
      FS.getBufferForFile(Filename);

  if (!FileOrErr) {
    return llvm::createStringError(
        std::make_error_code(std::errc::no_such_file_or_directory),
        "Failed to open file: %s", Filename.data());
  }

  return makeFromMemoryBuffer(std::move(*FileOrErr), Filename);
}

llvm::Expected<SourceBuffer>
SourceBuffer::makeFromMemoryBuffer(std::unique_ptr<llvm::MemoryBuffer> Buffer,
                                   llvm::StringRef Filename) {
  if (!Buffer) {
    return llvm::createStringError(
        std::make_error_code(std::errc::no_such_file_or_directory),
        "Failed to create memory buffer for file: %s", Filename.data());
  }

  return SourceBuffer(Filename.str(), std::move(Buffer));
}

llvm::StringRef SourceBuffer::getFilename() const { return Filename; }

llvm::StringRef SourceBuffer::getBuffer() const { return Buffer->getBuffer(); }

} // namespace eter
