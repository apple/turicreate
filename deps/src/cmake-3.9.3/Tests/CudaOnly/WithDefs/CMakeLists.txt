
cmake_minimum_required(VERSION 3.7)
project (CudaOnlyWithDefs CUDA)

#verify that we can pass explicit cuda arch flags
string(APPEND CMAKE_CUDA_FLAGS " -gencode arch=compute_30,code=compute_30")
set(debug_compile_flags --generate-code arch=compute_20,code=sm_20)
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

if(CMAKE_GENERATOR MATCHES "Visual Studio")
  # CUDA MSBuild rules do not pass '-x cu' to nvcc
  set(main main_for_vs.cu)
else()
  set(main main.notcu)
  set_source_files_properties(main.notcu PROPERTIES LANGUAGE CUDA)
endif()
add_executable(CudaOnlyWithDefs ${main})

target_compile_options(CudaOnlyWithDefs
  PRIVATE
    -Xcompiler=-DHOST_DEFINE
    $<$<CONFIG:DEBUG>:$<BUILD_INTERFACE:${debug_compile_flags}>>
  )

target_compile_definitions(CudaOnlyWithDefs
  PRIVATE
    $<$<CONFIG:RELEASE>:$<BUILD_INTERFACE:${release_compile_defs}>>
  )

#we need to add an rpath for the cuda library so that everything
#loads properly on the mac
if(CMAKE_SYSTEM_NAME MATCHES "Darwin")
  set_target_properties(CudaOnlyWithDefs PROPERTIES LINK_FLAGS "-Wl,-rpath,${CMAKE_CUDA_IMPLICIT_LINK_DIRECTORIES}")
endif()
