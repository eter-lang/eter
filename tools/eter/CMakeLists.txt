#===----------------------------------------------------------------------===#
#
# Part of the Eter Project, under the Apache License v2.0 with LLVM Exceptions.
# See /LICENSE for license information.
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
#
#===----------------------------------------------------------------------===#

add_executable(eterc eter.cpp)

target_link_libraries(eterc
  PRIVATE
    eter::Driver
)

target_include_directories(eterc SYSTEM
  PRIVATE
    ${LLVM_INCLUDE_DIRS}
)

target_compile_options(eterc
  PRIVATE
    ${LLVM_CXXFLAGS}
    -Wno-unused-parameter # LLVM headers trigger this warning, and we don't want to see it in our code.
)
