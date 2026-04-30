#===----------------------------------------------------------------------===#
#
# Part of the Eter Project, under the Apache License v2.0 with LLVM Exceptions.
# See /LICENSE for license information.
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
#
#===----------------------------------------------------------------------===#

cmake_minimum_required(VERSION 3.20)

project(eter VERSION 0.1.0 LANGUAGES C CXX)

if(NOT DEFINED CMAKE_COLOR_DIAGNOSTICS)
  set(CMAKE_COLOR_DIAGNOSTICS ON)
endif()

include(GNUInstallDirs)

option(ETER_ENABLE_WERROR "Treat warnings as errors" ON)
option(ETER_ENABLE_ASSERTIONS "Enable assertions in Eter" ON)

# Default: enable debug in Debug/RelWithDebInfo builds, but allow user override.
if(NOT DEFINED ETER_ENABLE_DEBUG)
  set(ETER_ENABLE_DEBUG_DEFAULT OFF)
  if(CMAKE_BUILD_TYPE STREQUAL "Debug" OR CMAKE_BUILD_TYPE STREQUAL "RelWithDebInfo")
    set(ETER_ENABLE_DEBUG_DEFAULT ON)
  endif()
  option(ETER_ENABLE_DEBUG "Enable debug output macros in Eter" ${ETER_ENABLE_DEBUG_DEFAULT})
endif()

option(ETER_INCLUDE_TESTS "Enable Eter's lit/FileCheck test suite" ON)
option(ETER_ENABLE_SANITIZERS "Enable sanitizer instrumentation for supported targets" OFF)
set(ETER_SANITIZERS "address;undefined;leak" CACHE STRING "Sanitizers to enable when ETER_ENABLE_SANITIZERS is ON. Supported values: address;undefined;leak;thread;memory")

# Option to run clang-tidy as part of the build. Disabled by default because
# running clang-tidy during every compile can significantly slow incremental builds.
option(ETER_ENABLE_CLANG_TIDY "Run clang-tidy during build (may slow builds)" OFF)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE Debug CACHE STRING "Build type" FORCE)
endif()

add_library(EterProjectOptions INTERFACE)
target_compile_features(EterProjectOptions INTERFACE cxx_std_17)

if(MSVC)
  target_compile_options(EterProjectOptions INTERFACE /W4)
  if(ETER_ENABLE_WERROR)
    target_compile_options(EterProjectOptions INTERFACE /WX)
  endif()
else()
  target_compile_options(EterProjectOptions INTERFACE -Wall -Wextra -Wpedantic)
  if(ETER_ENABLE_WERROR)
    target_compile_options(EterProjectOptions INTERFACE -Werror)
  endif()
endif()

if(ETER_ENABLE_ASSERTIONS)
  target_compile_definitions(EterProjectOptions INTERFACE ETER_ENABLE_ASSERTIONS=1)
endif()

if(ETER_ENABLE_DEBUG)
  target_compile_definitions(EterProjectOptions INTERFACE ETER_ENABLE_DEBUG=1)
endif()

if(ETER_ENABLE_SANITIZERS)
  string(REPLACE "," ";" ETER_SANITIZERS_LIST "${ETER_SANITIZERS}")
  set(ETER_SANITIZERS_VALID "address;undefined;leak;thread;memory")
  set(ETER_SANITIZERS_NORMALIZED "")

  foreach(ET_SANITIZER IN LISTS ETER_SANITIZERS_LIST)
    string(STRIP "${ET_SANITIZER}" ET_SANITIZER_STRIPPED)
    if(NOT ET_SANITIZER_STRIPPED STREQUAL "")
      list(APPEND ETER_SANITIZERS_NORMALIZED "${ET_SANITIZER_STRIPPED}")
    endif()
  endforeach()

  foreach(ET_SANITIZER IN LISTS ETER_SANITIZERS_NORMALIZED)
    list(FIND ETER_SANITIZERS_VALID "${ET_SANITIZER}" ETER_SANITIZER_INDEX)
    if(ETER_SANITIZER_INDEX EQUAL -1)
      message(FATAL_ERROR "Unsupported sanitizer '${ET_SANITIZER}' in ETER_SANITIZERS. Supported values: ${ETER_SANITIZERS_VALID}")
    endif()
  endforeach()

  list(FIND ETER_SANITIZERS_NORMALIZED memory ETER_MEMORY_INDEX)
  if(NOT ETER_MEMORY_INDEX EQUAL -1)
    foreach(ET_SANITIZER IN LISTS ETER_SANITIZERS_NORMALIZED)
      if(ET_SANITIZER STREQUAL "address" OR ET_SANITIZER STREQUAL "thread" OR ET_SANITIZER STREQUAL "leak")
        message(FATAL_ERROR "MemorySanitizer is mutually exclusive with '${ET_SANITIZER}'. Use only 'memory' or another compatible sanitizer set.")
      endif()
    endforeach()
  endif()

  list(JOIN ETER_SANITIZERS_NORMALIZED "," ETER_SANITIZER_FLAGS)

  if(MSVC)
    message(WARNING "ETER_ENABLE_SANITIZERS is ON, but sanitizer support is currently implemented only for Clang/GCC targets.")
  else()
    target_compile_options(EterProjectOptions INTERFACE
      -fsanitize=${ETER_SANITIZER_FLAGS}
      -fno-omit-frame-pointer
    )
    target_link_options(EterProjectOptions INTERFACE
      -fsanitize=${ETER_SANITIZER_FLAGS}
    )
  endif()
