//===----------------------------------------------------------------------===//
//
// Part of the Eter Project, under the Apache License v2.0 with LLVM Exceptions.
// See /LICENSE for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef ETER_BASE_SOURCEMANAGER_H
#define ETER_BASE_SOURCEMANAGER_H

#include <llvm/ADT/DenseMap.h>
#include <llvm/ADT/SmallVector.h>
#include <llvm/ADT/StringRef.h>
#include <llvm/Support/Error.h>
#include <llvm/Support/MemoryBuffer.h>
#include <llvm/Support/VirtualFileSystem.h>

namespace eter {

/// A source range identified by byte offsets into a source buffer.
struct Span {
  uint32_t Start = 0;
  uint32_t End = 0;

  Span() = default;
  Span(uint32_t Start, uint32_t End) : Start(Start), End(End) {}
};

/// A line and column position in a source file.
struct SourceLocation {
  uint32_t Line = 0;
  uint32_t Column = 0;
};

class SourceBuffer;

/// Manages source buffer access and resolves byte offsets to line/column
/// positions.
class SourceManager {
public:
  /// Construct a source manager from a source buffer.
  explicit SourceManager(const SourceBuffer &Buffer);

  /// Return the full contents of the source buffer.
  llvm::StringRef getBuffer() const;

  /// Return a substring of the source buffer for the given span.
  llvm::StringRef slice(Span Span) const;

  /// Resolve a byte offset to a line and column location.
  SourceLocation getLocation(uint32_t Offset) const;

  /// Return the filename of the source buffer.
  llvm::StringRef getFilename() const;

private:
  /// Build the line start offset index.
  void buildLineIndex();

private:
  const SourceBuffer &Buffer;

  llvm::SmallVector<uint32_t, 256> LineStarts;
  mutable llvm::DenseMap<uint32_t, SourceLocation> Cache;
};

} // namespace eter

#endif // ETER_BASE_SOURCEMANAGER_H
