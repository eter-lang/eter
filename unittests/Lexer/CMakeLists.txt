#===----------------------------------------------------------------------===#
#
# Part of the Eter Project, under the Apache License v2.0 with LLVM Exceptions.
# See /LICENSE for license information.
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
#
#===----------------------------------------------------------------------===#

# Add the Lexer unit tests
add_unittest(EterUnitTests LexerTest LexerTest.cpp)

# Link against Eter libraries
target_link_libraries(LexerTest PRIVATE
  EterBase
  EterLexer
)

# Add include directories for the test - mark LLVM as system to suppress warnings
target_include_directories(LexerTest PRIVATE
  ${CMAKE_SOURCE_DIR}/include
)

target_include_directories(LexerTest SYSTEM PRIVATE
  ${LLVM_INCLUDE_DIRS}
  ${MLIR_INCLUDE_DIRS}
)

# Disable -Werror for tests to avoid LLVM header warnings
set_property(TARGET LexerTest APPEND PROPERTY COMPILE_OPTIONS -Wno-error -Wno-unused-parameter)
