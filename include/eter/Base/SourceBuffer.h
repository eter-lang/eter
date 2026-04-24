//===----------------------------------------------------------------------===//
//
// Part of the Eter Project, under the Apache License v2.0 with LLVM Exceptions.
// See /LICENSE for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef ETER_BASE_SOURCEBUFFER_H
#define ETER_BASE_SOURCEBUFFER_H

#include <llvm/ADT/StringRef.h>
#include <llvm/Support/Error.h>
#include <llvm/Support/MemoryBuffer.h>
#include <llvm/Support/VirtualFileSystem.h>

namespace eter {

/// This class represents a source file buffer, which contains the contents of a
/// Eter source file. It provides utilities for loading the file into memory and
/// accessing its contents.
///
/// The `SourceBuffer` class is responsible for managing the memory buffer that
/// holds the source file contents. It provides a static factory method to
/// create a `SourceBuffer` instance from a file name, using the LLVM virtual
/// file system (VFS).
class SourceBuffer {
public:
  /// Create a `SourceBuffer` from a file name. On failure, print an error
  /// message and return an `llvm::Error`.
  static llvm::Expected<SourceBuffer>
  makeFromFileName(llvm::vfs::FileSystem &FS, llvm::StringRef Filename);

  /// Deleted default constructor to prevent creating an empty `SourceBuffer`.
  /// Use the factory methods (e.g., `makeFromFileName`) to create instances
  /// instead.
  SourceBuffer() = delete;

private:
  /// Create a `SourceBuffer` from an existing memory buffer.
  /// On failure, print an error message and return an `llvm::Error`.
  static llvm::Expected<SourceBuffer>
  makeFromMemoryBuffer(std::unique_ptr<llvm::MemoryBuffer> Buffer,
                       llvm::StringRef Filename);

  explicit SourceBuffer(std::string Filename,
                        std::unique_ptr<llvm::MemoryBuffer> Buffer)
      : Filename(std::move(Filename)), Buffer(std::move(Buffer)) {}

  /// The contents of the source file.
  std::string Filename;

  /// The memory buffer containing the file contents.
  std::unique_ptr<llvm::MemoryBuffer> Buffer;
};

} // namespace eter

#endif // ETER_BASE_SOURCEBUFFER_H