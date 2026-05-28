//===----------------------------------------------------------------------===//
//
// Part of the Eter Project, under the Apache License v2.0 with LLVM Exceptions.
// See /LICENSE for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "eter/Parser/NodePool.h"
#include "eter/Parser/Parser.h"
#include "eter/Parser/Regime.h"

#include <llvm/Support/VirtualFileSystem.h>

#include "TestParserHelper.h"
#include "gtest/gtest.h"

using namespace eter;
using namespace eter::parser;
using namespace eter::lexer;
using namespace ParserTestHelper;

TEST(ParserTestDecl, FnDeclEmptyMain) {
  parseSource("fn main() {}");

  EXPECT_TRUE(PR.ok());
  EXPECT_TRUE(checkChildrenKinds(PR.Root, NodeKind::FnDecl));

  const NodeIndex Fn = PR.Pool.childrenOf(PR.Root)[0];
  checkInternedString(Fn, "main");
  checkRegime(Fn, Regime::None);
  EXPECT_TRUE(checkChildrenKinds(Fn, NodeKind::ParamList, NodeKind::BlockExpr));

  const NodeIndex Params = PR.Pool.childrenOf(Fn)[0];
  EXPECT_EQ(PR.Pool.childrenOf(Params).size(), 0u);

  const NodeIndex Body = PR.Pool.childrenOf(Fn)[1];
  EXPECT_EQ(PR.Pool.childrenOf(Body).size(), 0u);
}

TEST(ParserTestDecl, FnDeclWithParamsAndReturnType) {
  parseSource("fn add(x: i32, y: i32): i32 {}");

  EXPECT_TRUE(PR.ok());
  EXPECT_TRUE(checkChildrenKinds(PR.Root, NodeKind::FnDecl));

  const NodeIndex Fn = PR.Pool.childrenOf(PR.Root)[0];
  checkInternedString(Fn, "add");
  checkRegime(Fn, Regime::None);
  EXPECT_TRUE(checkChildrenKinds(Fn, NodeKind::ParamList, NodeKind::NamedType,
                                 NodeKind::BlockExpr));

  const NodeIndex Params = PR.Pool.childrenOf(Fn)[0];
  EXPECT_TRUE(checkChildrenKinds(Params, NodeKind::Param, NodeKind::Param));

  const NodeIndex P0 = PR.Pool.childrenOf(Params)[0];
  checkInternedString(P0, "x");
  checkRegime(P0, Regime::None);
  EXPECT_TRUE(checkChildrenKinds(P0, NodeKind::NamedType));
  checkInternedString(PR.Pool.childrenOf(P0)[0], "i32");

  const NodeIndex P1 = PR.Pool.childrenOf(Params)[1];
  checkInternedString(P1, "y");
  checkRegime(P1, Regime::None);
  EXPECT_TRUE(checkChildrenKinds(P1, NodeKind::NamedType));
  checkInternedString(PR.Pool.childrenOf(P1)[0], "i32");

  const NodeIndex Ret = PR.Pool.childrenOf(Fn)[1];
  checkInternedString(Ret, "i32");
}

TEST(ParserTestDecl, FnDeclMutParam) {
  parseSource("fn store(mut buf: i32) {}");

  EXPECT_TRUE(PR.ok());
  const NodeIndex Fn = PR.Pool.childrenOf(PR.Root)[0];
  checkInternedString(Fn, "store");
  EXPECT_TRUE(checkChildrenKinds(Fn, NodeKind::ParamList, NodeKind::BlockExpr));

  const NodeIndex Params = PR.Pool.childrenOf(Fn)[0];
  EXPECT_TRUE(checkChildrenKinds(Params, NodeKind::Param));

  const NodeIndex P = PR.Pool.childrenOf(Params)[0];
  checkInternedString(P, "buf");
  checkRegime(P, Regime::Mut);
  checkInternedString(PR.Pool.childrenOf(P)[0], "i32");
}

TEST(ParserTestDecl, FnDeclProjParamWithReturnType) {
  parseSource("fn read(proj t: i32): bool {}");

  EXPECT_TRUE(PR.ok());
  const NodeIndex Fn = PR.Pool.childrenOf(PR.Root)[0];
  checkInternedString(Fn, "read");
  EXPECT_TRUE(checkChildrenKinds(Fn, NodeKind::ParamList, NodeKind::NamedType,
                                 NodeKind::BlockExpr));

  const NodeIndex Params = PR.Pool.childrenOf(Fn)[0];
  const NodeIndex P = PR.Pool.childrenOf(Params)[0];
  checkInternedString(P, "t");
  checkRegime(P, Regime::Proj);
  checkInternedString(PR.Pool.childrenOf(P)[0], "i32");

  checkInternedString(PR.Pool.childrenOf(Fn)[1], "bool");
}

TEST(ParserTestDecl, FnDeclMutReturnRegime) {
  // The regime precedes the function name and is stored in FnDecl's payload as
  // the return regime (see NodeKinds.def: "fn [regime] name(...)").
  parseSource("fn mut make(): i32 {}");

  EXPECT_TRUE(PR.ok());
  const NodeIndex Fn = PR.Pool.childrenOf(PR.Root)[0];
  checkInternedString(Fn, "make");
  checkRegime(Fn, Regime::Mut);
  EXPECT_TRUE(checkChildrenKinds(Fn, NodeKind::ParamList, NodeKind::NamedType,
                                 NodeKind::BlockExpr));

  checkInternedString(PR.Pool.childrenOf(Fn)[1], "i32");
}

TEST(ParserTestDecl, TopLevelDispatchFnAndConst) {
  parseSource("fn a() {} const b: i32 = 1;");

  EXPECT_TRUE(PR.ok());
  EXPECT_TRUE(
      checkChildrenKinds(PR.Root, NodeKind::FnDecl, NodeKind::ConstDecl));

  const NodeIndex Fn = PR.Pool.childrenOf(PR.Root)[0];
  checkInternedString(Fn, "a");

  const NodeIndex Const = PR.Pool.childrenOf(PR.Root)[1];
  checkInternedString(Const, "b");
}