endif()

set(ETER_REQUIRED_LLVM_MAJOR 22)
set(ETER_REQUIRED_LLVM_VERSION "${ETER_REQUIRED_LLVM_MAJOR}.0.0")
math(EXPR ETER_REQUIRED_LLVM_NEXT_MAJOR "${ETER_REQUIRED_LLVM_MAJOR} + 1")
set(ETER_REQUIRED_LLVM_NEXT_VERSION "${ETER_REQUIRED_LLVM_NEXT_MAJOR}.0.0")

find_package(LLVM CONFIG REQUIRED)
find_package(MLIR CONFIG REQUIRED)
find_package(Doxygen)

# If requested, find clang-tidy and configure CMake to run it during compilation.
if(ETER_ENABLE_CLANG_TIDY)
  find_program(CLANG_TIDY_BIN clang-tidy HINTS /opt/homebrew/opt/llvm/bin /usr/bin)
  if(CLANG_TIDY_BIN)
    message(STATUS "Found clang-tidy: ${CLANG_TIDY_BIN}; enabling clang-tidy during build")
    # Let users control checks via a .clang-tidy file; don't force checks here.
    set(CMAKE_CXX_CLANG_TIDY "${CLANG_TIDY_BIN}")
  else()
    message(WARNING "ETER_ENABLE_CLANG_TIDY is ON but clang-tidy was not found on PATH")
  endif()
endif()

string(REGEX MATCH "^([0-9]+)\\.([0-9]+)" _ ${LLVM_PACKAGE_VERSION})
set(LLVM_MAJOR ${CMAKE_MATCH_1})
set(LLVM_MINOR ${CMAKE_MATCH_2})

if(NOT LLVM_MAJOR EQUAL ${ETER_REQUIRED_LLVM_MAJOR})
  message(FATAL_ERROR "LLVM major mismatch: required ${ETER_REQUIRED_LLVM_MAJOR}, found ${LLVM_PACKAGE_VERSION}")
endif()

set(DOXYGEN_IN ${CMAKE_SOURCE_DIR}/docsrc/doxygen.cfg.in)
set(DOXYGEN_OUT ${CMAKE_BINARY_DIR}/Doxyfile)

set(abs_top_builddir ${CMAKE_BINARY_DIR})
set(abs_top_srcdir ${CMAKE_SOURCE_DIR}/docsrc)
set(PACKAGE_VERSION ${PROJECT_VERSION})
set(enable_searchengine NO)
set(enable_server_based_search NO)
set(enable_external_search NO)
set(llvm_doxygen_generate_qhp NO)
set(llvm_doxygen_qch_filename "")
set(llvm_doxygen_qhp_namespace "")
set(llvm_doxygen_qhp_cust_filter_name "")
set(llvm_doxygen_qhp_cust_filter_attrs "")
set(llvm_doxygen_qhelpgenerator_path "")
set(searchengine_url "")
set(extra_search_mappings "")
set(DOT "")

configure_file(${DOXYGEN_IN} ${DOXYGEN_OUT} @ONLY)

if(DOXYGEN_FOUND)
  add_custom_target(doc-doxygen
    COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_BINARY_DIR}/doxygen"
    COMMAND ${DOXYGEN_EXECUTABLE} ${DOXYGEN_OUT}
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    COMMENT "Generating API documentation with Doxygen"
    VERBATIM
  )
else()
  add_custom_target(doc-doxygen
    COMMAND ${CMAKE_COMMAND} -E echo "Doxygen not found. Install Doxygen and rerun the target."
    COMMENT "Doxygen not found: install Doxygen to generate API docs"
    VERBATIM
  )
  message(STATUS "Doxygen not found: doc-doxygen will exist as a noop target until Doxygen is installed")
endif()

message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION} in ${LLVM_DIR}")
message(STATUS "Found MLIR in ${MLIR_DIR}")

add_subdirectory(lib)
add_subdirectory(tools)

if(ETER_INCLUDE_TESTS)
  enable_testing()
  add_subdirectory(test)
  add_subdirectory(unittests)

  # Unified target to run both lit tests and unit tests
  # ETER_LIT_EXECUTABLE and FILECHECK_EXECUTABLE are set by test/CMakeLists.txt
  add_custom_target(check-all
    COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure --test-dir ${CMAKE_BINARY_DIR}
    COMMAND ${ETER_LIT_EXECUTABLE} -sv ${CMAKE_BINARY_DIR}/test
    DEPENDS eterc EterUnitTests
    COMMENT "Running all Eter tests (unit + lit)"
  )
endif()
