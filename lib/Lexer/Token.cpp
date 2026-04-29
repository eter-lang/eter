//===----------------------------------------------------------------------===//
//
// Part of the Eter Project, under the Apache License v2.0 with LLVM Exceptions.
// See /LICENSE for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "eter/Lexer/Token.h"

namespace eter::lexer {

const llvm::StringRef Token::getTokenName(Kind K) {
  switch (K) {
#define ETER_TOKEN(X)                                                          \
  case Kind::X:                                                                \
    return #X;
#define ETER_SYMBOL(X, Y)                                                      \
  case Kind::X:                                                                \
    return Y;
#define ETER_KEYWORD(X, Y)                                                     \
  case Kind::kw_##X:                                                           \
    return Y;
#include "eter/Lexer/TokenKinds.def"
#undef ETER_KEYWORD
#undef ETER_SYMBOL
#undef ETER_TOKEN
  }
  return "unknown";
}
} // namespace eter::lexer
