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
include(CheckIncludeFile)
include(CMakeParseArguments) # needed for CMake v3.4 and lower

option(AWS_ENABLE_LTO "Enables LTO on libraries. Ensure this is set on all consumed targets, or linking will fail" OFF)
option(LEGACY_COMPILER_SUPPORT "This enables builds with compiler versions such as gcc 4.1.2. This is not a 'supported' feature; it's just a best effort." OFF)

# This function will set all common flags on a target
# Options:
#  NO_WGNU: Disable -Wgnu
#  NO_WEXTRA: Disable -Wextra
#  NO_PEDANTIC: Disable -pedantic
function(aws_set_common_properties target)
    set(options NO_WGNU NO_WEXTRA NO_PEDANTIC NO_LTO)
    cmake_parse_arguments(SET_PROPERTIES "${options}" "" "" ${ARGN})

    if(MSVC)
        # Remove other /W flags
        if(CMAKE_C_FLAGS MATCHES "/W[0-4]")
            string(REGEX REPLACE "/W[0-4]" "" CMAKE_C_FLAGS "${CMAKE_C_FLAGS}")
            set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS}" PARENT_SCOPE)
        endif()

        list(APPEND AWS_C_FLAGS /W4 /WX /MP)
        # /volatile:iso relaxes some implicit memory barriers that MSVC normally applies for volatile accesses
        # Since we want to be compatible with user builds using /volatile:iso, use it for the tests.
        list(APPEND AWS_C_FLAGS /volatile:iso)

        string(TOUPPER "${CMAKE_BUILD_TYPE}" _CMAKE_BUILD_TYPE)
        if(STATIC_CRT)
            string(REPLACE "/MD" "/MT" _FLAGS "${CMAKE_C_FLAGS_${_CMAKE_BUILD_TYPE}}")
        else()
            string(REPLACE "/MT" "/MD" _FLAGS "${CMAKE_C_FLAGS_${_CMAKE_BUILD_TYPE}}")
        endif()
        string(REPLACE " " ";" _FLAGS "${_FLAGS}")
        list(APPEND AWS_C_FLAGS "${_FLAGS}")

    else()
        list(APPEND AWS_C_FLAGS -Wall -Werror -Wstrict-prototypes)

        if(NOT SET_PROPERTIES_NO_WEXTRA)
            list(APPEND AWS_C_FLAGS -Wextra)
        endif()

        if(NOT SET_PROPERTIES_NO_PEDANTIC)
            list(APPEND AWS_C_FLAGS -pedantic)
        endif()

        # Warning disables always go last to avoid future flags re-enabling them
        list(APPEND AWS_C_FLAGS -Wno-long-long)

        # Always enable position independent code, since this code will always end up in a shared lib
        list(APPEND AWS_C_FLAGS -fPIC)

        if (LEGACY_COMPILER_SUPPORT)
            list(APPEND AWS_C_FLAGS -Wno-strict-aliasing)
        endif()
    endif()

    check_include_file(stdint.h HAS_STDINT)
    check_include_file(stdbool.h HAS_STDBOOL)

    if (NOT HAS_STDINT)
        list(APPEND AWS_C_FLAGS -DNO_STDINT)
    endif()

    if (NOT HAS_STDBOOL)
        list(APPEND AWS_C_FLAGS -DNO_STDBOOL)
    endif()

    if(NOT SET_PROPERTIES_NO_WGNU)
        check_c_compiler_flag(-Wgnu HAS_WGNU)
        if(HAS_WGNU)
            # -Wgnu-zero-variadic-macro-arguments results in a lot of false positives
            list(APPEND AWS_C_FLAGS -Wgnu -Wno-gnu-zero-variadic-macro-arguments)

            # some platforms implement htonl family of functions via GNU statement expressions (https://gcc.gnu.org/onlinedocs/gcc/Statement-Exprs.html)
            # which generates -Wgnu-statement-expression warning.
            set(old_flags "${CMAKE_REQUIRED_FLAGS}")
            set(CMAKE_REQUIRED_FLAGS "-Wgnu -Werror")
            check_c_source_compiles("
            #include <netinet/in.h>

            int main() {
            uint32_t x = 0;
            x = htonl(x);
            return (int)x;
            }"  NO_GNU_EXPR)
            set(CMAKE_REQUIRED_FLAGS "${old_flags}")

            if (NOT NO_GNU_EXPR)
                list(APPEND AWS_C_FLAGS -Wno-gnu-statement-expression)
            endif()
        endif()
    endif()

    # some platforms (especially when cross-compiling) do not have the sysconf API in their toolchain files.
    check_c_source_compiles("
    #include <unistd.h>
    int main() {
      sysconf(_SC_NPROCESSORS_ONLN);
    }"  HAVE_SYSCONF)

    if (HAVE_SYSCONF)
        list(APPEND AWS_C_DEFINES_PRIVATE -DHAVE_SYSCONF)
    endif()

    if(CMAKE_BUILD_TYPE STREQUAL "" OR CMAKE_BUILD_TYPE MATCHES Debug)
        list(APPEND AWS_C_DEFINES_PRIVATE -DDEBUG_BUILD)
    else() # release build
        if (POLICY CMP0069)
            if ((NOT SET_PROPERTIES_NO_LTO) AND AWS_ENABLE_LTO)
                cmake_policy(SET CMP0069 NEW) # Enable LTO/IPO if available in the compiler
                include(CheckIPOSupported OPTIONAL RESULT_VARIABLE ipo_check_exists)
                if (ipo_check_exists)
                    check_ipo_supported(RESULT ipo_supported)
                    if (ipo_supported)
                        message(STATUS "Enabling IPO/LTO for Release builds")
                        set(AWS_ENABLE_LTO ON)
                    else()
                        message(STATUS "AWS_ENABLE_LTO is enabled, but cmake/compiler does not support it, disabling")
                        set(AWS_ENABLE_LTO OFF)
                    endif()
                endif()
            endif()
        endif()
    endif()

    if(BUILD_SHARED_LIBS)
        if (NOT MSVC)
            # this should only be set when building shared libs.
            list(APPEND AWS_C_FLAGS "-fvisibility=hidden")
        endif()
    endif()

    target_compile_options(${target} PRIVATE ${AWS_C_FLAGS})
    target_compile_definitions(${target} PRIVATE ${AWS_C_DEFINES_PRIVATE} PUBLIC ${AWS_C_DEFINES_PUBLIC})
    set_target_properties(${target} PROPERTIES LINKER_LANGUAGE C C_STANDARD 99 C_STANDARD_REQUIRED ON)
    if (AWS_ENABLE_LTO)
        set_target_properties(${target} PROPERTIES INTERPROCEDURAL_OPTIMIZATION TRUE)
    endif()
endfunction()
