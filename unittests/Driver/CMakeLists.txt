#===----------------------------------------------------------------------===#
#
# Part of the Eter Project, under the Apache License v2.0 with LLVM Exceptions.
# See /LICENSE for license information.
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
#
#===----------------------------------------------------------------------===#

# Add the Driver library unit tests (LLVM style: one executable, multiple .cpp files)
add_unittest(EterUnitTests EterDriverTests
  VersionTest.cpp
  DriverTest.cpp
)

target_link_libraries(EterDriverTests PRIVATE EterDriver EterBase)

set_target_properties(EterDriverTests
  PROPERTIES FOLDER "Eter/Tests/Driver"
)

target_include_directories(EterDriverTests PRIVATE ${CMAKE_SOURCE_DIR}/include)
target_include_directories(EterDriverTests SYSTEM PRIVATE ${LLVM_INCLUDE_DIRS} ${MLIR_INCLUDE_DIRS})
