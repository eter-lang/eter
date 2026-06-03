#===----------------------------------------------------------------------===#
#
# Part of the Eter Project, under the Apache License v2.0 with LLVM Exceptions.
# See /LICENSE for license information.
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
#
#===----------------------------------------------------------------------===#

# Add the Lexer unit tests
add_unittest(EterUnitTests ParserTestConstDecl TestParserConstDecl.cpp)

# Link against Eter libraries
target_link_libraries(ParserTestConstDecl PRIVATE
  EterBase
  EterParser
)

target_include_directories(ParserTestConstDecl PRIVATE
  ${CMAKE_SOURCE_DIR}/include
)

target_include_directories(ParserTestConstDecl SYSTEM PRIVATE
  ${LLVM_INCLUDE_DIRS}
  ${MLIR_INCLUDE_DIRS}
)

add_unittest(EterUnitTests ParserTestFnDecl TestParserFnDecl.cpp)

target_link_libraries(ParserTestFnDecl PRIVATE
  EterBase
  EterParser
)

target_include_directories(ParserTestFnDecl PRIVATE
  ${CMAKE_SOURCE_DIR}/include
)

target_include_directories(ParserTestFnDecl SYSTEM PRIVATE
  ${LLVM_INCLUDE_DIRS}
  ${MLIR_INCLUDE_DIRS}
)

add_unittest(EterUnitTests ParserTestDiagnostics TestParserDiagnostics.cpp)

target_link_libraries(ParserTestDiagnostics PRIVATE
add_unittest(EterUnitTests ParserTestExpr ParseExprTest.cpp)

target_link_libraries(ParserTestExpr PRIVATE
  EterBase
  EterParser
)

target_include_directories(ParserTestDiagnostics PRIVATE
  ${CMAKE_SOURCE_DIR}/include
)

target_include_directories(ParserTestDiagnostics SYSTEM PRIVATE
  ${LLVM_INCLUDE_DIRS}
  ${MLIR_INCLUDE_DIRS}
)

add_unittest(EterUnitTests ParserTestDocComments TestParserDocComments.cpp)

target_link_libraries(ParserTestDocComments PRIVATE
  EterBase
  EterParser
)

target_include_directories(ParserTestDocComments PRIVATE
  ${CMAKE_SOURCE_DIR}/include
)

target_include_directories(ParserTestDocComments SYSTEM PRIVATE
  ${LLVM_INCLUDE_DIRS}
  ${MLIR_INCLUDE_DIRS}
)
target_include_directories(ParserTestExpr PRIVATE
  ${CMAKE_SOURCE_DIR}/include
)

target_include_directories(ParserTestExpr SYSTEM PRIVATE
  ${LLVM_INCLUDE_DIRS}
  ${MLIR_INCLUDE_DIRS}
)
