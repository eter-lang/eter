//===----------------------------------------------------------------------===//
//
// Part of the Eter Project, under the Apache License v2.0 with LLVM Exceptions.
// See /LICENSE for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef ETER_BASE_SOURCEBUFFER_H
#define ETER_BASE_SOURCEBUFFER_H

#include "eter/Base/DiagnosticEngine.h"

#include <llvm/ADT/StringRef.h>
#include <llvm/Support/Error.h>
#include <llvm/Support/MemoryBuffer.h>
#include <llvm/Support/VirtualFileSystem.h>

namespace eter {

/// This class represents a source file buffer, which contains the contents of a
/// Eter source file. It provides utilities for loading the file into memory and
/// accessing its contents.
///
/// The buffer is backed by an `llvm::MemoryBuffer`, which may represent
/// either:
///  - heap-allocated memory, or
///  - a memory-mapped file (mmap), or
///  - other OS-backed storage provided by LLVM VFS.
///
/// Because of this, `SourceBuffer` is move-only and explicitly non-copyable.
/// Copying would require defining complex ownership semantics for the
/// underlying memory region (e.g., duplicating or sharing an mmap), which is
/// intentionally avoided.
///
/// Instances must be created through factory methods (e.g. `makeFromFileName`)
/// to ensure proper initialization and error handling.
///
/// The class guarantees that the underlying source text remains valid for the
/// lifetime of the `SourceBuffer`.
class SourceBuffer {
public:
  /// Create a `SourceBuffer` from a file name.
  /// \param FS The virtual file system to use for file access.
  /// \param Filename The name of the file to load.
  /// \param SDE The simple diagnostic engine for reporting errors.
  /// \returns The loaded source buffer on success, or an error on failure.
  static llvm::Expected<SourceBuffer>
  makeFromFileName(llvm::vfs::FileSystem &FS, llvm::StringRef Filename,
                    SimpleDiagnosticEngine &SDE);

  /// Create a `SourceBuffer` from a string (useful for testing).
  /// \param Content The string content to load into the buffer.
  /// \param Name An optional name for the buffer (default: "buffer").
  /// \returns A source buffer containing the provided string.
  static SourceBuffer makeFromString(llvm::StringRef Content,
                                    llvm::StringRef Name = "buffer");

  /// Return the name of the source file associated with this buffer.
  llvm::StringRef getFilename() const;

  /// Return the contents of the source file as a string reference.
  llvm::StringRef getBuffer() const;

  /// Deleted default constructor to prevent creating an empty `SourceBuffer`.
  /// Use the factory methods (e.g., `makeFromFileName`) to create instances
  /// instead.
  SourceBuffer() = delete;

private:
  /// Create a `SourceBuffer` from an existing memory buffer.
  /// \param Buffer The memory buffer containing the file contents.
  /// \param Filename The name of the source file.
  /// \param SDE The simple diagnostic engine for reporting errors.
  /// \returns The loaded source buffer on success, or an error on failure.
  static llvm::Expected<SourceBuffer>
  makeFromMemoryBuffer(std::unique_ptr<llvm::MemoryBuffer> Buffer,
                       llvm::StringRef Filename, SimpleDiagnosticEngine &SDE);

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
