#===----------------------------------------------------------------------===#
#
# Part of the Eter Project, under the Apache License v2.0 with LLVM Exceptions.
# See /LICENSE for license information.
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
#
#===----------------------------------------------------------------------===#

set(LLVM_TOOL_BIN_DIR "")
if(DEFINED LLVM_TOOLS_BINARY_DIR)
  list(APPEND LLVM_TOOL_BIN_DIR "${LLVM_TOOLS_BINARY_DIR}")
endif()

if(DEFINED LLVM_DIR)
  get_filename_component(LLVM_DIR_PARENT "${LLVM_DIR}" PATH)
  get_filename_component(LLVM_DIR_PARENT "${LLVM_DIR_PARENT}" PATH)
  get_filename_component(LLVM_ROOT_DIR "${LLVM_DIR_PARENT}" PATH)
  list(APPEND LLVM_TOOL_BIN_DIR "${LLVM_ROOT_DIR}/bin")
endif()

if(DEFINED CMAKE_PREFIX_PATH)
  foreach(_prefix IN LISTS CMAKE_PREFIX_PATH)
    list(APPEND LLVM_TOOL_BIN_DIR "${_prefix}/bin")
  endforeach()
endif()

message(STATUS "LLVM_DIR=${LLVM_DIR}")
message(STATUS "LLVM_TOOLS_BINARY_DIR=${LLVM_TOOLS_BINARY_DIR}")
message(STATUS "LLVM_TOOL_BIN_DIR=${LLVM_TOOL_BIN_DIR}")
message(STATUS "CMAKE_PREFIX_PATH=${CMAKE_PREFIX_PATH}")

if(NOT DEFINED ETER_LIT_EXECUTABLE)
  find_program(ETER_LIT_EXECUTABLE
    NAMES llvm-lit llvm-lit-22 lit
    HINTS ${LLVM_TOOL_BIN_DIR}
    PATHS ${LLVM_TOOL_BIN_DIR} ENV PATH
    DOC "Path to the lit test runner"
  )
endif()

if(NOT DEFINED FILECHECK_EXECUTABLE)
  find_program(FILECHECK_EXECUTABLE
    NAMES FileCheck FileCheck-22 filecheck filecheck-22 llvm-FileCheck llvm-filecheck llvm-FileCheck-22 llvm-filecheck-22
    HINTS ${LLVM_TOOL_BIN_DIR}
    PATHS ${LLVM_TOOL_BIN_DIR} ENV PATH
    DOC "Path to the FileCheck utility"
  )
endif()

if(NOT FILECHECK_EXECUTABLE)
  set(_FILECHECK_CANDIDATE_NAMES
    FileCheck
    FileCheck-22
    filecheck
    filecheck-22
    llvm-FileCheck
    llvm-filecheck
    llvm-FileCheck-22
    llvm-filecheck-22
  )

  foreach(_candidate_dir IN LISTS LLVM_TOOL_BIN_DIR)
    foreach(_candidate_name IN LISTS _FILECHECK_CANDIDATE_NAMES)
      set(_candidate_path "${_candidate_dir}/${_candidate_name}")
      if(EXISTS "${_candidate_path}" AND NOT IS_DIRECTORY "${_candidate_path}")
        set(FILECHECK_EXECUTABLE "${_candidate_path}")
        break()
      endif()
    endforeach()
    if(FILECHECK_EXECUTABLE)
      break()
    endif()
  endforeach()

  if(NOT FILECHECK_EXECUTABLE AND DEFINED LLVM_ROOT_DIR)
    file(GLOB_RECURSE _FILECHECK_FALLBACK_CANDIDATES
      LIST_DIRECTORIES FALSE
      "${LLVM_ROOT_DIR}/*FileCheck*"
      "${LLVM_ROOT_DIR}/*filecheck*"
    )

    foreach(_candidate_path IN LISTS _FILECHECK_FALLBACK_CANDIDATES)
      get_filename_component(_candidate_name "${_candidate_path}" NAME)
      if(_candidate_name IN_LIST _FILECHECK_CANDIDATE_NAMES)
        set(FILECHECK_EXECUTABLE "${_candidate_path}")
        break()
      endif()
    endforeach()
  endif()
endif()

if(NOT ETER_LIT_EXECUTABLE)
  message(FATAL_ERROR "lit was not found. Install lit or add it to PATH.")
endif()

if(NOT FILECHECK_EXECUTABLE)
  message(FATAL_ERROR
    "FileCheck was not found. Tried names: FileCheck, FileCheck-22, filecheck, filecheck-22, llvm-FileCheck, llvm-filecheck. "
    "Searched directories: ${LLVM_TOOL_BIN_DIR}. "
    "LLVM_ROOT_DIR: ${LLVM_ROOT_DIR}. "
    "Install LLVM tools package for your distro or pass -DLLVM_TOOLS_BINARY_DIR=<dir-containing-FileCheck>."
  )
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
  DEPENDS eterc
  COMMENT "Running Eter lit test suite"
)

