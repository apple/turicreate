
cmake_minimum_required(VERSION 3.7)
project (WithDefs CUDA)

#verify that we can pass explicit cuda arch flags
string(APPEND CMAKE_CUDA_FLAGS " -gencode arch=compute_30,code=compute_30")
if(CMAKE_CUDA_COMPILER_VERSION VERSION_GREATER_EQUAL 9)
  set(debug_compile_flags --generate-code arch=compute_32,code=sm_32)
else()
  set(debug_compile_flags --generate-code arch=compute_20,code=sm_20)
endif()
if(CMAKE_CUDA_SIMULATE_ID STREQUAL "MSVC")
  list(APPEND debug_compile_flags -Xcompiler=-WX)
else()
  list(APPEND debug_compile_flags -Xcompiler=-Werror)
endif()
set(release_compile_defs DEFREL)

#Goal for this example:
#build a executable that needs to be passed a complex define through add_defintions
#this verifies we can pass things such as '_','(' to nvcc
add_definitions("-DPACKED_DEFINE=__attribute__((packed))")

add_executable(CudaOnlyWithDefs main.notcu)
set_source_files_properties(main.notcu PROPERTIES LANGUAGE CUDA)

target_compile_options(CudaOnlyWithDefs
  PRIVATE
    -DFLAG_COMPILE_LANG_$<COMPILE_LANGUAGE>
    -DFLAG_LANG_IS_CUDA=$<COMPILE_LANGUAGE:CUDA>
    -Xcompiler=-DHOST_DEFINE
    $<$<CONFIG:DEBUG>:$<BUILD_INTERFACE:${debug_compile_flags}>>
  )

target_compile_definitions(CudaOnlyWithDefs
  PRIVATE
    $<$<CONFIG:RELEASE>:$<BUILD_INTERFACE:${release_compile_defs}>>
    -DDEF_COMPILE_LANG_$<COMPILE_LANGUAGE>
    -DDEF_LANG_IS_CUDA=$<COMPILE_LANGUAGE:CUDA>
  )

target_include_directories(CudaOnlyWithDefs
  PRIVATE
    $<$<COMPILE_LANGUAGE:CUDA>:${CMAKE_CURRENT_SOURCE_DIR}/inc_cuda>
)

if(APPLE)
  # Help the static cuda runtime find the driver (libcuda.dyllib) at runtime.
  set_property(TARGET CudaOnlyWithDefs PROPERTY BUILD_RPATH ${CMAKE_CUDA_IMPLICIT_LINK_DIRECTORIES})
endif()
