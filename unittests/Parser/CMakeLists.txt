#===----------------------------------------------------------------------===#
#
# Part of the Eter Project, under the Apache License v2.0 with LLVM Exceptions.
# See /LICENSE for license information.
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
#
#===----------------------------------------------------------------------===#

# Add the Lexer unit tests
add_unittest(EterUnitTests ParserTest ASTNodesTest.cpp)

# Link against Eter libraries
target_link_libraries(ParserTest PRIVATE
  EterBase
  EterParser
)

target_include_directories(ParserTest PRIVATE
  ${CMAKE_SOURCE_DIR}/include
)

target_include_directories(ParserTest SYSTEM PRIVATE
  ${LLVM_INCLUDE_DIRS}
  ${MLIR_INCLUDE_DIRS}
)
