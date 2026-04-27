#===----------------------------------------------------------------------===#
#
# Part of the Eter Project, under the Apache License v2.0 with LLVM Exceptions.
# See /LICENSE for license information.
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
#
#===----------------------------------------------------------------------===#

add_library(EterLexer
    Token.cpp
    Lexer.cpp
)

add_library(eter::Lexer ALIAS EterLexer)

target_link_libraries(EterLexer
  PUBLIC
    EterProjectOptions
    LLVM
)

target_include_directories(EterLexer
  PUBLIC
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

target_include_directories(EterLexer SYSTEM
  PRIVATE
    ${LLVM_INCLUDE_DIRS}
)
