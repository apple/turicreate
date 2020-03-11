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

# This cmake logic verifies that each of our headers is complete, in that it
# #includes any necessary dependencies, and that it builds under C++ as well.
#
# To do so, we generate a single-line C or C++ source file that includes each
# header, and link all of these stub source files into a test executable.

option(PERFORM_HEADER_CHECK "Performs compile-time checks that each header can be included independently. Requires a C++ compiler.")

if (PERFORM_HEADER_CHECK)
    enable_language(CXX)
endif()

# Call as: aws_check_headers(${target} HEADERS TO CHECK LIST)
function(aws_check_headers target)
    if (PERFORM_HEADER_CHECK)

        set(HEADER_CHECKER_ROOT "${CMAKE_CURRENT_BINARY_DIR}/header-checker")

        # Write stub main file
        set(HEADER_CHECKER_MAIN "${HEADER_CHECKER_ROOT}/stub.c")
        file(WRITE ${HEADER_CHECKER_MAIN} "
            int main(int argc, char **argv) {
                (void)argc;
                (void)argv;

                return 0;
            }")

        set(HEADER_CHECKER_LIB ${target}-header-check)
        add_executable(${HEADER_CHECKER_LIB} ${HEADER_CHECKER_MAIN})
        target_link_libraries(${HEADER_CHECKER_LIB} ${target})
        target_compile_definitions(${HEADER_CHECKER_LIB} PRIVATE AWS_UNSTABLE_TESTING_API=1 AWS_HEADER_CHECKER=1)

        # We want to be able to verify that the proper C++ header guards are in place, so
        # build this target as a C++ application
        set_target_properties(${HEADER_CHECKER_LIB} PROPERTIES
            LINKER_LANGUAGE CXX
            CXX_STANDARD 11
            CXX_STANDARD_REQUIRED 0
            C_STANDARD 90
        )

        foreach(header IN LISTS ARGN)
            if (NOT ${header} MATCHES "\\.inl$")
                file(RELATIVE_PATH rel_header ${CMAKE_HOME_DIRECTORY} ${header})
                file(RELATIVE_PATH include_path "${CMAKE_HOME_DIRECTORY}/include" ${header})
                set(stub_dir "${HEADER_CHECKER_ROOT}/${rel_header}")
                file(MAKE_DIRECTORY "${stub_dir}")
                file(WRITE "${stub_dir}/check.c" "#include <${include_path}>\n")
                file(WRITE "${stub_dir}/checkcpp.cpp" "#include <${include_path}>\n")

                target_sources(${HEADER_CHECKER_LIB} PUBLIC "${stub_dir}/check.c" "${stub_dir}/checkcpp.cpp")
            endif()
        endforeach(header)
    endif() # PERFORM_HEADER_CHECK
endfunction()
