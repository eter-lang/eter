//===----------------------------------------------------------------------===//
//
// Part of the Eter Project, under the Apache License v2.0 with LLVM Exceptions.
// See /LICENSE for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "eter/Lexer/Token.h"

namespace eter::lexer {

const char *Token::getTokenName(Kind K) {
  switch (K) {
#define TOK(X)                                                                 \
  case Kind::X:                                                                \
    return #X;
#define PUNCTUATOR(X, Y)                                                       \
  case Kind::X:                                                                \
    return Y;
#define KEYWORD(X, Y)                                                          \
  case Kind::kw_##X:                                                           \
    return Y;
#include "eter/Lexer/TokenKinds.def"
#undef KEYWORD
#undef PUNCTUATOR
#undef TOK
  }
  return "unknown";
}
} // namespace eter::lexer
