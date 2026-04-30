//===----------------------------------------------------------------------===//
//
// Part of the Eter Project, under the Apache License v2.0 with LLVM Exceptions.
// See /LICENSE for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "eter/Base/SourceBuffer.h"
#include "eter/Base/DiagnosticEngine.h"

#include <llvm/Support/VirtualFileSystem.h>

#include "gtest/gtest.h"

using namespace eter;

TEST(SourceBufferTest, MakeFromString) {
  auto Buffer = SourceBuffer::makeFromString("hello world");
  EXPECT_EQ(Buffer.getBuffer(), "hello world");
  EXPECT_EQ(Buffer.getFilename(), "buffer");
}

TEST(SourceBufferTest, MakeFromStringWithCustomName) {
  auto Buffer = SourceBuffer::makeFromString("content", "my-file.et");
  EXPECT_EQ(Buffer.getBuffer(), "content");
  EXPECT_EQ(Buffer.getFilename(), "my-file.et");
}

TEST(SourceBufferTest, MakeFromStringEmpty) {
  auto Buffer = SourceBuffer::makeFromString("");
  EXPECT_EQ(Buffer.getBuffer(), "");
  EXPECT_EQ(Buffer.getFilename(), "buffer");
}

TEST(SourceBufferTest, MakeFromStringUnicode) {
  auto Buffer = SourceBuffer::makeFromString("Hello 世界 🌍");
  EXPECT_EQ(Buffer.getBuffer(), "Hello 世界 🌍");
}

TEST(SourceBufferTest, MakeFromFileNameNonexistent) {
  llvm::vfs::InMemoryFileSystem FS;
  SimpleDiagnosticEngine SDE;
  auto BufferOrErr = SourceBuffer::makeFromFileName(FS, "nonexistent.et", SDE);
  EXPECT_FALSE(!!BufferOrErr);
}

TEST(SourceBufferTest, MakeFromFileNameSuccess) {
  llvm::vfs::InMemoryFileSystem FS;
  const char *TestContent = "test content";
  FS.addFile("test.et", 0, llvm::MemoryBuffer::getMemBuffer(TestContent));

  SimpleDiagnosticEngine SDE;
  auto BufferOrErr = SourceBuffer::makeFromFileName(FS, "test.et", SDE);
  ASSERT_TRUE(!!BufferOrErr);
  EXPECT_EQ(BufferOrErr->getBuffer(), "test content");
  EXPECT_EQ(BufferOrErr->getFilename(), "test.et");
}
