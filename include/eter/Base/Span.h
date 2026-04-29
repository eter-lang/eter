//===----------------------------------------------------------------------===//
//
// Part of the Eter Project, under the Apache License v2.0 with LLVM Exceptions.
// See /LICENSE for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef ETER_BASE_SPAN_H
#define ETER_BASE_SPAN_H

#include <cstdint>
namespace eter {
/// A source range identified by byte offsets into a source buffer.
struct Span {
  uint32_t Start = 0;
  uint32_t End = 0;

  Span() = default;
  Span(uint32_t Start, uint32_t End) : Start(Start), End(End) {}
};

} // namespace eter

#endif // ETER_BASE_SPAN_H
