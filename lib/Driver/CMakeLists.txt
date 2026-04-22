#===----------------------------------------------------------------------===#
#
# Part of the Eter Project, under the Apache License v2.0 with LLVM Exceptions.
# See /LICENSE for license information.
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
#
#===----------------------------------------------------------------------===#

add_library(EterDriver
  Driver.cpp
  Version.cpp
)

add_library(eter::Driver ALIAS EterDriver)

target_link_libraries(EterDriver
  PUBLIC
    EterProjectOptions
    LLVM
)

target_include_directories(EterDriver
  PUBLIC
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

target_include_directories(EterDriver SYSTEM
  PRIVATE
    ${LLVM_INCLUDE_DIRS}
)

target_compile_options(EterDriver
  PRIVATE
    -Wno-unused-parameter
)

target_compile_options(EterDriver
  PRIVATE
    -Wno-unused-parameter
)
