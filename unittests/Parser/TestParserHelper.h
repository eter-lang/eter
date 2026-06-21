//===----------------------------------------------------------------------===//
//
// Part of the Eter Project, under the Apache License v2.0 with LLVM Exceptions.
// See /LICENSE for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef UNITTESTS_PARSER_TESTPARSERHELPER_H
#define UNITTESTS_PARSER_TESTPARSERHELPER_H

#include "eter/Base/PhaseDiagnostic.h"
#include "eter/Base/SourceBuffer.h"
#include "eter/Base/StringInterner.h"
#include "eter/Lexer/Lexer.h"
#include "eter/Parser/NodePool.h"
#include "eter/Parser/Parser.h"
#include "eter/Parser/ParserDiagnostics.h"
#include "eter/Parser/Regime.h"
#include "eter/Parser/TokenStream.h"

#include <llvm/ADT/StringRef.h>
#include <llvm/Support/VirtualFileSystem.h>

#include <string>

#include "gtest/gtest.h"

using namespace eter;
using namespace eter::parser;

namespace ParserTestHelper {

static ParseResult PR;
static StringInterner SI;
static std::string SourceText;

inline SourceBuffer createTestBuffer(llvm::StringRef Content) {
  return SourceBuffer::makeFromString(Content);
}

template <typename... Kinds, std::size_t... Is>
inline bool checkChildrenKindsImpl(const llvm::ArrayRef<NodeIndex> Children,
                                   std::index_sequence<Is...>,
                                   Kinds... Expected) {
  return ((PR.Pool.kindOf(Children[Is]) == Expected) && ...);
}

template <typename... Kinds>
inline bool checkChildrenKinds(NodeIndex Node, Kinds... Expected) {
  static_assert((std::is_same_v<Kinds, NodeKind> && ...),
                "All expected children arguments must be of type NodeKind");

  const llvm::ArrayRef<NodeIndex> Children = PR.Pool.childrenOf(Node);

  if (Children.size() != sizeof...(Expected))
    return false;

  return checkChildrenKindsImpl(
      Children, std::make_index_sequence<sizeof...(Expected)>{}, Expected...);
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
  SourceText = Source.str();
  SourceBuffer SB = createTestBuffer(Source);
  auto Tokens = L.lex(SB);
  const TokenStream Ts = TokenStream(Tokens, SB.getBuffer());
  PR = Parser::parse(Ts, SI);
}

inline void checkSpan(NodeIndex NI, llvm::StringRef Expected) {
  const Span S = PR.Pool.spanOf(NI);
  const llvm::StringRef Actual =
      llvm::StringRef(SourceText).substr(S.Start, S.End - S.Start);
  EXPECT_EQ(Actual, Expected)
      << "  node span at offsets [" << S.Start << ", " << S.End << ")";
}

inline bool hasDiag(DiagID Want) {
  for (const auto &D : PR.Errors) {
    if (D.Ph != diag::Phase::Parser)
      continue;
    if (static_cast<DiagID>(D.LocalID) == Want)
      return true;
  }
  return false;
}

inline void expectDiag(DiagID Want) {
  if (!hasDiag(Want))
    ADD_FAILURE() << "expected parser diagnostic "
                  << static_cast<unsigned>(Want) << " (\""
                  << messageFor(Want).str() << "\") in ParseResult";
}

inline const eter::diag::PhaseDiagnostic *findDiag(DiagID Want) {
  for (const auto &D : PR.Errors) {
    if (D.Ph != eter::diag::Phase::Parser)
      continue;
    if (static_cast<DiagID>(D.LocalID) == Want)
      return &D;
  }
  return nullptr;
}

inline void expectDiagWithMessage(DiagID Want, llvm::StringRef MessagePart) {
  const auto *D = findDiag(Want);
  ASSERT_NE(D, nullptr) << "expected diagnostic "
                        << static_cast<unsigned>(Want);
  const std::string Message =
      eter::diag::renderMessage(messageFor(Want), D->Args);
  EXPECT_TRUE(Message.find(MessagePart.str()) != std::string::npos)
      << "message '" << Message << "' does not contain '" << MessagePart.str()
      << "'";
}

inline void expectDiagWithNote(DiagID Want, llvm::StringRef NotePart) {
  const auto *D = findDiag(Want);
  ASSERT_NE(D, nullptr) << "expected diagnostic "
                        << static_cast<unsigned>(Want);
  EXPECT_FALSE(D->Notes.empty()) << "diagnostic has no notes";
  bool Found = false;
  for (const auto &Note : D->Notes) {
    if (Note.find(NotePart.str()) != std::string::npos) {
      Found = true;
      break;
    }
  }
  EXPECT_TRUE(Found) << "no note contains '" << NotePart.str() << "'";
}

inline void expectDiagWithHelp(DiagID Want, llvm::StringRef HelpPart) {
  const auto *D = findDiag(Want);
  ASSERT_NE(D, nullptr) << "expected diagnostic "
                        << static_cast<unsigned>(Want);
  EXPECT_FALSE(D->Helps.empty()) << "diagnostic has no help";
  bool Found = false;
  for (const auto &Help : D->Helps) {
    if (Help.find(HelpPart.str()) != std::string::npos) {
      Found = true;
      break;
    }
  }
  EXPECT_TRUE(Found) << "no help contains '" << HelpPart.str() << "'";
}

} // namespace ParserTestHelper

#endif // UNITTESTS_PARSER_TESTPARSERHELPER_H
