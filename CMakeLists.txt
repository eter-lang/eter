# Part of the Quar Language project, under the Apache License v2.0 with LLVM
# Exceptions. See /LICENSE for license information.
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

cmake_minimum_required(VERSION 3.20)

project(quar VERSION 0.1.0 LANGUAGES C CXX)

include(GNUInstallDirs)

option(QUAR_ENABLE_WERROR "Treat warnings as errors" ON)
option(QUAR_ENABLE_ASSERTIONS "Enable assertions in Quar" ON)
option(QUAR_INCLUDE_TESTS "Enable Quar's lit/FileCheck test suite" ON)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE Debug CACHE STRING "Build type" FORCE)
endif()

add_library(QuarProjectOptions INTERFACE)
target_compile_features(QuarProjectOptions INTERFACE cxx_std_23)

if(MSVC)
  target_compile_options(QuarProjectOptions INTERFACE /W4)
  if(QUAR_ENABLE_WERROR)
    target_compile_options(QuarProjectOptions INTERFACE /WX)
  endif()
else()
  target_compile_options(QuarProjectOptions INTERFACE -Wall -Wextra -Wpedantic)
  if(QUAR_ENABLE_WERROR)
    target_compile_options(QuarProjectOptions INTERFACE -Werror)
  endif()
endif()

if(QUAR_ENABLE_ASSERTIONS)
  target_compile_definitions(QuarProjectOptions INTERFACE QUAR_ENABLE_ASSERTIONS=1)
endif()

set(QUAR_REQUIRED_LLVM_MAJOR 22)
set(QUAR_REQUIRED_LLVM_VERSION "${QUAR_REQUIRED_LLVM_MAJOR}.0.0")
math(EXPR QUAR_REQUIRED_LLVM_NEXT_MAJOR "${QUAR_REQUIRED_LLVM_MAJOR} + 1")
set(QUAR_REQUIRED_LLVM_NEXT_VERSION "${QUAR_REQUIRED_LLVM_NEXT_MAJOR}.0.0")

find_package(LLVM CONFIG REQUIRED)
find_package(MLIR CONFIG REQUIRED)

if(LLVM_PACKAGE_VERSION VERSION_LESS QUAR_REQUIRED_LLVM_VERSION OR
   NOT LLVM_PACKAGE_VERSION VERSION_LESS QUAR_REQUIRED_LLVM_NEXT_VERSION)
  message(FATAL_ERROR
    "Quar requires LLVM/MLIR ${QUAR_REQUIRED_LLVM_MAJOR}.x, but found LLVM ${LLVM_PACKAGE_VERSION}"
  )
endif()

message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION} in ${LLVM_DIR}")
message(STATUS "Found MLIR in ${MLIR_DIR}")

add_subdirectory(lib)
add_subdirectory(tools)

if(QUAR_INCLUDE_TESTS)
  add_subdirectory(test)
  add_subdirectory(unittests)
endif()
