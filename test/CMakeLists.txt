#===----------------------------------------------------------------------===#
#
# Part of the Eter Project, under the Apache License v2.0 with LLVM Exceptions.
# See /LICENSE for license information.
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
#
#===----------------------------------------------------------------------===#

find_program(ETER_LIT_EXECUTABLE
  NAMES llvm-lit lit
  HINTS "${LLVM_TOOLS_BINARY_DIR}"
  DOC "Path to the lit test runner"
)

find_program(FILECHECK_EXECUTABLE
  NAMES FileCheck filecheck
  HINTS "${LLVM_TOOLS_BINARY_DIR}"
  DOC "Path to the FileCheck utility"
)

if(NOT ETER_LIT_EXECUTABLE)
  message(FATAL_ERROR "lit was not found. Install lit or add it to PATH.")
endif()

if(NOT FILECHECK_EXECUTABLE)
  message(FATAL_ERROR "FileCheck was not found. Install LLVM tools or add them to PATH.")
endif()

message(STATUS "Found lit: ${ETER_LIT_EXECUTABLE}")
message(STATUS "Found FileCheck: ${FILECHECK_EXECUTABLE}")

configure_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/lit.site.cfg.py.in
  ${CMAKE_CURRENT_BINARY_DIR}/lit.site.cfg.py
  @ONLY
)

add_custom_target(check-eter
  COMMAND ${ETER_LIT_EXECUTABLE} -sv ${CMAKE_CURRENT_BINARY_DIR}
  DEPENDS eter
  COMMENT "Running Eter lit test suite"
)

