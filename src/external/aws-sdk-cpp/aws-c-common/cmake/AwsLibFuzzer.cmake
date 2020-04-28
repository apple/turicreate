# Copyright 2010-2018 Amazon.com, Inc. or its affiliates. All Rights Reserved.
#
# Licensed under the Apache License, Version 2.0 (the "License").
# You may not use this file except in compliance with the License.
# A copy of the License is located at
#
#  http://aws.amazon.com/apache2.0
#
# or in the "license" file accompanying this file. This file is distributed
# on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either
# express or implied. See the License for the specific language governing
# permissions and limitations under the License.

include(CTest)
include(AwsSanitizers)

option(ENABLE_FUZZ_TESTS "Build and run fuzz tests" OFF)
set(FUZZ_TESTS_MAX_TIME 60 CACHE STRING "Max time to run each fuzz test")

# Adds fuzz tests to ctest
# Options:
#  fuzz_files: The list of fuzz test files
#  other_files: Other files to link into each fuzz test
#  corpus_dir:  directory where corpus files can be found
function(aws_add_fuzz_tests fuzz_files other_files corpus_dir)
    if(ENABLE_FUZZ_TESTS)
        if(NOT ENABLE_SANITIZERS)
            message(FATAL_ERROR "ENABLE_FUZZ_TESTS is set but ENABLE_SANITIZERS is set to OFF")
        endif()

        aws_check_sanitizer(fuzzer)
        if (NOT HAS_SANITIZER_fuzzer)
            message(FATAL_ERROR "ENABLE_FUZZ_TESTS is set but the current compiler (${CMAKE_CXX_COMPILER_ID}) doesn't support -fsanitize=fuzzer")
        endif()

        foreach(test_file ${fuzz_files})
            get_filename_component(TEST_FILE_NAME ${test_file} NAME_WE)

            set(FUZZ_BINARY_NAME ${PROJECT_NAME}-fuzz-${TEST_FILE_NAME})
            add_executable(${FUZZ_BINARY_NAME} ${test_file} ${other_files})
            target_link_libraries(${FUZZ_BINARY_NAME} PRIVATE ${PROJECT_NAME})
            aws_set_common_properties(${FUZZ_BINARY_NAME})
            aws_add_sanitizers(${FUZZ_BINARY_NAME} SANITIZERS "fuzzer")
            target_compile_definitions(${FUZZ_BINARY_NAME} PRIVATE AWS_UNSTABLE_TESTING_API=1)
            target_include_directories(${FUZZ_BINARY_NAME} PRIVATE ${CMAKE_CURRENT_LIST_DIR})

            if (corpus_dir)
                file(TO_NATIVE_PATH "${corpus_dir}/${TEST_FILE_NAME}" TEST_CORPUS_DIR)
            endif()

            if (TEST_CORPUS_DIR AND (EXISTS "${TEST_CORPUS_DIR}"))
                add_test(NAME fuzz_${TEST_FILE_NAME} COMMAND ${FUZZ_BINARY_NAME} -timeout=1 -max_total_time=${FUZZ_TESTS_MAX_TIME} "${TEST_CORPUS_DIR}")
            else()
                add_test(NAME fuzz_${TEST_FILE_NAME} COMMAND ${FUZZ_BINARY_NAME} -timeout=1 -max_total_time=${FUZZ_TESTS_MAX_TIME})
            endif()
        endforeach()
    endif()
endfunction()
