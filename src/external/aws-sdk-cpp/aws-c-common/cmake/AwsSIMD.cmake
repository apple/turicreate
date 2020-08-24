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

if (MSVC)
    check_c_compiler_flag("/arch:AVX2" HAVE_M_AVX2_FLAG)
    if (HAVE_M_AVX2_FLAG)
        set(AVX2_CFLAGS "/arch:AVX2")
    endif()
else()
    check_c_compiler_flag(-mavx2 HAVE_M_AVX2_FLAG)
    if (HAVE_M_AVX2_FLAG)
        set(AVX2_CFLAGS "-mavx -mavx2")
    endif()
endif()


set(old_flags "${CMAKE_REQUIRED_FLAGS}")
set(CMAKE_REQUIRED_FLAGS "${CMAKE_REQUIRED_FLAGS} ${AVX2_CFLAGS}")

check_c_source_compiles("
#include <immintrin.h>
#include <emmintrin.h>
#include <string.h>

int main() {
    __m256i vec;
    memset(&vec, 0, sizeof(vec));

    _mm256_shuffle_epi8(vec, vec);
    _mm256_set_epi32(1,2,3,4,5,6,7,8);
    _mm256_permutevar8x32_epi32(vec, vec);

    return 0;
}"  HAVE_AVX2_INTRINSICS)

check_c_source_compiles("
#include <immintrin.h>
#include <string.h>

int main() {
    __m256i vec;
    memset(&vec, 0, sizeof(vec));
    return (int)_mm256_extract_epi64(vec, 2);
}" HAVE_MM256_EXTRACT_EPI64)

check_c_source_compiles("
#include <immintrin.h>

int main() {
    return _may_i_use_cpu_feature(_FEATURE_AVX2 | _FEATURE_SSE4_1);
}
"   HAVE_MAY_I_USE)

check_c_source_compiles("
#include <immintrin.h>

int main() {
    return __builtin_cpu_supports(\"avx2\");
}
" HAVE_BUILTIN_CPU_SUPPORTS)

check_c_source_compiles("
#include <intrin.h>

int main() {
    int cpuInfo[4] = {0};
    int function_id = 1;
    int subfunction_id = 1;
    __cpuidex(cpuInfo, function_id, subfunction_id);
    return 0;
}" HAVE_MSVC_CPUIDEX)

set(CMAKE_REQUIRED_FLAGS "${old_flags}")

macro(simd_add_definition_if target definition)
    if(${definition})
        target_compile_definitions(${target} PRIVATE -D${definition})
    endif(${definition})
endmacro(simd_add_definition_if)

# Configure private preprocessor definitions for SIMD-related features
# Does not set any processor feature codegen flags
function(simd_add_definitions target)
    simd_add_definition_if(${target} HAVE_AVX2_INTRINSICS)
    simd_add_definition_if(${target} HAVE_MAY_I_USE)
    simd_add_definition_if(${target} HAVE_BUILTIN_CPU_SUPPORTS)
    simd_add_definition_if(${target} HAVE_MSVC_CPUIDEX)
    simd_add_definition_if(${target} HAVE_MM256_EXTRACT_EPI64)
endfunction(simd_add_definitions)

# Adds source files only if AVX2 is supported. These files will be built with
# avx2 intrinsics enabled.
# Usage: simd_add_source_avx2(target file1.c file2.c ...)
function(simd_add_source_avx2 target)
    foreach(file ${ARGN})
        target_sources(${target} PRIVATE ${file})
        set_source_files_properties(${file} PROPERTIES COMPILE_FLAGS "${AVX2_CFLAGS}")
    endforeach()
endfunction(simd_add_source_avx2)
