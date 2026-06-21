//===----------------------------------------------------------------------===//
//
// Part of the Eter Project, under the Apache License v2.0 with LLVM Exceptions.
// See /LICENSE for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "eter/Base/Debug.h"
#include "eter/Parser/NodePool.h"

#include <llvm/Support/ErrorHandling.h>
#include <llvm/Support/raw_ostream.h>

#define DEBUG_TYPE "parser-pool"

namespace eter::parser {

const char *nodeKindName(NodeKind K) {
  switch (K) {
#define ETER_NODE(X)                                                           \
  case NodeKind::X:                                                            \
    return #X;
#include "eter/Parser/NodeKinds.def"
  }
  return "<unknown>";
}

NodePool::NodePool() {
  // Index 0 is reserved as a sentinel; push a dummy record so that any
  // accidental use of index 0 is immediately visible in debug output.
  Nodes.push_back(NodeData{NodeKind::Error, 0, 0, 0, 0});
  Spans.push_back(Span{0, 0});
}

NodeIndex NodePool::alloc(NodeKind Kind, Span S,
                          llvm::ArrayRef<NodeIndex> ChildNodes,
                          uint32_t Payload, uint32_t Flags) {
  ETER_DEBUG(llvm::dbgs() << "[" DEBUG_TYPE "] alloc " << nodeKindName(Kind)
                          << " children=" << ChildNodes.size() << "\n");

  auto ChildrenBegin = static_cast<uint32_t>(Children.size());
  Children.insert(Children.end(), ChildNodes.begin(), ChildNodes.end());

  NodeIndex Idx{static_cast<uint32_t>(Nodes.size())};
  Nodes.push_back(NodeData{Kind, static_cast<uint16_t>(ChildNodes.size()),
                           ChildrenBegin, Payload, Flags});
  Spans.push_back(S);
  return Idx;
}

NodeIndex NodePool::allocLeaf(NodeKind Kind, Span S, uint32_t Payload) {
  return alloc(Kind, S, {}, Payload);
}

const NodeData &NodePool::operator[](NodeIndex I) const {
  assert(I.Value < Nodes.size() && "NodeIndex out of bounds");
  return Nodes[I.Value];
}

void NodePool::reserve(size_t NodeHint) {
  Nodes.reserve(NodeHint + 1); // +1 for the sentinel at index 0
  Spans.reserve(NodeHint + 1);
  Children.reserve(NodeHint * 2); // rough heuristic: ~2 children per node
}

NodeKind NodePool::kindOf(NodeIndex I) const { return (*this)[I].Kind; }

uint32_t NodePool::flagsOf(NodeIndex I) const { return (*this)[I].Flags; }

Span NodePool::spanOf(NodeIndex I) const {
  assert(I.Value < Spans.size() && "NodeIndex out of bounds");
  return Spans[I.Value];
}

llvm::ArrayRef<NodeIndex> NodePool::childrenOf(NodeIndex I) const {
  const NodeData &N = (*this)[I];
  return llvm::ArrayRef<NodeIndex>(Children.data() + N.ChildrenBegin,
                                   N.ChildCount);
}

NodeIndex NodePool::childAt(NodeIndex I, uint16_t N) const {
  auto Kids = childrenOf(I);
  assert(N < Kids.size() && "child index out of bounds");
  return Kids[N];
}

uint32_t NodePool::makePayload(InternedStr Name, Regime R) {
  assert(Name <= 0x3FFFFFFFu &&
         "StringInterner ID exceeded 30-bit payload capacity");
  return (static_cast<uint32_t>(R) << 30) | (Name & 0x3FFFFFFFu);
}

InternedStr NodePool::payloadStr(uint32_t P) { return P & 0x3FFFFFFFu; }

Regime NodePool::payloadRegime(uint32_t P) {
  return static_cast<Regime>(P >> 30);
}

uint32_t NodePool::makeOpPayload(uint16_t TokenKindValue) {
  return static_cast<uint32_t>(TokenKindValue);
}

uint16_t NodePool::payloadOp(uint32_t P) {
  return static_cast<uint16_t>(P & 0xFFFFu);
}

} // namespace eter::parser
