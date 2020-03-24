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

set(LIBRARY_DIRECTORY lib)
set(RUNTIME_DIRECTORY bin)
# Set the default lib installation path on GNU systems with GNUInstallDirs
if (UNIX AND NOT APPLE)
    include(GNUInstallDirs)
    set(LIBRARY_DIRECTORY ${CMAKE_INSTALL_LIBDIR})
    set(RUNTIME_DIRECTORY ${CMAKE_INSTALL_BINDIR})

    # this is the absolute dumbest thing in the world, but find_package won't work without it
    # also I verified this is correctly NOT "lib64" when CMAKE_C_FLAGS includes "-m32"
    if (${LIBRARY_DIRECTORY} STREQUAL "lib64")
        set(FIND_LIBRARY_USE_LIB64_PATHS true)
    endif()
endif()


function(aws_prepare_shared_lib_exports target)
    if (BUILD_SHARED_LIBS)
        install(TARGETS ${target}
                EXPORT ${target}-targets
                ARCHIVE
                DESTINATION ${LIBRARY_DIRECTORY}
                COMPONENT Development
                LIBRARY
                DESTINATION ${LIBRARY_DIRECTORY}
                NAMELINK_SKIP
                COMPONENT Runtime
                RUNTIME
                DESTINATION ${RUNTIME_DIRECTORY}
                COMPONENT Runtime)
        install(TARGETS ${target}
                EXPORT ${target}-targets
                LIBRARY
                DESTINATION ${LIBRARY_DIRECTORY}
                NAMELINK_ONLY
                COMPONENT Development)
    else()
        install(TARGETS ${target}
                EXPORT ${target}-targets
                ARCHIVE DESTINATION ${LIBRARY_DIRECTORY}
                COMPONENT Development)
    endif()
endfunction()

function(aws_prepare_symbol_visibility_args target lib_prefix)
    if (BUILD_SHARED_LIBS)
        target_compile_definitions(${target} PUBLIC "-D${lib_prefix}_USE_IMPORT_EXPORT")
        target_compile_definitions(${target} PRIVATE "-D${lib_prefix}_EXPORTS")
    endif()
endfunction()
