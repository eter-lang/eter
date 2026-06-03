//===----------------------------------------------------------------------===//
//
// Part of the Eter Project, under the Apache License v2.0 with LLVM Exceptions.
// See /LICENSE for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef UNITTESTS_PARSER_TESTPARSERHELPER_H
#define UNITTESTS_PARSER_TESTPARSERHELPER_H

#include "eter/Base/SourceBuffer.h"
#include "eter/Base/StringInterner.h"
#include "eter/Lexer/Lexer.h"
#include "eter/Parser/NodePool.h"
#include "eter/Parser/Parser.h"
#include "eter/Parser/Regime.h"
#include "eter/Parser/TokenStream.h"

#include <llvm/ADT/StringRef.h>
#include <llvm/Support/VirtualFileSystem.h>

#include <eter/Base/SourceBuffer.h>
#include <eter/Parser/Parser.h>
#include <string>

#include "gtest/gtest.h"

using namespace eter;
using namespace eter::parser;

namespace ParserTestHelper {

static ParseResult PR;
static StringInterner SI;

inline SourceBuffer createTestBuffer(llvm::StringRef Content) {
  return SourceBuffer::makeFromString(Content);
}

template <typename... Kinds>
inline bool checkChildrenKinds(NodeIndex Node, Kinds... Expected) {
  // Ensure the caller only passes NodeKind arguments
  static_assert((std::is_same_v<Kinds, NodeKind> && ...),
                "All expected children arguments must be of type NodeKind");

  const llvm::ArrayRef<NodeIndex> Children = PR.Pool.childrenOf(Node);

  // Early exit if the arity doesn't match the number of variadic arguments
  if (Children.size() != sizeof...(Expected))
    return false;

  // NOLINTNEXTILNE (misc-const-correctness)
  size_t Index = 0;
  // Fold expression checking each child's kind against the expected variadic
  // pack
  return ((PR.Pool.kindOf(Children[Index++]) == Expected) && ...);
}

inline void checkInternedString(NodeIndex NI, std::string Expected) {
  const llvm::StringRef Stored =
      SI.get(NodePool::payloadStr(PR.Pool[NI].Payload));
  EXPECT_EQ(Stored, Expected);
}

inline void checkRegime(NodeIndex NI, Regime Expected) {
  EXPECT_EQ(NodePool::payloadRegime(PR.Pool[NI].Payload), Expected);
}

inline void parseSource(llvm::StringRef Source) {
  eter::lexer::Lexer L;
  SI = StringInterner();
  SourceBuffer SB = createTestBuffer(Source);
  auto Tokens = L.lex(SB);
  const TokenStream Ts = TokenStream(Tokens, SB.getBuffer());
  PR = Parser::parse(Ts, SI);
}

} // namespace ParserTestHelper

#endif // UNITTESTS_PARSER_TESTPARSERHELPER_H
