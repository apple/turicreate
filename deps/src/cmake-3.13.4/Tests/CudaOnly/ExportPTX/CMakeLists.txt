cmake_minimum_required(VERSION 3.8)
project (ExportPTX CUDA)

#Goal for this example:
# How to generate PTX files instead of OBJECT files
# How to reference PTX files for custom commands
# How to install PTX files

add_library(CudaPTX OBJECT kernelA.cu kernelB.cu)
set_property(TARGET CudaPTX PROPERTY CUDA_PTX_COMPILATION ON)

#Test ObjectFiles with file(GENERATE)
file(GENERATE
     OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/gen_$<LOWER_CASE:$<CONFIG>/>path_to_objs.h
     CONTENT [[

#include <vector>
#include <string>

#ifndef path_to_objs
#define path_to_objs

static std::string ptx_paths = "$<TARGET_OBJECTS:CudaPTX>";

#endif

]]
)
#We are going to need a wrapper around bin2c for multiple reasons
# 1. bin2c only converts a single file at a time
# 2. bin2c has only standard out support, so we have to manually
# redirect to a cmake buffer
# 3. We want to pack everything into a single output file, so we
# need to also pass the --name option
set(output_file ${CMAKE_CURRENT_BINARY_DIR}/embedded_objs.h)

get_filename_component(cuda_compiler_bin "${CMAKE_CUDA_COMPILER}" DIRECTORY)
find_program(bin_to_c
  NAMES bin2c
  PATHS ${cuda_compiler_bin}
  )
if(NOT bin_to_c)
  message(FATAL_ERROR
    "bin2c not found:\n"
    "  CMAKE_CUDA_COMPILER='${CMAKE_CUDA_COMPILER}'\n"
    "  cuda_compiler_bin='${cuda_compiler_bin}'\n"
    )
endif()

add_custom_command(
  OUTPUT "${output_file}"
  COMMAND ${CMAKE_COMMAND}
    "-DBIN_TO_C_COMMAND=${bin_to_c}"
    "-DOBJECTS=$<TARGET_OBJECTS:CudaPTX>"
    "-DOUTPUT=${output_file}"
    -P ${CMAKE_CURRENT_SOURCE_DIR}/bin2c_wrapper.cmake
  VERBATIM
  DEPENDS $<TARGET_OBJECTS:CudaPTX>
  COMMENT "Converting Object files to a C header"
  )

add_executable(CudaOnlyExportPTX main.cu ${output_file})
add_dependencies(CudaOnlyExportPTX CudaPTX)
target_include_directories(CudaOnlyExportPTX PRIVATE
                           ${CMAKE_CURRENT_BINARY_DIR} )
target_compile_definitions(CudaOnlyExportPTX PRIVATE
                           "CONFIG_TYPE=gen_$<LOWER_CASE:$<CONFIG>>")

if(APPLE)
  # Help the static cuda runtime find the driver (libcuda.dyllib) at runtime.
  set_property(TARGET CudaOnlyExportPTX PROPERTY BUILD_RPATH ${CMAKE_CUDA_IMPLICIT_LINK_DIRECTORIES})
endif()

#Verify that we can install object targets properly
install(TARGETS CudaPTX CudaOnlyExportPTX
   EXPORT cudaPTX
   RUNTIME DESTINATION bin
   LIBRARY DESTINATION lib
   OBJECTS DESTINATION objs
   )
install(EXPORT cudaPTX DESTINATION lib/cudaPTX)
