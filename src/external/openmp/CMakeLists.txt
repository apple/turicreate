project(Turi)

message("Building openmp using native build system.")

set(_openmp_dir openmp-src)

set(ON_X86
    ON
    CACHE BOOL "build on x86 architecture")

if(${TC_BUILD_IOS})
  set(ON_X86 OFF)
  foreach(variable ${CMAKE_OSX_ARCHITECTURES})
    if(variable STREQUAL "arm64")
      set(LIBOMP_ARCH aarch64)
      break()
    endif() # let OMP decide it self
  endforeach(variable)
  check_cxx_compiler_flag(-no-integrated-as COMPILER_SUPPORTS_NO_ASM)
endif()

if(COMPILER_SUPPORTS_NO_ASM AND ${TC_BUILD_IOS})
  set(CMAKE_C_FLAGS "-integrated-as ${CMAKE_C_FLAGS}")
  set(CMAKE_CXX_FLAGS "-integrated-as ${CMAKE_CXX_FLAGS}")
endif()

# custom settings
set(OPENMP_STANDALONE_BUILD
    TRUE
    CACHE BOOL "not build within llvm source tree")
set(LIBOMP_ENABLE_SHARED
    OFF
    CACHE BOOL "open static/dynamic link")
set(LIBOMP_USE_ADAPTIVE_LOCKS
    ${ON_X86}
    CACHE BOOL "enable only for x86")
set(OPENMP_ENABLE_LIBOMPTARGET
    OFF
    CACHE BOOL "no libomptarget")
set(LIBOMP_COPY_EXPORTS
    FALSE
    CACHE STRING "build in place")

# for <crt_extern.h>
include_directories(${CMAKE_CURRENT_SOURCE_DIR})

# build openmp
add_subdirectory(${_openmp_dir})

set(TURI_OMP_H_PATH
    ${CMAKE_CURRENT_BINARY_DIR}/${_openmp_dir}/runtime/src
    CACHE STRING "build time generated omp.h path")

target_include_directories(omp INTERFACE ${TURI_OMP_H_PATH})

set_target_properties(omp PROPERTIES TURI_OMP_H_PATH ${TURI_OMP_H_PATH})
