#===----------------------------------------------------------------------===#
#
# Part of the Eter Project, under the Apache License v2.0 with LLVM Exceptions.
# See /LICENSE for license information.
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
#
#===----------------------------------------------------------------------===#

add_unittest(EterUnitTests ParserTestConstDecl TestParserConstDecl.cpp)

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

add_unittest(EterUnitTests ParserTestStructDecl TestParserStructDecl.cpp)

target_link_libraries(ParserTestStructDecl PRIVATE
  EterBase
  EterParser
)

target_include_directories(ParserTestStructDecl PRIVATE
  ${CMAKE_SOURCE_DIR}/include
)

target_include_directories(ParserTestStructDecl SYSTEM PRIVATE
  ${LLVM_INCLUDE_DIRS}
  ${MLIR_INCLUDE_DIRS}
)

add_unittest(EterUnitTests ParserTestEnumDecl TestParserEnumDecl.cpp)

target_link_libraries(ParserTestEnumDecl PRIVATE
  EterBase
  EterParser
)

target_include_directories(ParserTestEnumDecl PRIVATE
  ${CMAKE_SOURCE_DIR}/include
)

target_include_directories(ParserTestEnumDecl SYSTEM PRIVATE
  ${LLVM_INCLUDE_DIRS}
  ${MLIR_INCLUDE_DIRS}
)

add_unittest(EterUnitTests ParserTestUnionDecl TestParserUnionDecl.cpp)

target_link_libraries(ParserTestUnionDecl PRIVATE
  EterBase
  EterParser
)

target_include_directories(ParserTestUnionDecl PRIVATE
  ${CMAKE_SOURCE_DIR}/include
)

target_include_directories(ParserTestUnionDecl SYSTEM PRIVATE
  ${LLVM_INCLUDE_DIRS}
  ${MLIR_INCLUDE_DIRS}
)

add_unittest(EterUnitTests ParserTestDiagnostics TestParserDiagnostics.cpp)

target_link_libraries(ParserTestDiagnostics PRIVATE
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

add_unittest(EterUnitTests ParserTestExpr ParseExprTest.cpp)

target_link_libraries(ParserTestExpr PRIVATE
  EterBase
  EterParser
)

target_include_directories(ParserTestExpr PRIVATE
  ${CMAKE_SOURCE_DIR}/include
)

target_include_directories(ParserTestExpr SYSTEM PRIVATE
  ${LLVM_INCLUDE_DIRS}
  ${MLIR_INCLUDE_DIRS}
)

add_unittest(EterUnitTests ParserTestStmt ParseStatementTest.cpp)

target_link_libraries(ParserTestStmt PRIVATE
  EterBase
  EterParser
)

target_include_directories(ParserTestStmt PRIVATE
  ${CMAKE_SOURCE_DIR}/include
)

target_include_directories(ParserTestStmt SYSTEM PRIVATE
  ${LLVM_INCLUDE_DIRS}
  ${MLIR_INCLUDE_DIRS}
)

add_unittest(EterUnitTests ParserTestUseDecl TestParserUseDecl.cpp)

target_link_libraries(ParserTestUseDecl PRIVATE
  EterBase
  EterParser
)

target_include_directories(ParserTestUseDecl PRIVATE
  ${CMAKE_SOURCE_DIR}/include
)

target_include_directories(ParserTestUseDecl SYSTEM PRIVATE
  ${LLVM_INCLUDE_DIRS}
  ${MLIR_INCLUDE_DIRS}
)

add_unittest(EterUnitTests ParserTestTraitDecl TestParserTraitDecl.cpp)

target_link_libraries(ParserTestTraitDecl PRIVATE
  EterBase
  EterParser
)

target_include_directories(ParserTestTraitDecl PRIVATE
  ${CMAKE_SOURCE_DIR}/include
)

target_include_directories(ParserTestTraitDecl SYSTEM PRIVATE
  ${LLVM_INCLUDE_DIRS}
  ${MLIR_INCLUDE_DIRS}
)

add_unittest(EterUnitTests ParserTestImplDecl TestParserImplDecl.cpp)

target_link_libraries(ParserTestImplDecl PRIVATE
  EterBase
  EterParser
)

target_include_directories(ParserTestImplDecl PRIVATE
  ${CMAKE_SOURCE_DIR}/include
)

target_include_directories(ParserTestImplDecl SYSTEM PRIVATE
  ${LLVM_INCLUDE_DIRS}
  ${MLIR_INCLUDE_DIRS}
)

add_unittest(EterUnitTests ParserTestForStmt TestParserForStmt.cpp)

target_link_libraries(ParserTestForStmt PRIVATE
  EterBase
  EterParser
)

target_include_directories(ParserTestForStmt PRIVATE
  ${CMAKE_SOURCE_DIR}/include
)

target_include_directories(ParserTestForStmt SYSTEM PRIVATE
  ${LLVM_INCLUDE_DIRS}
  ${MLIR_INCLUDE_DIRS}
)

add_unittest(EterUnitTests ParserTestUnsafe TestParserUnsafe.cpp)

target_link_libraries(ParserTestUnsafe PRIVATE
  EterBase
  EterParser
)

target_include_directories(ParserTestUnsafe PRIVATE
  ${CMAKE_SOURCE_DIR}/include
)

target_include_directories(ParserTestUnsafe SYSTEM PRIVATE
  ${LLVM_INCLUDE_DIRS}
  ${MLIR_INCLUDE_DIRS}
)

add_unittest(EterUnitTests ParserTestBreakContinue TestParserBreakContinue.cpp)

target_link_libraries(ParserTestBreakContinue PRIVATE
  EterBase
  EterParser
)

target_include_directories(ParserTestBreakContinue PRIVATE
  ${CMAKE_SOURCE_DIR}/include
)

target_include_directories(ParserTestBreakContinue SYSTEM PRIVATE
  ${LLVM_INCLUDE_DIRS}
  ${MLIR_INCLUDE_DIRS}
)

add_unittest(EterUnitTests ParserTestCastExpr TestParserCastExpr.cpp)

target_link_libraries(ParserTestCastExpr PRIVATE
  EterBase
  EterParser
)

target_include_directories(ParserTestCastExpr PRIVATE
  ${CMAKE_SOURCE_DIR}/include
)

target_include_directories(ParserTestCastExpr SYSTEM PRIVATE
  ${LLVM_INCLUDE_DIRS}
  ${MLIR_INCLUDE_DIRS}
)

add_unittest(EterUnitTests ParserTestProjAssign TestParserProjAssign.cpp)

target_link_libraries(ParserTestProjAssign PRIVATE
  EterBase
  EterParser
)

target_include_directories(ParserTestProjAssign PRIVATE
  ${CMAKE_SOURCE_DIR}/include
)

target_include_directories(ParserTestProjAssign SYSTEM PRIVATE
  ${LLVM_INCLUDE_DIRS}
  ${MLIR_INCLUDE_DIRS}
)
