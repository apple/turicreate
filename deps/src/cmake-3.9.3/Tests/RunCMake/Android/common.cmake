enable_language(C)
enable_language(CXX)

if(NOT ANDROID)
  message(SEND_ERROR "CMake variable 'ANDROID' is not set to a true value.")
endif()

foreach(f
    "${CMAKE_C_ANDROID_TOOLCHAIN_PREFIX}gcc${CMAKE_C_ANDROID_TOOLCHAIN_SUFFIX}"
    "${CMAKE_CXX_ANDROID_TOOLCHAIN_PREFIX}g++${CMAKE_CXX_ANDROID_TOOLCHAIN_SUFFIX}"
    "${CMAKE_CXX_ANDROID_TOOLCHAIN_PREFIX}cpp${CMAKE_CXX_ANDROID_TOOLCHAIN_SUFFIX}"
    "${CMAKE_CXX_ANDROID_TOOLCHAIN_PREFIX}ar${CMAKE_CXX_ANDROID_TOOLCHAIN_SUFFIX}"
    "${CMAKE_CXX_ANDROID_TOOLCHAIN_PREFIX}ld${CMAKE_CXX_ANDROID_TOOLCHAIN_SUFFIX}"
    )
  if(NOT EXISTS "${f}")
    message(SEND_ERROR "Expected file does not exist:\n \"${f}\"")
  endif()
endforeach()

string(APPEND CMAKE_C_FLAGS " -Werror -Wno-attributes")
string(APPEND CMAKE_CXX_FLAGS " -Werror -Wno-attributes")
string(APPEND CMAKE_EXE_LINKER_FLAGS " -Wl,-no-undefined")

if(CMAKE_ANDROID_NDK)
  if(NOT CMAKE_ANDROID_NDK_TOOLCHAIN_VERSION)
    message(SEND_ERROR "CMAKE_ANDROID_NDK_TOOLCHAIN_VERSION is not set!")
  elseif(CMAKE_ANDROID_NDK_TOOLCHAIN_VERSION MATCHES "^clang")
    add_definitions(-DCOMPILER_IS_CLANG)
  elseif(NOT "${CMAKE_C_COMPILER}" MATCHES "toolchains/[^/]+-${CMAKE_ANDROID_NDK_TOOLCHAIN_VERSION}/prebuilt")
    message(SEND_ERROR "CMAKE_ANDROID_NDK_TOOLCHAIN_VERSION is\n"
      "  ${CMAKE_ANDROID_NDK_TOOLCHAIN_VERSION}\n"
      "which does not appear in CMAKE_C_COMPILER:\n"
      "  ${CMAKE_C_COMPILER}")
  endif()
  if(NOT CMAKE_ANDROID_NDK_TOOLCHAIN_HOST_TAG)
    message(SEND_ERROR "CMAKE_ANDROID_NDK_TOOLCHAIN_HOST_TAG is not set!")
  elseif(NOT "${CMAKE_C_COMPILER}" MATCHES "prebuilt/${CMAKE_ANDROID_NDK_TOOLCHAIN_HOST_TAG}/bin")
    message(SEND_ERROR "CMAKE_ANDROID_NDK_TOOLCHAIN_HOST_TAG is\n"
      "  ${CMAKE_ANDROID_NDK_TOOLCHAIN_HOST_TAG}\n"
      "which does not appear in CMAKE_C_COMPILER:\n"
      "  ${CMAKE_C_COMPILER}")
  endif()
elseif(CMAKE_ANDROID_STANDALONE_TOOLCHAIN)
  execute_process(
    COMMAND ${CMAKE_ANDROID_STANDALONE_TOOLCHAIN}/bin/clang --version
    OUTPUT_VARIABLE _out
    ERROR_VARIABLE _err
    RESULT_VARIABLE _res
    )
  if(_res EQUAL 0)
    add_definitions(-DCOMPILER_IS_CLANG)
  endif()
endif()

execute_process(
  COMMAND "${CMAKE_C_ANDROID_TOOLCHAIN_PREFIX}gcc${CMAKE_C_ANDROID_TOOLCHAIN_SUFFIX}" -dumpmachine
  OUTPUT_VARIABLE _out OUTPUT_STRIP_TRAILING_WHITESPACE
  ERROR_VARIABLE _err
  RESULT_VARIABLE _res
  )
if(NOT _res EQUAL 0)
  message(SEND_ERROR "Failed to run 'gcc -dumpmachine':\n ${_res}")
endif()
if(NOT _out STREQUAL "${CMAKE_C_ANDROID_TOOLCHAIN_MACHINE}")
  message(SEND_ERROR "'gcc -dumpmachine' produced:\n"
    " ${_out}\n"
    "which is not equal to CMAKE_C_ANDROID_TOOLCHAIN_MACHINE:\n"
    " ${CMAKE_C_ANDROID_TOOLCHAIN_MACHINE}"
    )
endif()

if(CMAKE_ANDROID_STL_TYPE STREQUAL "none")
  add_definitions(-DSTL_NONE)
elseif(CMAKE_ANDROID_STL_TYPE STREQUAL "system")
  add_definitions(-DSTL_SYSTEM)
elseif(CMAKE_ANDROID_STL_TYPE MATCHES [[^gabi\+\+]])
  add_definitions(-DSTL_GABI)
elseif(CMAKE_ANDROID_STL_TYPE MATCHES [[^stlport]])
  add_definitions(-DSTL_STLPORT)
endif()

string(REPLACE "-" "_" abi "${CMAKE_ANDROID_ARCH_ABI}")
add_definitions(-DABI_${abi})
add_definitions(-DAPI_LEVEL=${CMAKE_SYSTEM_VERSION})
if(CMAKE_ANDROID_ARCH_ABI MATCHES "^armeabi")
  add_definitions(-DARM_MODE=${CMAKE_ANDROID_ARM_MODE})
  message(STATUS "CMAKE_ANDROID_ARM_MODE=${CMAKE_ANDROID_ARM_MODE}")
endif()
if(CMAKE_ANDROID_ARCH_ABI STREQUAL "armeabi-v7a")
  add_definitions(-DARM_NEON=${CMAKE_ANDROID_ARM_NEON})
  message(STATUS "CMAKE_ANDROID_ARM_NEON=${CMAKE_ANDROID_ARM_NEON}")
endif()
add_executable(android_c android.c)
add_executable(android_cxx android.cxx)

# Test that an explicit /usr/include is ignored in favor of
# appearing as a standard include directory at the end.
set(sysinc_dirs)
if(CMAKE_ANDROID_NDK)
  if(NOT CMAKE_ANDROID_NDK_DEPRECATED_HEADERS)
    list(APPEND sysinc_dirs ${CMAKE_SYSROOT_COMPILE}/usr/include)
  else()
    list(APPEND sysinc_dirs ${CMAKE_SYSROOT}/usr/include)
  endif()
endif()
list(APPEND sysinc_dirs ${CMAKE_CURRENT_SOURCE_DIR}/sysinc)
add_executable(android_sysinc_c android_sysinc.c)
target_include_directories(android_sysinc_c SYSTEM PRIVATE ${sysinc_dirs})
add_executable(android_sysinc_cxx android_sysinc.cxx)
target_include_directories(android_sysinc_cxx SYSTEM PRIVATE ${sysinc_dirs})
