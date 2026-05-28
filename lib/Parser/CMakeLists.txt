#===----------------------------------------------------------------------===#
#
# Part of the Eter Project, under the Apache License v2.0 with LLVM Exceptions.
# See /LICENSE for license information.
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
#
#===----------------------------------------------------------------------===#

add_library(EterParser
    Parser.cpp
    ParseConst.cpp
    ParseDecl.cpp
    ParseStmt.cpp
    ParseExpr.cpp
    ParseType.cpp
    ParsePat.cpp
    NodePool.cpp
    TokenStream.cpp
)

add_library(eter::Parser ALIAS EterParser)

target_link_libraries(EterParser
  PUBLIC
    EterProjectOptions
    EterBase
    EterLexer
    LLVM
)

target_include_directories(EterParser
  PUBLIC
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

target_include_directories(EterParser SYSTEM
  PRIVATE
    ${LLVM_INCLUDE_DIRS}
)
