//===----------------------------------------------------------------------===//
//
// Part of the Eter Project, under the Apache License v2.0 with LLVM Exceptions.
// See /LICENSE for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#pragma once

#include "eter/Base/StringInterner.h"
#include "eter/Parser/NodePool.h"
#include "eter/Parser/Parser.h"

#include <llvm/ADT/StringRef.h>
#include <llvm/Support/VirtualFileSystem.h>

#include <eter/Base/SourceBuffer.h>
#include <eter/Parser/Parser.h>
#include <string>

#include "gtest/gtest.h"

using namespace eter;
using namespace eter::parser;

namespace ParserTestHelper {

static ParseResult Pr;
static StringInterner Si;

inline SourceBuffer createTestBuffer(llvm::StringRef Content) {
  return SourceBuffer::makeFromString(Content);
}

template <typename... Kinds>
inline bool checkChildrenKinds(NodeIndex Node, Kinds... Expected) {
  // Ensure the caller only passes NodeKind arguments
  static_assert((std::is_same_v<Kinds, NodeKind> && ...),
                "All expected children arguments must be of type NodeKind");

  const llvm::ArrayRef<NodeIndex> Children = Pr.Pool.childrenOf(Node);

  // Early exit if the arity doesn't match the number of variadic arguments
  if (Children.size() != sizeof...(Expected))
    return false;

  size_t Index = 0;
  // Fold expression checking each child's kind against the expected variadic
  // pack
  return ((Pr.Pool.kindOf(Children[Index++]) == Expected) && ...);
}

inline void checkInternedString(NodeIndex Ni, std::string Expected) {
  const llvm::StringRef Stored =
      Si.get(NodePool::payloadStr(Pr.Pool[Ni].Payload));
  EXPECT_EQ(Stored, Expected);
}

} // namespace ParserTestHelper
