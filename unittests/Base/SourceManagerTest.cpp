//===----------------------------------------------------------------------===//
//
// Part of the Eter Project, under the Apache License v2.0 with LLVM Exceptions.
// See /LICENSE for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "eter/Base/SourceBuffer.h"
#include "eter/Base/SourceManager.h"

#include "gtest/gtest.h"

using namespace eter;

TEST(SourceManagerTest, GetLocationSingleLine) {
  auto Buffer = SourceBuffer::makeFromString("hello");
  SourceManager SM(Buffer);

  auto Loc = SM.getLocation(0);
  EXPECT_EQ(Loc.Line, 1u);
  EXPECT_EQ(Loc.Column, 1u);

  Loc = SM.getLocation(4);
  EXPECT_EQ(Loc.Line, 1u);
  EXPECT_EQ(Loc.Column, 5u);
}

TEST(SourceManagerTest, GetLocationMultipleLines) {
  auto Buffer = SourceBuffer::makeFromString("hello\nworld");
  SourceManager SM(Buffer);

  auto Loc = SM.getLocation(0);
  EXPECT_EQ(Loc.Line, 1u);
  EXPECT_EQ(Loc.Column, 1u);

  Loc = SM.getLocation(6);
  EXPECT_EQ(Loc.Line, 2u);
  EXPECT_EQ(Loc.Column, 1u);
}

TEST(SourceManagerTest, GetLocationAfterNewline) {
  auto Buffer = SourceBuffer::makeFromString("line1\nline2\nline3");
  SourceManager SM(Buffer);

  auto Loc = SM.getLocation(5);
  EXPECT_EQ(Loc.Line, 1u);
  EXPECT_EQ(Loc.Column, 6u);

  Loc = SM.getLocation(6);
  EXPECT_EQ(Loc.Line, 2u);
  EXPECT_EQ(Loc.Column, 1u);
}

TEST(SourceManagerTest, GetLocationEmptyBuffer) {
  auto Buffer = SourceBuffer::makeFromString("");
  SourceManager SM(Buffer);

  auto Loc = SM.getLocation(0);
  EXPECT_EQ(Loc.Line, 1u);
  EXPECT_EQ(Loc.Column, 1u);
}

TEST(SourceManagerTest, Slice) {
  auto Buffer = SourceBuffer::makeFromString("hello world");
  SourceManager SM(Buffer);

  auto Slice = SM.slice(Span(0, 5));
  EXPECT_EQ(Slice, "hello");

  Slice = SM.slice(Span(6, 11));
  EXPECT_EQ(Slice, "world");
}

TEST(SourceManagerTest, SliceEmpty) {
  auto Buffer = SourceBuffer::makeFromString("hello");
  SourceManager SM(Buffer);

  auto Slice = SM.slice(Span(2, 2));
  EXPECT_EQ(Slice, "");
}

TEST(SourceManagerTest, GetBuffer) {
  auto Buffer = SourceBuffer::makeFromString("test content");
  SourceManager SM(Buffer);

  EXPECT_EQ(SM.getBuffer(), "test content");
}

TEST(SourceManagerTest, GetFilename) {
  auto Buffer = SourceBuffer::makeFromString("content", "myfile.et");
  SourceManager SM(Buffer);

  EXPECT_EQ(SM.getFilename(), "myfile.et");
}

TEST(SourceManagerTest, LocationCaching) {
  auto Buffer = SourceBuffer::makeFromString("line1\nline2\nline3");
  SourceManager SM(Buffer);

  auto Loc1 = SM.getLocation(6);
  auto Loc2 = SM.getLocation(6);
  EXPECT_EQ(Loc1.Line, Loc2.Line);
  EXPECT_EQ(Loc1.Column, Loc2.Column);
}
