#===----------------------------------------------------------------------===#
#
# Part of the Eter Project, under the Apache License v2.0 with LLVM Exceptions.
# See /LICENSE for license information.
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
#
#===----------------------------------------------------------------------===#

# Add the Base library unit tests (LLVM style: one executable, multiple .cpp files)
add_unittest(EterUnitTests EterBaseTests
  SpanTest.cpp
  SourceBufferTest.cpp
  SourceManagerTest.cpp
  DiagnosticEngineTest.cpp
)

target_link_libraries(EterBaseTests PRIVATE EterBase)

set_target_properties(EterBaseTests
  PROPERTIES FOLDER "Eter/Tests/Base"
)

target_include_directories(EterBaseTests PRIVATE ${CMAKE_SOURCE_DIR}/include)
target_include_directories(EterBaseTests SYSTEM PRIVATE ${LLVM_INCLUDE_DIRS} ${MLIR_INCLUDE_DIRS})
