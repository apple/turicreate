# Copyright (c) 2020, Apple Inc. All rights reserved.
#
# Use of this source code is governed by a BSD-3-clause license that can be
# found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause


#
# Add custom commands and targets to build a proto file.
#
# Parameters
# - proto_fn The base name of the proto file to build.
# - target_suffix A string to append to all target names.
#
# Environment
# - Consults the variable/option OVERWRITE_PB_SOURCE to determine whether
#   to regenerate in-source.
#
# Side Effects
#  - If OVERWRITE_PB_SOURCE, targets created to generate in-source are appended
#    to the list proto_depends in PARENT_SCOPE.
#
function(coreml_add_build_proto proto_fn target_suffix)
    add_custom_command(
        OUTPUT
            ${CMAKE_CURRENT_BINARY_DIR}/format/${proto_fn}.pb.cc
            ${CMAKE_CURRENT_BINARY_DIR}/format/${proto_fn}.pb.h
        COMMENT "Generating c++ sources from ${proto_fn}.proto into ${CMAKE_CURRENT_BINARY_DIR}/format/"
        COMMAND ${CMAKE_BINARY_DIR}/deps/protobuf/cmake/protoc
            --cpp_out=${CMAKE_CURRENT_BINARY_DIR}/format/
            -I${CMAKE_CURRENT_SOURCE_DIR}/format
            ${CMAKE_CURRENT_SOURCE_DIR}/format/${proto_fn}.proto
        DEPENDS protoc
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    )
    add_custom_command(
        OUTPUT
            ${CMAKE_CURRENT_BINARY_DIR}/format/${proto_fn}_enum.h
        COMMENT "Generating c++ enums from ${proto_fn}.proto into ${CMAKE_CURRENT_BINARY_DIR}/format/"
        COMMAND ${CMAKE_BINARY_DIR}/deps/protobuf/cmake/protoc
            --plugin=protoc-gen-enum=mlmodel${target_suffix}/enumgen
            --enum_out=${CMAKE_CURRENT_BINARY_DIR}/format/
            -I${CMAKE_CURRENT_SOURCE_DIR}/format/
            ${CMAKE_CURRENT_SOURCE_DIR}/format/${proto_fn}.proto
        DEPENDS enumgen protoc
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    )
    add_custom_command(
        OUTPUT
            ${CMAKE_BINARY_DIR}/coremltools${target_suffix}/proto/${proto_fn}_pb2.py
        COMMENT "Generating Python sources from ${proto_fn}.proto into ${CMAKE_BINARY_DIR}/coremltools${target_suffix}/proto/"
        COMMAND ${CMAKE_BINARY_DIR}/deps/protobuf/cmake/protoc
            --python_out=${CMAKE_BINARY_DIR}/coremltools${target_suffix}/proto
            -I${CMAKE_CURRENT_SOURCE_DIR}/format/
            ${CMAKE_CURRENT_SOURCE_DIR}/format/${proto_fn}.proto
        COMMAND python
            -m lib2to3
            -wn
            --no-diff
            -f import
            ${CMAKE_BINARY_DIR}/coremltools${target_suffix}/${proto_fn}_pb2.py
        DEPENDS protoc
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    )
    # For the CoreML framework we read the source file locations for these, and
    # so it can be useful to update the source tree in addition.  So we repeat
    # all of the above with different outputs.
    if(OVERWRITE_PB_SOURCE)
        add_custom_target(tgt_${proto_fn}_source ALL
            COMMENT "Generating c++ sources from ${proto_fn}.proto into ${CMAKE_CURRENT_SOURCE_DIR}/build/format/"
            COMMAND ${CMAKE_BINARY_DIR}/deps/protobuf/cmake/protoc
                --cpp_out=${CMAKE_CURRENT_SOURCE_DIR}/build/format/
                -I${CMAKE_CURRENT_SOURCE_DIR}/format
                ${CMAKE_CURRENT_SOURCE_DIR}/format/${proto_fn}.proto
            DEPENDS protoc
            WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
        )
        add_custom_target(tgt_${proto_fn}_enums ALL
            COMMENT "Generating c++ enums from ${proto_fn}.proto into ${CMAKE_CURRENT_SOURCE_DIR}/build/format/"
            COMMAND ${CMAKE_BINARY_DIR}/deps/protobuf/cmake/protoc
                --plugin=protoc-gen-enum=mlmodel${target_suffix}/enumgen
                --enum_out=${CMAKE_CURRENT_SOURCE_DIR}/build/format/
                -I${CMAKE_CURRENT_SOURCE_DIR}/format/
                ${CMAKE_CURRENT_SOURCE_DIR}/format/${proto_fn}.proto
            DEPENDS enumgen protoc
            WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
        )
        add_custom_target(tgt_${proto_fn}_python ALL
            COMMENT "Generating Python sources from ${proto_fn}.proto into ${CMAKE_SOURCE_DIR}/coremltools${target_suffix}/proto/"
            COMMAND ${CMAKE_BINARY_DIR}/deps/protobuf/cmake/protoc
                --python_out=${CMAKE_SOURCE_DIR}/coremltools${target_suffix}/proto
                -I${CMAKE_CURRENT_SOURCE_DIR}/format/
                ${CMAKE_CURRENT_SOURCE_DIR}/format/${proto_fn}.proto
            COMMAND python
                -m lib2to3
                -wn
                --no-diff
                -f import
                ${CMAKE_SOURCE_DIR}/coremltools${target_suffix}/proto/${proto_fn}_pb2.py
            DEPENDS protoc
            WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
        )
        # Record dependencies for 'protosrc' target.
        list(APPEND proto_depends tgt_${proto_fn}_source)
        list(APPEND proto_depends tgt_${proto_fn}_enums)
        list(APPEND proto_depends tgt_${proto_fn}_python)
        set(proto_depends ${proto_depends} PARENT_SCOPE)
    endif()
endfunction()