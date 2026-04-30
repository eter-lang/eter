#===----------------------------------------------------------------------===#
#
# Part of the Eter Project, under the Apache License v2.0 with LLVM Exceptions.
# See /LICENSE for license information.
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
#
#===----------------------------------------------------------------------===#

# Add the Base library unit tests
add_unittest(EterUnitTests BaseSpanTest SpanTest.cpp)
target_link_libraries(BaseSpanTest PRIVATE EterBase)

add_unittest(EterUnitTests BaseSourceBufferTest SourceBufferTest.cpp)
target_link_libraries(BaseSourceBufferTest PRIVATE EterBase)

add_unittest(EterUnitTests BaseSourceManagerTest SourceManagerTest.cpp)
target_link_libraries(BaseSourceManagerTest PRIVATE EterBase)

add_unittest(EterUnitTests BaseDiagnosticEngineTest DiagnosticEngineTest.cpp)
target_link_libraries(BaseDiagnosticEngineTest PRIVATE EterBase)

# Set folder for IDE organization
set_target_properties(BaseSpanTest BaseSourceBufferTest BaseSourceManagerTest BaseDiagnosticEngineTest
  PROPERTIES FOLDER "Eter/Tests/Base"
)

# Add include directories
target_include_directories(BaseSpanTest PRIVATE ${CMAKE_SOURCE_DIR}/include)
target_include_directories(BaseSourceBufferTest PRIVATE ${CMAKE_SOURCE_DIR}/include)
target_include_directories(BaseSourceManagerTest PRIVATE ${CMAKE_SOURCE_DIR}/include)
target_include_directories(BaseDiagnosticEngineTest PRIVATE ${CMAKE_SOURCE_DIR}/include)

target_include_directories(BaseSpanTest SYSTEM PRIVATE ${LLVM_INCLUDE_DIRS} ${MLIR_INCLUDE_DIRS})
target_include_directories(BaseSourceBufferTest SYSTEM PRIVATE ${LLVM_INCLUDE_DIRS} ${MLIR_INCLUDE_DIRS})
target_include_directories(BaseSourceManagerTest SYSTEM PRIVATE ${LLVM_INCLUDE_DIRS} ${MLIR_INCLUDE_DIRS})
target_include_directories(BaseDiagnosticEngineTest SYSTEM PRIVATE ${LLVM_INCLUDE_DIRS} ${MLIR_INCLUDE_DIRS})
