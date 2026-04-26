//===----------------------------------------------------------------------===//
//
// Part of the Eter Project, under the Apache License v2.0 with LLVM Exceptions.
// See /LICENSE for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "eter/Base/SourceBuffer.h"
#include "eter/Base/SourceManager.h"

#include <algorithm>

#include "llvm/ADT/STLExtras.h"

namespace eter {

SourceManager::SourceManager(const SourceBuffer &Buffer) : Buffer(Buffer) {
  buildLineIndex();
}

llvm::StringRef SourceManager::getBuffer() const { return Buffer.getBuffer(); }

llvm::StringRef SourceManager::getFilename() const {
  return Buffer.getFilename();
}

void SourceManager::buildLineIndex() {
  const llvm::StringRef Data = getBuffer();

  LineStarts.push_back(0);

  for (uint32_t I = 0; I < Data.size(); I++) {
    if (Data[I] == '\n')
      LineStarts.push_back(I + 1);
  }
}

SourceLocation SourceManager::getLocation(uint32_t Offset) const {
  const llvm::DenseMap<uint32_t, SourceLocation>::const_iterator It =
      Cache.find(Offset);
  if (It != Cache.end())
    return It->second;

  // O(log N) binary search the offset
  const uint32_t Line =
      std::distance(LineStarts.begin(), llvm::upper_bound(LineStarts, Offset)) -
      1;

  SourceLocation Loc;
  Loc.Line = Line + 1;
  Loc.Column = Offset - LineStarts[Line] + 1;

  Cache[Offset] = Loc;
  return Loc;
}

llvm::StringRef SourceManager::slice(Span Span) const {
  const llvm::StringRef Buf = getBuffer();
  return Buf.slice(Span.Start, Span.End);
}

} // namespace eter
