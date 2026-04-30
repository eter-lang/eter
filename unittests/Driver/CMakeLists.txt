#===----------------------------------------------------------------------===#
#
# Part of the Eter Project, under the Apache License v2.0 with LLVM Exceptions.
# See /LICENSE for license information.
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
#
#===----------------------------------------------------------------------===#

# Add the Driver library unit tests
add_unittest(EterUnitTests DriverVersionTest VersionTest.cpp)
target_link_libraries(DriverVersionTest PRIVATE EterDriver)

add_unittest(EterUnitTests DriverTest DriverTest.cpp)
target_link_libraries(DriverTest PRIVATE EterDriver EterBase)

# Set folder for IDE organization
set_target_properties(DriverVersionTest DriverTest
  PROPERTIES FOLDER "Eter/Tests/Driver"
)

# Add include directories
target_include_directories(DriverVersionTest PRIVATE ${CMAKE_SOURCE_DIR}/include)
target_include_directories(DriverTest PRIVATE ${CMAKE_SOURCE_DIR}/include)

target_include_directories(DriverVersionTest SYSTEM PRIVATE ${LLVM_INCLUDE_DIRS} ${MLIR_INCLUDE_DIRS})
target_include_directories(DriverTest SYSTEM PRIVATE ${LLVM_INCLUDE_DIRS} ${MLIR_INCLUDE_DIRS})
