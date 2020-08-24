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

cmake_minimum_required(VERSION 3.0)
option(STATIC_CRT "Windows specific option that to specify static/dynamic run-time library" OFF)
option(ALLOW_CROSS_COMPILED_TESTS "Allow tests to be compiled via cross compile, for use with qemu" OFF)

project(aws-c-common LANGUAGES C VERSION 0.1.0)

if (POLICY CMP0069)
    cmake_policy(SET CMP0069 NEW) # Enable LTO/IPO if available in the compiler, see AwsCFlags
endif()

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include(AwsCFlags)
include(AwsCheckHeaders)
include(AwsSharedLibSetup)
include(AwsFeatureTests)
include(AwsSanitizers)
include(AwsSIMD)
include(CTest)

set(GENERATED_ROOT_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated")
set(GENERATED_INCLUDE_DIR "${GENERATED_ROOT_DIR}/include")
set(GENERATED_CONFIG_HEADER "${GENERATED_INCLUDE_DIR}/aws/common/config.h")
set(CONFIG_HEADER_TEMPLATE "${CMAKE_CURRENT_SOURCE_DIR}/include/aws/common/config.h.in")

file(GLOB AWS_COMMON_HEADERS
        "include/aws/common/*.h"
        "include/aws/common/*.inl"
        )

file(GLOB AWS_TEST_HEADERS
        "include/aws/testing/*.h"
        )

file(GLOB AWS_COMMON_PRIV_HEADERS
        "include/aws/common/private/*.h"
        "include/aws/common/private/*.c"
        )

file(GLOB AWS_COMMON_SRC
        "source/*.c"
        )

option(AWS_NUM_CPU_CORES "Number of CPU cores of the target machine. Useful when cross-compiling." 0)

if (WIN32)
    set(WINDOWS_KERNEL_LIB "Kernel32" CACHE STRING "The name of the kernel library to link against (default: Kernel32)")

    file(GLOB AWS_COMMON_OS_HEADERS
            "include/aws/common/windows/*"
            )
    file(GLOB AWS_COMMON_OS_SRC
            "source/windows/*.c"
            )

    if (MSVC)
        source_group("Header Files\\aws\\common" FILES ${AWS_COMMON_HEADERS})
        source_group("Header Files\\aws\\common\\private" FILES ${AWS_COMMON_PRIV_HEADERS})
        source_group("Header Files\\aws\\testing" FILES ${AWS_TEST_HEADERS})
        source_group("Source Files" FILES ${AWS_COMMON_SRC})
        source_group("Source Files\\windows" FILES ${AWS_COMMON_OS_SRC})
    endif ()

    list(APPEND PLATFORM_DEFINES WINDOWS_KERNEL_LIB=${WINDOWS_KERNEL_LIB})
    list(APPEND PLATFORM_LIBS BCrypt ${WINDOWS_KERNEL_LIB} Ws2_32)
else ()
    file(GLOB AWS_COMMON_OS_HEADERS
        "include/aws/common/posix/*"
        )
    file(GLOB AWS_COMMON_OS_SRC
        "source/posix/*.c"
        )
    set(THREADS_PREFER_PTHREAD_FLAG ON)
    find_package(Threads REQUIRED)
    if (APPLE)
        find_library(CORE_FOUNDATION_LIB CoreFoundation)
        if (NOT CORE_FOUNDATION_LIB)
            message(FATAL_ERROR "Core Foundation not found")
        endif ()

        list(APPEND PLATFORM_LIBS Threads::Threads ${CORE_FOUNDATION_LIB})
    elseif (${CMAKE_SYSTEM_NAME} STREQUAL "Linux") # Android does not link to libpthread nor librt, so this is fine
        list(APPEND PLATFORM_LIBS m Threads::Threads rt)
    elseif(CMAKE_SYSTEM_NAME STREQUAL "FreeBSD")
        list(APPEND PLATFORM_LIBS m thr execinfo)
    elseif(CMAKE_SYSTEM_NAME STREQUAL "NetBSD")
        list(APPEND PLATFORM_LIBS m Threads::Threads execinfo)
    endif()

endif()

list(APPEND PLATFORM_LIBS ${CMAKE_DL_LIBS})

file(GLOB COMMON_HEADERS
        ${AWS_COMMON_HEADERS}
        ${AWS_COMMON_OS_HEADERS}
        ${AWS_COMMON_PRIV_HEADERS}
        ${AWS_TEST_HEADERS}
        )

file(GLOB COMMON_SRC
        ${AWS_COMMON_SRC}
        ${AWS_COMMON_OS_SRC}
        )


add_library(${PROJECT_NAME} ${COMMON_SRC})
aws_set_common_properties(${PROJECT_NAME} NO_WEXTRA)
aws_prepare_symbol_visibility_args(${PROJECT_NAME} "AWS_COMMON")
target_compile_options(${PROJECT_NAME} PUBLIC ${PLATFORM_CFLAGS})

aws_check_headers(${PROJECT_NAME} ${AWS_COMMON_HEADERS} ${AWS_TEST_HEADERS} ${AWS_COMMON_OS_HEADERS})

#apple source already includes the definitions we want, and setting this posix source
#version causes it to revert to an older version. So don't turn it on there, we don't need it.
if (UNIX AND NOT APPLE AND NOT ${CMAKE_SYSTEM_NAME} MATCHES FreeBSD)
    #this only gets applied to aws-c-common (not its consumers).
    target_compile_definitions(${PROJECT_NAME} PRIVATE -D_POSIX_C_SOURCE=200809L -D_XOPEN_SOURCE=500)
endif()

aws_add_sanitizers(${PROJECT_NAME} BLACKLIST "sanitizer-blacklist.txt")
target_link_libraries(${PROJECT_NAME} PUBLIC ${PLATFORM_LIBS})
target_compile_definitions(${PROJECT_NAME} PRIVATE ${PLATFORM_DEFINES})

if (AWS_NUM_CPU_CORES)
    target_compile_definitions(${PROJECT_NAME} PRIVATE -DAWS_NUM_CPU_CORES=${AWS_NUM_CPU_CORES})
endif()

# Our ABI is not yet stable
set_target_properties(${PROJECT_NAME} PROPERTIES VERSION 1.0.0)
set_target_properties(${PROJECT_NAME} PROPERTIES SOVERSION 0unstable)

target_include_directories(${PROJECT_NAME} PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>)
# When we install, the generated header will be at the INSTALL_INTERFACE:include location,
# but at build time we need to explicitly include this here
target_include_directories(${PROJECT_NAME} PUBLIC
        $<BUILD_INTERFACE:${GENERATED_INCLUDE_DIR}>)

# Enable SIMD encoder if the compiler supports the right features
simd_add_definitions(${PROJECT_NAME})

if (HAVE_MAY_I_USE OR HAVE_BUILTIN_CPU_SUPPORTS OR HAVE_MSVC_CPUIDEX)
    set(HAVE_SIMD_CPUID TRUE)
endif()

if (HAVE_AVX2_INTRINSICS AND HAVE_SIMD_CPUID)
    target_compile_definitions(${PROJECT_NAME} PRIVATE -DUSE_SIMD_ENCODING)
    target_sources(${PROJECT_NAME} PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/source/arch/cpuid.c")
    simd_add_source_avx2(${PROJECT_NAME} "${CMAKE_CURRENT_SOURCE_DIR}/source/arch/encoding_avx2.c")
    message(STATUS "Building SIMD base64 decoder")
endif()

# Preserve subdirectories when installing headers
foreach(HEADER_SRCPATH IN ITEMS ${AWS_COMMON_HEADERS} ${AWS_COMMON_OS_HEADERS} ${GENERATED_CONFIG_HEADER} ${AWS_TEST_HEADERS})
    get_filename_component(HEADER_DIR ${HEADER_SRCPATH} DIRECTORY)
# Note: We need to replace the generated include directory component first, otherwise if the build
# directory is located inside the source tree, we'll partially rewrite the path and fail to replace it
# when we replace the generated include dir.
# We also need to take care to not run the source-directory match if the generated-directory match
# succeeds; otherwise, if we're installing to /foo/aws-c-common-install, and our source directory is
# /foo/aws-c-common, we'll end up installing to /foo/aws-c-common-install-install

    unset(HEADER_DSTDIR)

    foreach(POTENTIAL_PREFIX IN ITEMS ${GENERATED_ROOT_DIR} ${CMAKE_CURRENT_SOURCE_DIR})
        string(LENGTH ${POTENTIAL_PREFIX} _prefixlen)
        string(SUBSTRING ${HEADER_DIR} 0 ${_prefixlen} _actual_prefix)
        if(${_actual_prefix} STREQUAL ${POTENTIAL_PREFIX})
            string(REPLACE "${POTENTIAL_PREFIX}/" "" HEADER_DSTDIR "${HEADER_DIR}")
            break()
        endif()
    endforeach()

    if(NOT HEADER_DSTDIR)
        message(ERROR "Couldn't find source root for header ${HEADER_SRCPATH}")
    endif()

    install(FILES ${HEADER_SRCPATH}
            DESTINATION ${HEADER_DSTDIR}
            COMPONENT Development)
endforeach()

aws_prepare_shared_lib_exports(${PROJECT_NAME})

configure_file("cmake/${PROJECT_NAME}-config.cmake"
  "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}-config.cmake"
  @ONLY)

if (BUILD_SHARED_LIBS)
   set (TARGET_DIR "shared")
else()
   set (TARGET_DIR "static")
endif()

install(EXPORT "${PROJECT_NAME}-targets"
    DESTINATION "${LIBRARY_DIRECTORY}/${PROJECT_NAME}/cmake/${TARGET_DIR}"
    NAMESPACE AWS::
    COMPONENT Development)

install(FILES "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}-config.cmake"
    DESTINATION "${LIBRARY_DIRECTORY}/${PROJECT_NAME}/cmake"
    COMPONENT Development)

list(APPEND EXPORT_MODULES
    "cmake/AwsCFlags.cmake"
    "cmake/AwsCheckHeaders.cmake"
    "cmake/AwsSharedLibSetup.cmake"
    "cmake/AwsTestHarness.cmake"
    "cmake/AwsLibFuzzer.cmake"
    "cmake/AwsSanitizers.cmake"
    "cmake/AwsSIMD.cmake"
    "cmake/AwsFindPackage.cmake"
    )

install(FILES ${EXPORT_MODULES}
        DESTINATION "${LIBRARY_DIRECTORY}/cmake"
        COMPONENT Development)

# This should come last, to ensure all variables defined by cmake will be available for export
configure_file(${CONFIG_HEADER_TEMPLATE}
        ${GENERATED_CONFIG_HEADER}
        ESCAPE_QUOTES)

if (ALLOW_CROSS_COMPILED_TESTS OR NOT CMAKE_CROSSCOMPILING)
    if (BUILD_TESTING)
        add_subdirectory(tests)
    endif()
endif()

include(CPackConfig)
