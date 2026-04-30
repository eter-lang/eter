#===----------------------------------------------------------------------===#
#
# Part of the Eter Project, under the Apache License v2.0 with LLVM Exceptions.
# See /LICENSE for license information.
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
#
#===----------------------------------------------------------------------===#

include(FetchContent)

# Google Test setup - using FetchContent to download and build gtest
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/refs/tags/v1.15.2.zip
  DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)
# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

# Add a custom target for all unit tests (similar to LLVM's approach)
add_custom_target(EterUnitTests)
set_target_properties(EterUnitTests PROPERTIES FOLDER "Eter/Tests")

# Function to add a unit test (similar to LLVM's add_unittest)
# Usage: add_unittest(test_suite test_name source1.cpp source2.cpp ...)
function(add_unittest test_suite test_name)
  add_executable(${test_name} ${ARGN})
  target_link_libraries(${test_name} PRIVATE GTest::gtest_main GTest::gtest ${LLVM_PTHREAD_LIB})
  target_compile_features(${test_name} PRIVATE cxx_std_17)
  target_compile_options(${test_name} PRIVATE
    $<$<COMPILE_LANGUAGE:CXX>:-Wno-error>
    $<$<COMPILE_LANGUAGE:CXX>:-Wno-unused-parameter>
  )
  add_dependencies(${test_suite} ${test_name})
  add_test(NAME ${test_name} COMMAND ${test_name})
  set_target_properties(${test_name} PROPERTIES FOLDER "Eter/Tests")
endfunction()

# Function to add a unit test with input files
function(add_unittest_with_input_files test_suite test_name)
  add_unittest(${test_suite} ${test_name} ${ARGN})
endfunction()

# Helper function for adding Eter unit tests
function(add_eter_unittest test_dirname)
  add_subdirectory(${test_dirname})
endfunction()

# Add unittest subdirectories here as Eter grows
add_eter_unittest(Base)
add_eter_unittest(Driver)
add_eter_unittest(Lexer)
