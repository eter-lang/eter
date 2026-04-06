#===----------------------------------------------------------------------===#
#
# Part of the Eter Project, under the Apache License v2.0 with LLVM Exceptions.
# See /LICENSE for license information.
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
#
#===----------------------------------------------------------------------===#

add_library(EterBasic
  Basic/Version.cpp
)

add_library(eter::Basic ALIAS EterBasic)

target_link_libraries(EterBasic
  PUBLIC
    EterProjectOptions
)

target_include_directories(EterBasic
  PUBLIC
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)
