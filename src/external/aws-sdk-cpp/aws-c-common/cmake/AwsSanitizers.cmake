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

include(CheckCCompilerFlag)

option(ENABLE_SANITIZERS "Enable sanitizers in debug builds" OFF)
set(SANITIZERS "address;undefined" CACHE STRING "List of sanitizers to build with")

# This function checks if a sanitizer is available
# Options:
#  sanitizer: The sanitizer to check
#  out_variable: The variable to assign the result to. Defaults to HAS_SANITIZER_${sanitizer}
function(aws_check_sanitizer sanitizer)

    if(NOT ${ARGN})
        set(out_variable "${ARGN}")
    else()
        set(out_variable HAS_SANITIZER_${sanitizer})
        # Sanitize the variable name to remove illegal characters
        string(MAKE_C_IDENTIFIER ${out_variable} out_variable)
    endif()

    if(ENABLE_SANITIZERS)
        # When testing for libfuzzer, if attempting to link there will be 2 mains
        if(${sanitizer} STREQUAL "fuzzer")
            set(sanitizer_test_flag -fsanitize=fuzzer-no-link)
        else()
            set(sanitizer_test_flag -fsanitize=${sanitizer})
        endif()

        # Need to set this here so that the flag is passed to the linker
        set(CMAKE_REQUIRED_FLAGS ${sanitizer_test_flag})
        check_c_compiler_flag(${sanitizer_test_flag} ${out_variable})
    else()
        set(${out_variable} 0 PARENT_SCOPE)
    endif()
endfunction()

# This function enables sanitizers on the given target
# Options:
#  SANITIZERS: The list of extra sanitizers to enable
#  BLACKLIST: The blacklist file to use (passed to -fsanitizer-blacklist=)
function(aws_add_sanitizers target)
    set(oneValueArgs BLACKLIST)
    set(multiValueArgs SANITIZERS)
    cmake_parse_arguments(SANITIZER "" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    check_c_compiler_flag(-fsanitize= HAS_SANITIZERS)
    if(HAS_SANITIZERS)

        list(APPEND SANITIZER_SANITIZERS ${SANITIZERS})
        message(STATUS "attempting to use sanitizer list ${SANITIZER_SANITIZERS}")

        foreach(sanitizer IN LISTS SANITIZER_SANITIZERS)

            set(sanitizer_variable HAS_SANITIZER_${sanitizer})
            # Sanitize the variable name to remove illegal characters
            string(MAKE_C_IDENTIFIER ${sanitizer_variable} sanitizer_variable)

            aws_check_sanitizer(${sanitizer} ${sanitizer_variable})
            if(${${sanitizer_variable}})
                set(PRESENT_SANITIZERS "${PRESENT_SANITIZERS},${sanitizer}")
            endif()
        endforeach()

        if(PRESENT_SANITIZERS)
            target_compile_options(${target} PRIVATE -fno-omit-frame-pointer -fsanitize=${PRESENT_SANITIZERS})
            target_link_libraries(${target} PUBLIC "-fno-omit-frame-pointer -fsanitize=${PRESENT_SANITIZERS}")

            if(SANITIZER_BLACKLIST)
                target_compile_options(${target} PRIVATE -fsanitize-blacklist=${CMAKE_CURRENT_SOURCE_DIR}/${SANITIZER_BLACKLIST})
            endif()

            string(REPLACE "," ";" PRESENT_SANITIZERS "${PRESENT_SANITIZERS}")
            set(${target}_SANITIZERS ${PRESENT_SANITIZERS} PARENT_SCOPE)
        endif()
    endif()
endfunction()
