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

include(AwsCFlags)
include(AwsSanitizers)

option(ENABLE_NET_TESTS "Run tests requiring an internet connection." ON)

# Registers a test case by name (the first argument to the AWS_TEST_CASE macro in aws_test_harness.h)
macro(add_test_case name)
    list(APPEND TEST_CASES "${name}")
endmacro()

# Like add_test_case, but for tests that require a working internet connection.
macro(add_net_test_case name)
    if (ENABLE_NET_TESTS)
        list(APPEND TEST_CASES "${name}")
    endif()
endmacro()

# Generate a test driver executable with the given name
function(generate_test_driver driver_exe_name)
    create_test_sourcelist(test_srclist test_runner.c ${TEST_CASES})
    # Write clang tidy file that disables all but one check to avoid false positives
    file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/.clang-tidy" "Checks: '-*,misc-static-assert'")

    add_executable(${driver_exe_name} ${CMAKE_CURRENT_BINARY_DIR}/test_runner.c ${TESTS})
    aws_set_common_properties(${driver_exe_name} NO_WEXTRA NO_PEDANTIC)
    aws_add_sanitizers(${driver_exe_name} ${${PROJECT_NAME}_SANITIZERS})

    target_link_libraries(${driver_exe_name} PRIVATE ${PROJECT_NAME})

    set_target_properties(${driver_exe_name} PROPERTIES LINKER_LANGUAGE C C_STANDARD 99)
    target_compile_definitions(${driver_exe_name} PRIVATE AWS_UNSTABLE_TESTING_API=1)
    target_include_directories(${driver_exe_name} PRIVATE ${CMAKE_CURRENT_LIST_DIR})

    foreach(name IN LISTS TEST_CASES)
        add_test(${name} ${driver_exe_name} "${name}")
    endforeach()

    # Clear test cases in case another driver needsto be generated
    unset(TEST_CASES PARENT_SCOPE)
endfunction()

function(generate_cpp_test_driver driver_exe_name)
    create_test_sourcelist(test_srclist test_runner.cpp ${TEST_CASES})

    add_executable(${driver_exe_name} ${CMAKE_CURRENT_BINARY_DIR}/test_runner.cpp ${TESTS})
    target_link_libraries(${driver_exe_name} PRIVATE ${PROJECT_NAME})

    set_target_properties(${driver_exe_name} PROPERTIES LINKER_LANGUAGE CXX)
    target_compile_definitions(${driver_exe_name} PRIVATE AWS_UNSTABLE_TESTING_API=1)
    target_include_directories(${driver_exe_name} PRIVATE ${CMAKE_CURRENT_LIST_DIR})

    foreach(name IN LISTS TEST_CASES)
        add_test(${name} ${driver_exe_name} "${name}")
    endforeach()

    # Clear test cases in case another driver needsto be generated
    unset(TEST_CASES PARENT_SCOPE)
endfunction()
