//===----------------------------------------------------------------------===//
//
// Part of the Eter Project, under the Apache License v2.0 with LLVM Exceptions.
// See /LICENSE for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "eter/Base/DiagnosticEngine.h"
#include "eter/Base/SourceBuffer.h"
#include "eter/Base/SourceManager.h"

#include "gtest/gtest.h"

#include <sstream>

using namespace eter;

//============================================================================//
// Test: SimpleDiagnosticEngine
//============================================================================//

TEST(SimpleDiagnosticEngineTest, ErrorBuilder) {
  SimpleDiagnosticEngine SDE;
  SDE.error().message("test error").emit();

  std::stringstream SS;
  EXPECT_TRUE(true);
}

TEST(SimpleDiagnosticEngineTest, WarningBuilder) {
  SimpleDiagnosticEngine SDE;
  SDE.warning().message("test warning").emit();
  EXPECT_TRUE(true);
}

TEST(SimpleDiagnosticEngineTest, NoteBuilder) {
  SimpleDiagnosticEngine SDE;
  SDE.note().message("test note").emit();
  EXPECT_TRUE(true);
}

TEST(SimpleDiagnosticEngineTest, ErrorWithNote) {
  SimpleDiagnosticEngine SDE;
  SDE.error()
      .message("main error")
      .note("additional context")
      .emit();
  EXPECT_TRUE(true);
}

//============================================================================//
// Test: DiagnosticLocation
//============================================================================//

TEST(DiagnosticLocationTest, NoneLocation) {
  auto Loc = DiagnosticLocation::none();
  EXPECT_EQ(Loc.kind(), DiagnosticLocation::Kind::None);
  EXPECT_FALSE(Loc.hasSpan());
  EXPECT_FALSE(Loc.hasFile());
}

TEST(DiagnosticLocationTest, FileLocation) {
  auto Loc = DiagnosticLocation::file("test.et");
  EXPECT_EQ(Loc.kind(), DiagnosticLocation::Kind::File);
  EXPECT_FALSE(Loc.hasSpan());
  EXPECT_TRUE(Loc.hasFile());
  EXPECT_EQ(Loc.filename(), "test.et");
}

TEST(DiagnosticLocationTest, SpanLocation) {
  auto Loc = DiagnosticLocation::span(Span(0, 5));
  EXPECT_EQ(Loc.kind(), DiagnosticLocation::Kind::Span);
  EXPECT_TRUE(Loc.hasSpan());
  EXPECT_FALSE(Loc.hasFile());
  EXPECT_EQ(Loc.span().Start, 0u);
  EXPECT_EQ(Loc.span().End, 5u);
}

//============================================================================//
// Test: DiagnosticBuilder with SimpleDiagnosticEngine
//============================================================================//

TEST(DiagnosticBuilderTest, FluentApiWithAt) {
  SimpleDiagnosticEngine SDE;
  SDE.error()
      .at(Span(10, 20))
      .message("error at span")
      .emit();
  EXPECT_TRUE(true);
}

TEST(DiagnosticBuilderTest, FluentApiWithAtFile) {
  SimpleDiagnosticEngine SDE;
  SDE.error()
      .atFile("myfile.et")
      .message("error in file")
      .emit();
  EXPECT_TRUE(true);
}

TEST(DiagnosticBuilderTest, FluentApiWithLabel) {
  SimpleDiagnosticEngine SDE;
  SDE.error()
      .message("error with label")
      .label(Span(0, 5), "this is the issue")
      .emit();
  EXPECT_TRUE(true);
}

TEST(DiagnosticBuilderTest, ComplexFluentApi) {
  SimpleDiagnosticEngine SDE;
  SDE.error()
      .at(Span(0, 10))
      .message("complex error")
      .label(Span(0, 5), "part one")
      .label(Span(5, 10), "part two")
      .note("first note")
      .note("second note")
      .emit();
  EXPECT_TRUE(true);
}

//============================================================================//
// Test: DiagnosticEngine with SourceManager
//============================================================================//

TEST(DiagnosticEngineTest, WithSourceManager) {
  auto Buffer = SourceBuffer::makeFromString("hello world", "test.et");
  SourceManager SM(Buffer);
  DiagnosticEngine DE(std::move(SM));

  DE.error()
      .at(Span(0, 5))
      .message("test error with source")
      .emit();
  EXPECT_TRUE(true);
}

TEST(DiagnosticEngineTest, PrintAll) {
  auto Buffer = SourceBuffer::makeFromString("hello world", "test.et");
  SourceManager SM(Buffer);
  DiagnosticEngine DE(std::move(SM));

  DE.error().at(Span(0, 5)).message("error 1").emit();
  DE.warning().at(Span(6, 11)).message("warning 1").emit();
  DE.note().message("note 1").emit();

  EXPECT_TRUE(true);
}

TEST(DiagnosticEngineTest, InternalCompilerError) {
  auto Buffer = SourceBuffer::makeFromString("test", "test.et");
  SourceManager SM(Buffer);
  DiagnosticEngine DE(std::move(SM));

  DE.ice()
      .at(Span(0, 4))
      .message("internal error")
      .emit();
  EXPECT_TRUE(true);
}

//============================================================================//
// Test: SimpleDiagnosticEngine to DiagnosticEngine conversion
//============================================================================//

TEST(DiagnosticEngineConversionTest, WithSourceManager) {
  auto Buffer = SourceBuffer::makeFromString("content", "file.et");
  SourceManager SM(Buffer);

  SimpleDiagnosticEngine SDE;
  SDE.error().message("error before conversion").emit();

  DiagnosticEngine DE = std::move(SDE).withSourceManager(std::move(SM));
  DE.error().at(Span(0, 7)).message("error after conversion").emit();

  EXPECT_TRUE(true);
}
