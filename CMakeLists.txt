project(Turi)

# We require a recent version of cmake and automatically
# install a compatible version when using the cmake lists,
# if one is not already present.
cmake_minimum_required(VERSION 3.12.0)

# Libraries linked via full path no longer produce linker search paths.
cmake_policy(SET CMP0003 NEW)
# Preprocessor definition values are now escaped automatically.
cmake_policy(SET CMP0005 NEW)
# for cmake 3.0
cmake_policy(SET CMP0045 OLD)
cmake_policy(SET CMP0046 OLD)
cmake_policy(SET CMP0042 NEW)

# Generate a compilation database for use with automated tools like IDE/editor
# plugins. See http://clang.llvm.org/docs/JSONCompilationDatabase.html
set( CMAKE_EXPORT_COMPILE_COMMANDS ON )

# Determine where additional Turi specific cmake modules are
# defined
set(DEPS_CMAKE ${CMAKE_SOURCE_DIR}/deps/cmake)
set(CMAKE_MODULE_PATH ${DEPS_CMAKE};${CMAKE_SOURCE_DIR}/cmake)
set(CMAKE_PREFIX_PATH ${CMAKE_SOURCE_DIR}/deps/local)
set(ENV{PATH} "${CMAKE_SOURCE_DIR}/deps/local/bin:${CMAKE_SOURCE_DIR}/deps/env/bin:$ENV{PATH}")
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

if(${TC_BUILD_PYTHON})
  message("Building python libraries.")

  if(NOT EXISTS ${CMAKE_SOURCE_DIR}/deps/env/)
    message(FATAL, "Internal error: Python toolchain expected; not present.")
  endif()

  include(UseCython)
  add_definitions(-DTC_HAS_PYTHON)
  add_definitions(-DTC_BUILD_VISUALIZATION_CLIENT)
else()
  message("Skipping python libraries.")
endif()

set(TC_PLATFORM_APPLE false)
set(TC_PLATFORM_LINUX false)
set(TC_PLATFORM_WIN32 false)

if (APPLE)
  set(TC_PLATFORM_APPLE true)
  add_definitions(-DTC_PLATFORM_APPLE)
  find_library(ACCELERATE NAMES Accelerate)
  find_library(APPKIT AppKit)
  find_library(CORE_GRAPHICS CoreGraphics)
  find_library(JAVASCRIPT_CORE JavaScriptCore)
  find_library(FOUNDATION Foundation)
  find_library(CORE_IMAGE NAMES CoreImage)
  find_library(CORE_ML CoreML)
  find_library(CORE_VIDEO CoreVideo)
  find_library(METAL NAMES Metal)
  find_library(METAL_PERFORMANCE_SHADERS NAMES MetalPerformanceShaders)
elseif (WIN32)
  set(TC_PLATFORM_WIN32 true)
  add_definitions(-DTC_PLATFORM_WIN32)
else()
  set(TC_PLATFORM_LINUX true)
  add_definitions(-DTC_PLATFORM_LINUX)
endif()


include(CheckLibraryExists)
include(CheckFunctionExists)
include(ExternalProject)
include(CheckCXXSourceCompiles)
include(CheckCXXCompilerFlag)

if (${TC_HAS_LLVM})
  find_package(LLVM REQUIRED PATHS ${TC_LLVM_PATH}/lib/cmake)

  if (${LLVM_VERSION_MAJOR} EQUAL 6 AND ${LLVM_VERSION_MINOR} EQUAL 0)
    message("Found LLVM.")
    add_definitions(-DTC_HAS_LLVM)
    FOREACH(DIR ${LLVM_INCLUDE_DIRS})
      include_directories(SYSTEM ${DIR})
    ENDFOREACH()
    link_directories("${TC_LLVM_PATH}/lib")
  else()
    message("Found LLVM, but only version 6.0 is currently supported; skipping.")
    set(TC_HAS_LLVM false)
  endif()
else()
  message("LLVM not provided; skipping.")
  set(TC_HAS_LLVM false)
endif()

set(TC_HAS_LIBUNWIND false)
set(TC_HAS_LIBUNWIND_STANDALONE false)

if (TC_PLATFORM_APPLE)
  if (TC_HAS_LLVM)
    message("Using libunwind from LLVM.")
    set(TC_HAS_LIBUNWIND true)
    set(TC_HAS_LIBUNWIND_STANDALONE false)
    add_definitions(-DTC_HAS_LIBUNWIND)
  else()
    set(TC_HAS_LIBUNWIND false)
    set(TC_HAS_LIBUNWIND_STANDALONE false)
  endif()
elseif (TC_PLATFORM_LINUX)
  find_library(TC_LIBUNWIND unwind)
  if (${TC_LIBUNWIND} STREQUAL "TC_LIBUNWIND-NOTFOUND")
    message("Did not find libunwind; skipping.")
    set(TC_HAS_LIBUNWIND false)
  else()
    message("Found libunwind.")
    set(TC_HAS_LIBUNWIND true)
    set(TC_HAS_LIBUNWIND_STANDALONE true)
    add_definitions(-DTC_HAS_LIBUNWIND)
  endif()
endif()

if (${TC_STACK_DISPLAY})
  if (${TC_HAS_LIBUNWIND} AND ${TC_HAS_LLVM})
    message("Enabling display of annotated stack traces.")
    add_definitions(-DTC_STACK_DISPLAY)
  else()
    set(TC_STACK_DISPLAY false)
  endif()
else()
  message("Disabling display of annotated stack traces.")
endif()



#**************************************************************************/
#*                                                                        */
#*                     Global Link and Include Flags                      */
#*                                                                        */
#**************************************************************************/
include_directories(SYSTEM
 ${CMAKE_SOURCE_DIR}/deps/local/include)

link_directories(${CMAKE_SOURCE_DIR}/deps/local/lib)

if (EXISTS ${CMAKE_SOURCE_DIR}/deps/local/lib64)
  link_directories(${CMAKE_SOURCE_DIR}/deps/local/lib64)
endif()

#**************************************************************************
#*                                                                        *
#*                       Platform Specific Stuff                          *
#*                                                                        *
#**************************************************************************
if(APPLE)
  if(TC_BUILD_IOS)
    SET(TC_BASE_SDK iphoneos)
    SET(COMPILER_FLAGS "${COMPILER_FLAGS} -miphoneos-version-min=10.0 -Wno-c++11-narrowing")
  else()
    SET(TC_BASE_SDK macosx)

    # Modify COMPILER_FLAGS instead of setting CMAKE_OSX_DEPLOYMENT_TARGET so
    # that this setting propagates to external dependencies.
    SET(COMPILER_FLAGS "${COMPILER_FLAGS} -mmacosx-version-min=10.12")
  endif()

  EXEC_PROGRAM(xcrun ARGS --sdk ${TC_BASE_SDK} --show-sdk-version OUTPUT_VARIABLE TC_BASE_SDK_VERSION RETURN_VALUE _xcrun_ret)
  if(NOT ${_xcrun_ret} EQUAL 0)
    message(ERROR, "xcrun command failed with return code ${_xcrun_ret}.")
  endif()

  EXEC_PROGRAM(xcrun ARGS --sdk ${TC_BASE_SDK} --show-sdk-path OUTPUT_VARIABLE TC_BASE_SDK_PATH RETURN_VALUE _xcrun_ret)
  if(NOT ${_xcrun_ret} EQUAL 0)
    message(ERROR, "xcrun command failed with return code ${_xcrun_ret}.")
  endif()

  SET(CMAKE_OSX_SYSROOT "${TC_BASE_SDK_PATH}")
  SET(COMPILER_FLAGS "${COMPILER_FLAGS} -isysroot ${TC_BASE_SDK_PATH}")

  # Core ML is only present on macOS 10.13 or higher.
  # Logic reversed to get around what seems to be a CMake bug.
  if(NOT TC_BASE_SDK_VERSION VERSION_LESS 10.13)
    add_definitions(-DHAS_CORE_ML)
    set(HAS_CORE_ML TRUE)
  endif()

  # Core ML only supports batch inference on macOS 10.14 or higher
  # Logic reversed to get around what seems to be a CMake bug.
  if(NOT TC_BASE_SDK_VERSION VERSION_LESS 10.14)
    add_definitions(-DHAS_CORE_ML_BATCH_INFERENCE)

    # GPU-accelerated training with MPS backend requires macOS 10.14 or higher
    add_definitions(-DHAS_MPS)
    set(HAS_MPS TRUE)

    # CoreML MLCustomModel requires macOS 10.14 or higher
    set(HAS_MLCUSTOMMODEL TRUE)
  endif()
endif()


if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
  set(CLANG false)
elseif(WIN32)
# for whatever reason on windows the compiler identification is an empty string
  set(CLANG false)
else()
  set(CLANG true)
endif()

if(CMAKE_GENERATOR MATCHES "MinGW Makefiles")
  SET(MINGW_MAKEFILES true)
else()
  SET(MINGW_MAKEFILES false)
endif()

if(CMAKE_GENERATOR MATCHES "MSYS Makefiles")
  SET(MSYS_MAKEFILES true)
else()
  SET(MSYS_MAKEFILES false)
endif()


if(WIN32 AND MINGW)
  SET(COMPILER_FLAGS "${COMPILER_FLAGS} -Wa,-mbig-obj")
endif()

set(MINGW_ROOT "/mingw64/bin")
# Separate variable so that cython's CMakeLists.txt can use it too
if (WIN32)
        set(INSTALLATION_SYSTEM_BINARY_FILES
        ${CMAKE_SOURCE_DIR}/deps/local/bin/libsodium-13.dll
        ${MINGW_ROOT}/libiconv-2.dll
        ${MINGW_ROOT}/libssh2-1.dll
        ${MINGW_ROOT}/zlib1.dll
        ${MINGW_ROOT}/libwinpthread-1.dll
        ${MINGW_ROOT}/libgcc_s_seh-1.dll
        ${MINGW_ROOT}/libstdc++-6.dll
        ${MINGW_ROOT}/libeay32.dll)
else()
        if (EXISTS ${CMAKE_SOURCE_DIR}/deps/local/lib64/libgomp.so.1)
                list(APPEND INSTALLATION_SYSTEM_BINARY_FILES ${CMAKE_SOURCE_DIR}/deps/local/lib64/libgomp.so.1)
        endif()
endif()


#**************************************************************************/
#*                                                                        */
#*                             Common Defines                             */
#*                                                                        */
#**************************************************************************/
set(CMAKE_CXX_STANDARD 11)


if(WIN32)
  add_definitions(-DWINVER=0x0600)
  add_definitions(-D_WIN32_WINNT=0x0600)
endif()

if(TC_BUILD_IOS)
  add_definitions(-DTC_BUILD_IOS)
  add_definitions(-DTC_NO_CURL)
  set(TC_ENABLE_REMOTEFS 0)
endif()

if(${TC_ENABLE_REMOTEFS})
  add_definitions(-DTC_ENABLE_REMOTEFS)
  add_definitions(-DCURL_STATICLIB)
else()
  add_definitions(-DTC_NO_CURL)
endif()

if (${TC_EXTRA_OPTIMIZATION_FLAGS})
  message("Enabling extra optimization flags.")
  add_definitions(-DTC_EXTRA_OPTIMIZATION_FLAGS)
else()
  message("Disabling extra optimization flags.")
endif()

add_definitions(-DTC_BUILD_PATH_BASE="${CMAKE_SOURCE_DIR}")

add_definitions(-DCURL_STATICLIB)
add_definitions(-DIN_TURI_SOURCE_TREE)
add_definitions(-DFUSION_MAX_VECTOR_SIZE=20)
add_definitions(-DBOOST_SPIRIT_THREAD_SAFE)
# These determine the maximum number of arguments for extension functions
add_definitions(-DBOOST_FUSION_INVOKE_MAX_ARITY=12)
add_definitions(-DBOOST_FUSION_INVOKE_PROCEDURE_MAX_ARITY=12)
add_definitions(-DBOOST_FUSION_INVOKE_FUNCTION_OBJECT_MAX_ARITY=12)

# Workaround for https://svn.boost.org/trac10/ticket/10443 in Boost 1.68.0
add_definitions(-DBOOST_RESULT_OF_USE_TR1_WITH_DECLTYPE_FALLBACK)

#**************************************************************************/
#*                                                                        */
#*                          Apple Legal Defines                           */
#*                                                                        */
#**************************************************************************/
add_definitions(-DEIGEN_MPL2_ONLY)

#**************************************************************************/
#*                                                                        */
#*                      Force Disable OpenMP                              */

if(NOT TC_COREMLTOOLS_DIR) 
  set(TC_COREMLTOOLS_DIR ${CMAKE_SOURCE_DIR}/src/external/coremltools_wrap/coremltools/)
else()
  message("Using mlmodel interface in source directory ${TC_COREMLTOOLS_DIR}/mlmodel.")
endif()

include_directories(${TC_COREMLTOOLS_DIR})

#*                                                                        */
#**************************************************************************/
add_library(openmp INTERFACE)
target_compile_definitions(openmp INTERFACE __NO_OPENMP__)
message(WARNING "OpenMP Libraries were not found")

#**************************************************************************/
#*                                                                        */
#*                      coremltools                                       */
#*                                                                        */
#**************************************************************************/

if(NOT TC_COREMLTOOLS_DIR)
  set(TC_COREMLTOOLS_DIR ${CMAKE_SOURCE_DIR}/src/external/coremltools_wrap/coremltools/)
endif()

message("Using ${TC_COREMLTOOLS_DIR} for coremltools directory.")
include_directories(${TC_COREMLTOOLS_DIR}/)


#**************************************************************************/
#*                                                                        */
#*             Adapt Compiler and Linker Flags to the system              */
#*                                                                        */
#**************************************************************************/

if (CLANG)

  set(_cpp11_optional_flags "")

  check_cxx_compiler_flag("-Wno-stdlibcxx-not-found" __no_stdlibcxx_not_found)
  if(${__no_stdlibcxx_not_found})
    set(_cpp11_optional_flags "${_cpp11_optional_flags} -Wno-stdlibcxx-not-found")
  endif()

  check_cxx_compiler_flag("-Wno-deprecated-register" __no_deprecated_register)
  if(${__no_deprecated_register})
    set(_cpp11_optional_flags "${_cpp11_optional_flags} -Wno-deprecated-register")
  endif()

  # clang on Linux gives unused argument warnings all over the place
  # they look useful, but let's suppress them for now since there are tons
  # (and with warning-as-error, they break the build)
  check_cxx_compiler_flag("-Qunused-arguments" __qunused_arguments)
  if(${__qunused_arguments})
    add_compile_options(-Qunused-arguments)
  endif()
  
  # same goes for unknown warning group (for #pragma clang warning suppression)
  # seems different clang distributions have quite different warning arguments
  check_cxx_compiler_flag("-Wno-unknown-warning-option" __no_unknown_warning_option)
  if(${__no_unknown_warning_option})
    add_compile_options(-Wno-unknown-warning-option)
  endif()
  
  # clang on Linux with libstdc++ from GCC 4.8.1 gives errors about class vs. struct hash.
  # apparently it's defined as both in separate places in libstdc++
  # and GCC 4.8.1 doesn't care, but clang does.
  # Not sure how to tell whether we're using libstdc++ specifically, so for now,
  # we'll just avoid this warning-as-error on non-Apple platforms entirely.
  if(NOT APPLE)
    check_cxx_compiler_flag("-Wno-mismatched-tags" __no_mismatched_tags)
    if (${__no_mismatched_tags})
      set(_cpp11_optional_flags "${_cpp11_optional_flags} -Wno-error=mismatched-tags")
      set(_cpp11_optional_flags "${_cpp11_optional_flags} -Wno-mismatched-tags")
    endif()
  endif()

  set(CPP11_FLAGS "-Wno-enum-compare -Wno-conversion-null -Wno-constant-logical-operand -Wno-parentheses-equality -ftemplate-depth=900 ${_cpp11_optional_flags}")

  set(WERROR_FLAGS "-Werror -Wall -Wsign-compare")
else()
  # non-clang
  set(CPP11_FLAGS "-Wno-enum-compare -Wno-conversion-null -ftemplate-depth=900 -Wno-unknown-pragmas")
  set(WERROR_FLAGS "-Werror")
endif()

if (${TC_STACK_DISPLAY})
  set(DEBUG_INFO_FLAGS "-g3 -fno-omit-frame-pointer")
else()
  set(DEBUG_INFO_FLAGS "-g")
endif()

# Shared compiler flags used by all builds (debug, profile, release)
# Allow COMPILER_FLAGS to be used as options from the ./configure without
# forcing all the other options I want to add to be lost
set(C_REAL_COMPILER_FLAGS "-Wall ${DEBUG_INFO_FLAGS} ${COMPILER_FLAGS}")
set(CPP_REAL_COMPILER_FLAGS "-Wall ${DEBUG_INFO_FLAGS} ${CPP11_FLAGS} ${COMPILER_FLAGS}")

if (CLANG)
  # clang 9.0.0 and earlier will warn (error with -Werror) on unused -pthread during linking
  # so adding only to compile options is what we need.
  add_compile_options(-pthread)
else()
  set(C_REAL_COMPILER_FLAGS "${C_REAL_COMPILER_FLAGS} -pthread")
  set(CPP_REAL_COMPILER_FLAGS "${CPP_REAL_COMPILER_FLAGS} -pthread")
endif()

# Disable address space randomization for OSX lion and above
if (APPLE)
  set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Xlinker -no_pie")
  set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -Xlinker -no_pie")
# if mac, but not using clang, we should use the clang linker anyway since
# the gcc linker seems to cause problems. Especially with -march=native
#
# see:
# http://stackoverflow.com/questions/9840207/how-to-use-avx-pclmulqdq-on-mac-os-x-lion
  if (NOT CLANG)
    set(ALTERNATE_LINKER "-Wa,-q")
  endif()
endif()


# Install time rpath gets highest priority, we should always use rpath relative to the installation location first.
if (WIN32)
  set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,-rpath,${CMAKE_SOURCE_DIR}/deps/local/bin")
  set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,-rpath,/mingw64/bin")
  set(CMAKE_MODULE_LINKER_FLAGS "${CMAKE_MODULE_LINKER_FLAGS} -Wl,-rpath,/mingw64/bin")
elseif (APPLE)
  # For whatever reason just adding -rpath=@loader_path has issues on 10.9 but not 10.10
  # Here are two possible alternatives that "may" fix the issue. If they fix it
  # I am not sure which line was the one which actually worked
  set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,-rpath,@loader_path/.")
  set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,-rpath,.")
  set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -Wl,-rpath,@loader_path -Wl,-rpath,@loader_path/..")
  set(CMAKE_MODULE_LINKER_FLAGS "${CMAKE_MODULE_LINKER_FLAGS} -Wl,-rpath,@loader_path -Wl,-rpath,@loader_path/..")
else()
  # LINUX
  set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,-rpath,$ORIGIN")
  set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -Wl,-rpath,$ORIGIN -Wl,-rpath,$ORIGIN/..")
  set(CMAKE_MODULE_LINKER_FLAGS "${CMAKE_MODULE_LINKER_FLAGS} -Wl,-rpath,$ORIGIN -Wl,-rpath,$ORIGIN/..")
endif()

# if (EXISTS ${CMAKE_SOURCE_DIR}/deps/local/lib64)
  set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,-rpath,${CMAKE_SOURCE_DIR}/deps/local/lib64")
  set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -Wl,-rpath,${CMAKE_SOURCE_DIR}/deps/local/lib64")
  set(CMAKE_MODULE_LINKER_FLAGS "${CMAKE_MODULE_LINKER_FLAGS} -Wl,-rpath,${CMAKE_SOURCE_DIR}/deps/local/lib64")
  # endif()

  # if (EXISTS ${CMAKE_SOURCE_DIR}/deps/local/lib)
  set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,-rpath,${CMAKE_SOURCE_DIR}/deps/local/lib")
  set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -Wl,-rpath,${CMAKE_SOURCE_DIR}/deps/local/lib")
  set(CMAKE_MODULE_LINKER_FLAGS "${CMAKE_MODULE_LINKER_FLAGS} -Wl,-rpath,${CMAKE_SOURCE_DIR}/deps/local/lib")
  # endif()



if (MARCH)
  set(ARCH_FLAG "-march=${MARCH}")
elseif (ARCH)
  set(ARCH_FLAG "-arch ${ARCH}")
else()
  check_cxx_compiler_flag(-march=nehalem HAS_MARCH_NEHALEM)
  if(HAS_MARCH_NEHALEM)
    # Old CPU archetecture; enables SSE up to 4.2 + popcnt + other instructions.
    # Supported on GCC 4.9.x and above
    set(MARCH "nehalem")
  else()
    # Old CPU archetecture; enables SSE up to 3
    # Supported on at least GCC 4.8.x, maybe earlier
    set(MARCH "core2")
    check_cxx_compiler_flag(-msse4 HAS_M_SSE4)
    if(HAS_M_SSE4)
      set(MARCH "${MARCH} -msse4")
    endif()
    check_cxx_compiler_flag(-msse4.1 HAS_M_SSE4_1)
    if(HAS_M_SSE4_1)
      set(MARCH "${MARCH} -msse4.1")
    endif()
    check_cxx_compiler_flag(-msse4.2 HAS_M_SSE4_2)
    if(HAS_M_SSE4_2)
      set(MARCH "${MARCH} -msse4.2")
    endif()
  endif()
  set(ARCH_FLAG "-march=${MARCH}")
endif()

# check if MTUNE is set
if (NOT MTUNE)
  set(MTUNE "-mtune=generic") # Reommended way of setting this for generic distributions
endif()

set(EXTRA_OPTIMIZATION_FLAGS "")
set(DISABLE_WARNING_FLAGS "")
# If profiling mode is enabled then additional profiling flags are set for
# the compiler
if (COMPILE_PROFILING MATCHES 1)
        set(PROFILING_FLAGS "-DUSE_EVENT_LOG -DUSE_TRACEPOINT")
        set(EXTRA_OPTIMIZATION_FLAGS "${EXTRA_OPTIMIZATION_FLAGS} ${PROFILING_FLAGS}")
endif()

# Set up the 128 bit integer support; defines INT128_FLAGS
include(FindInt128)
Find_Int128_Types()


#disable Wno-unused-local-typedefs if available
check_cxx_compiler_flag(-Wno-unused-local-typedefs HAS_WNO_LOCAL_TYPEDEFS)
if(HAS_WNO_LOCAL_TYPEDEFS)
        set(DISABLE_WARNING_FLAGS "${DISABLE_WARNING_FLAGS} -Wno-unused-local-typedefs")
endif()

#disable gcc-only flags on clang (newer clang with XCode 5.1 errors on these)
check_cxx_compiler_flag(-fpeel-loops HAS_FPEEL_LOOPS)
if(HAS_FPEEL_LOOPS)
        set(EXTRA_OPTIMIZATION_FLAGS "${EXTRA_OPTIMIZATION_FLAGS} -fpeel-loops")
endif()
check_cxx_compiler_flag(-funswitch-loops HAS_FUNSWITCH_LOOPS)
if(HAS_FUNSWITCH_LOOPS)
        set(EXTRA_OPTIMIZATION_FLAGS "${EXTRA_OPTIMIZATION_FLAGS} -funswitch-loops")
endif()
check_cxx_compiler_flag(-ftracer HAS_FTRACER)
if(HAS_FTRACER)
        set(EXTRA_OPTIMIZATION_FLAGS "${EXTRA_OPTIMIZATION_FLAGS} -ftracer")
endif()



if(${DEBUG_OPT_FOR_SIZE})
  set(DEBUG_OPTIMIZATION_FLAGS "-Os")
else()
  set(DEBUG_OPTIMIZATION_FLAGS "-O0 -fno-inline")
endif()

if(${RELEASE_OPT_FOR_SIZE})
  set(RELEASE_OPTIMIZATION_FLAGS "-Os -DNDEBUG")
else()
  set(RELEASE_OPTIMIZATION_FLAGS "-O3 -DNDEBUG")
endif()

if(WIN32)
        # on windows due to some string table limitations + template
        # insanities, on -O0 we run out of string table entries or something
        # like that.
        set(DEBUG_OPTIMIZATION_FLAGS -O2)
endif()

# Set the debug flags
set(EXTERNAL_CMAKE_C_FLAGS_DEBUG
  "${DEBUG_OPTIMIZATION_FLAGS} -Wno-attributes ${ARCH_FLAG} -Winit-self  ${PROFILING_FLAGS} ${C_REAL_COMPILER_FLAGS} -fno-inline ${ALTERNATE_LINKER} -DHAVE_PTHREAD=1"
  CACHE STRING "compiler options" FORCE)
set(CMAKE_C_FLAGS_DEBUG
  "${EXTERNAL_CMAKE_C_FLAGS_DEBUG} ${WERROR_FLAGS} -Wuninitialized "
  CACHE STRING "compiler options" FORCE)
set(EXTERNAL_CMAKE_CXX_FLAGS_DEBUG
  "${DEBUG_OPTIMIZATION_FLAGS} ${DISABLE_WARNING_FLAGS} -Wno-attributes ${ARCH_FLAG} -Winit-self ${PROFILING_FLAGS} ${CPP_REAL_COMPILER_FLAGS} ${INT128_FLAGS} ${ALTERNATE_LINKER} -fno-inline -DHAVE_PTHREAD=1"
  CACHE STRING "compiler options" FORCE)
set(CMAKE_CXX_FLAGS_DEBUG
  "${EXTERNAL_CMAKE_CXX_FLAGS_DEBUG} ${WERROR_FLAGS} -Wuninitialized"
  CACHE STRING "compiler options" FORCE)

set(CMAKE_C_FLAGS_RELEASE
  "${RELEASE_OPTIMIZATION_FLAGS} -Wno-attributes ${ARCH_FLAG} ${MTUNE} ${PROFILING_FLAGS} ${C_REAL_COMPILER_FLAGS} ${WERROR_FLAGS} -DNDEBUG ${ALTERNATE_LINKER} -DHAVE_PTHREAD=1"
  CACHE STRING "compiler options" FORCE)
set(CMAKE_CXX_FLAGS_RELEASE
  "${RELEASE_OPTIMIZATION_FLAGS} ${EXTRA_OPTIMIZATION_FLAGS} ${DISABLE_WARNING_FLAGS} ${PROFILING_FLAGS} -Wno-attributes ${ARCH_FLAG} ${MTUNE} ${CPP_REAL_COMPILER_FLAGS} ${WERROR_FLAGS} ${INT128_FLAGS} ${ALTERNATE_LINKER} -DNDEBUG -DHAVE_PTHREAD=1"
  CACHE STRING "compiler options" FORCE)

#**************************************************************************/
#*                                                                        */
#*       Some C++ Implementation Oddities between libc++ and stdc++       */
#*                                                                        */
#**************************************************************************/
set(CMAKE_REQUIRED_FLAGS ${CMAKE_CXX_FLAGS_DEBUG})
check_cxx_source_compiles("#include <ios>
                           #include <system_error>
                           int main(int argc, char** argv) {
                             throw std::ios_base::failure(\"hello\", std::error_code());
                             }" COMPILER_HAS_IOS_BASE_FAILURE_WITH_ERROR_CODE)

if(COMPILER_HAS_IOS_BASE_FAILURE_WITH_ERROR_CODE)
  message(STATUS "Compiler supports ios_base::failure(str, error_code)")
  add_definitions(-DCOMPILER_HAS_IOS_BASE_FAILURE_WITH_ERROR_CODE)
else()
  message(STATUS "Compiler does not support ios_base::failure(str, error_code)")
endif()

check_cxx_source_compiles("#include <ios>
                           #include <system_error>

                           class io_error : public std::ios_base::failure {
                             public:   /* The noexcept here is different between C++ 17 and C++ 11. */
                              const char *what() const noexcept { return \"\"; }
                           };
                           int main(int argc, char** argv) { return 0; }"
                          COMPILER_HAS_NOEXCEPT_WHAT_ON_EXCEPTIONS)

if(COMPILER_HAS_NOEXCEPT_WHAT_ON_EXCEPTIONS)
  message(STATUS "Compiler supports noexcept on what() with ios_base::failure.")
  add_definitions(-DCOMPILER_HAS_NOEXCEPT_WHAT_ON_EXCEPTIONS)
else()
  message(STATUS "Compiler uses throw() on what() with ios_base::failure.")
endif()


#**************************************************************************/
#*                                                                        */
#*                              Final Flags                               */
#*                                                                        */
#**************************************************************************/

set(CMAKE_BUILD_TYPE "${CMAKE_BUILD_TYPE}" CACHE STRING "Build Type")
if (CMAKE_BUILD_TYPE MATCHES "Release")
  message(STATUS "Release build with C++ flags: " ${CMAKE_CXX_FLAGS_RELEASE})
  message(STATUS "Release build with C flags: " ${CMAKE_C_FLAGS_RELEASE})
elseif(CMAKE_BUILD_TYPE MATCHES "Debug")
  message(STATUS "Debug build with C++ flags: " ${CMAKE_CXX_FLAGS_DEBUG})
  message(STATUS "Debug build with C flags: " ${CMAKE_C_FLAGS_DEBUG})
else()
  message(FATAL_ERROR "Unknown build type: " ${CMAKE_BUILD_TYPE} "!. Rerun ./configure")
endif()


#**************************************************************************/
#*                                                                        */
#*         Definitions and options to support dependencies                */
#*                                                                        */
#**************************************************************************/
add_compile_options(-Dgoogle=__tc_google)
add_compile_options(-DCoreML=__tc_CoreML)
add_compile_options(-DHAVE_PTHREAD=1)

##############################################################################
##############################################################################
##############################################################################
##############################################################################
##############################################################################
##############################################################################

# We are done with the system configuration. Now everything else below here
# is about getting dependencies and macros and various build behaviors

##############################################################################
##############################################################################
##############################################################################
##############################################################################
##############################################################################
##############################################################################


# some useful utilities
include(copy_file)
include(CMakeParseArguments)
include(eval)


# This is an internal function and should not be used
# Usage:
# make_target_impl(target compile_flags sources requirements is_library SHARED)
#
# Example:
# make_target_impl(fileio "-fPIC"
#                   "asyncurl.cpp;sysutils.cpp"
#                   "logger;dl;pthread;z"
#                   TRUE FALSE)
#
# This generates a target library/binary with the given name. The optional
# compile_flags are appended to the target compile flags. "-fPIC" is ALWAYS
# added for libraries. "sources" is a list listing all the library/binary
# source files.  "requirements" is a list listing all the libraries, and
# builtins this target depends on. IS_LIBRARY must be "TRUE" or "FALSE"
#
# if DYNAMIC is true, a dynamic library is built.
#
# Boost, pthread is always added as a default dependency. OpenMP is added
# when possible.
macro(make_target_impl NAME FLAGS REQUIREMENTS IS_LIBRARY SHARED SHARED_ALL_DEFINED OBJECT)
  # create the target
  if (${IS_LIBRARY})
    message(STATUS "Adding Library: ${NAME}")
  else()
    message(STATUS "Adding Executable: ${NAME}")
    # default dependencies
    target_link_libraries(${NAME} boost pthread openmp)
  endif()

  set_property(TARGET ${NAME} PROPERTY IS_LIBRARY ${IS_LIBRARY})

  # add a custom property to the target listing its dependencies
  if(NOT ${FLAGS} STREQUAL "")
    set_property(TARGET ${NAME} APPEND_STRING PROPERTY COMPILE_FLAGS " ${FLAGS}")
  endif()
  if (${IS_LIBRARY})
    if (NOT WIN32)
      #windows is always fPIC
      set_property(TARGET ${NAME} APPEND_STRING PROPERTY COMPILE_FLAGS " -fPIC")
    endif()
    if (APPLE)
      if (${SHARED})
        if (NOT ${SHARED_ALL_DEFINED})
          set_property(TARGET ${NAME} APPEND_STRING PROPERTY LINK_FLAGS " -undefined dynamic_lookup")
        endif()
      endif()
    endif()
  endif()

  if (${IS_LIBRARY})
    if(${SHARED})
      target_link_libraries(${NAME} PRIVATE ${REQUIREMENTS})
    elseif(${OBJECT})
      # TODO we can link the requirements from here when target_link_libraries
      # works with OBJECT library targets (requires CMake 3.12)
      # See https://gitlab.kitware.com/cmake/cmake/issues/14778
      # For now, do nothing.
    else()
      target_link_libraries(${NAME} PUBLIC ${REQUIREMENTS})
    endif()
  else()
    target_link_libraries(${NAME} ${REQUIREMENTS})
  endif()
  # make sure boost is always built
  add_dependencies(${NAME} boost)
endmacro()


# This is an external function
# Usage:
#    make_library(NAME target
#                 SOURCES a.cpp b.cpp
#                 REQUIRES libx liby
#                 MAC_REQUIRES libz libzz
#                 LINUX_REQUIRES libk libj
#                 [SHARED] [OUTPUT_NAME xxxx] [SHARED_ALL_DEFINED]
#                 [OBJECT]
#                 )
# Example:
#
# make_library(NAME fileio
#              SOURCES
#                asyncurl.cpp
#                sysutils.cpp
#                wsconn.cpp
#                s3_api.cpp
#                hdfs.cpp
#               REQUIRES
#                 logger dl pthread z curl xml2 openssl
#               MAC_REQUIRES
#                 iconv
#                 )
# This generates a library with the provided target name.
#
# NAME and SOURCES must be specified.
# REQUIRES lists all dependent libraries. These can be:
#   - other libraries built by the the turicreate build system
#   - builtin libraries
#   - system libraries
# MAC_REQUIRES lists all dependent libraries which are included only on Mac.
# LINUX_REQUIRES lists all dependent libraries which are included only on Linux.
# SHARED will build a shared library instead of a static library
# EXTERNAL_VISIBILITY will make the symbols be publicly visible. Default is hidden
# SHARED_ALL_DEFINED will require shared libraries to have all symbols defined
# OBJECT will build an object library instead of a static library
#
# All other targets which depends on this library (using the "requires" function)
# will automatically include all recursive dependencies.
#
# Boost, pthread is always added as a default dependency. OpenMP is added
# when possible.
macro(make_library NAME)
  set(options SHARED EXTERNAL_VISIBILITY SHARED_ALL_DEFINED DEAD_STRIP OBJECT)
  set(one_value_args COMPILE_FLAGS OUTPUT_NAME EXPORT_LINUX_MAP_FILE EXPORT_OSX_MAP_FILE)
  set(multi_value_args
    SOURCES REQUIRES MAC_REQUIRES LINUX_REQUIRES
    COMPILE_FLAGS_EXTRA COMPILE_FLAGS_EXTRA_CLANG COMPILE_FLAGS_EXTRA_GCC)
 CMAKE_PARSE_ARGUMENTS(make_library "${options}" "${one_value_args}" "${multi_value_args}" ${ARGN})
  if(NOT make_library_SOURCES)
    MESSAGE(FATAL_ERROR "make_library call with no sources")
  endif()

  if (APPLE)
    if (make_library_MAC_REQUIRES)
      set(make_library_REQUIRES ${make_library_REQUIRES} ${make_library_MAC_REQUIRES})
    endif()
  else()
    if (make_library_LINUX_REQUIRES)
      set(make_library_REQUIRES ${make_library_REQUIRES} ${make_library_LINUX_REQUIRES})
    endif()
  endif()

  if (${make_library_SHARED})
    add_library(${NAME} SHARED ${make_library_SOURCES})
  elseif(${make_library_OBJECT})
    add_library(${NAME} OBJECT ${make_library_SOURCES})
  else()
    add_library(${NAME} STATIC ${make_library_SOURCES})
  endif()

  make_target_impl("${NAME}" "${make_library_COMPILE_FLAGS}"
    "${make_library_REQUIRES}" TRUE "${make_library_SHARED}" "${make_library_SHARED_ALL_DEFINED}" "${make_library_OBJECT}")

  if (make_library_OUTPUT_NAME)
          message(STATUS "make_library ${NAME} ===> ${make_library_OUTPUT_NAME}")
          set_target_properties(${NAME} PROPERTIES OUTPUT_NAME ${make_library_OUTPUT_NAME})
  endif()

  if (make_library_COMPILE_FLAGS_EXTRA)
    target_compile_options(${NAME} PRIVATE ${make_library_COMPILE_FLAGS_EXTRA})
  endif()

  if (CLANG)
    if (make_library_COMPILE_FLAGS_EXTRA_CLANG)
      target_compile_options(${NAME} PRIVATE ${make_library_COMPILE_FLAGS_EXTRA_CLANG})
    endif()
  endif()

  if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
    if (make_library_COMPILE_FLAGS_EXTRA_GCC)
      target_compile_options(${NAME} PRIVATE ${make_library_COMPILE_FLAGS_EXTRA_GCC})
    endif()
  endif()

  if (${make_library_EXTERNAL_VISIBILITY} OR ${make_library_OBJECT})
    # do nothing
    message(STATUS "External Visibility: " ${NAME})
    target_compile_options(${NAME} PRIVATE "-fvisibility=default")
    target_compile_options(${NAME} PRIVATE $<$<COMPILE_LANGUAGE:CXX>:-fvisibility-inlines-hidden>)
  else()
    target_compile_options(${NAME} PRIVATE "-fvisibility=hidden")
    target_compile_options(${NAME} PRIVATE $<$<COMPILE_LANGUAGE:CXX>:-fvisibility-inlines-hidden>)
  endif()

  if(NOT CLANG)
    if (NOT WIN32)
      # set_property(TARGET ${NAME} APPEND_STRING PROPERTY LINK_FLAGS " -static-libstdc++ ")
    endif()
  endif()

  if(APPLE)
  if(make_library_EXPORT_OSX_MAP_FILE)
    set_property(TARGET ${NAME} APPEND PROPERTY LINK_DEPENDS "${make_library_EXPORT_OSX_MAP_FILE}")
    set_property(TARGET ${NAME} APPEND_STRING PROPERTY LINK_FLAGS " -Wl,-exported_symbols_list,${make_library_EXPORT_OSX_MAP_FILE} ")
  endif()

  if(make_library_DEAD_STRIP)
    set_property(TARGET ${NAME} APPEND_STRING PROPERTY LINK_FLAGS " -Wl,-dead_strip")
  endif()

else()
  if(make_library_EXPORT_LINUX_MAP_FILE)
    set_property(TARGET ${NAME} APPEND PROPERTY LINK_DEPENDS "${make_library_EXPORT_LINUX_MAP_FILE}")
    set_property(TARGET ${NAME} APPEND_STRING PROPERTY LINK_FLAGS " -Wl,--version-script=${make_library_EXPORT_LINUX_MAP_FILE} ")
  endif()
endif()

endmacro()

# This is an external function
# Usage:
#    make_empty_library(NAME target
#                       REQUIRES libx liby
#                       MAC_REQUIRES libz libzz
#                       LINUX_REQUIRES libk libj
#                       [OUTPUT_NAME xxxx])
# Example:
#
# make_empty_library(NAME graph
#               REQUIRES
#                 logger dl pthread z curl xml2 openssl
#               MAC_REQUIRES
#                 iconv
#                 )
# This generates an empty target with the provided target name, but all
# other targets which require this target will inherit all REQUIRED
# dependencies.
#
# NAME must be specified
# REQUIRES lists all dependent libraries. These can be:
#   - other libraries built by the the turicreate build system
#   - builtin libraries
#   - system libraries
# MAC_REQUIRES lists all dependent libraries which are included only on Mac.
# LINUX_REQUIRES lists all dependent libraries which are included only on Linux.
#
# All other targets which depends on this library (using the "requires" function)
# will automatically include all recursive dependencies.
macro(make_empty_library NAME)
  set(one_value_args COMPILE_FLAGS)
  set(multi_value_args REQUIRES MAC_REQUIRES LINUX_REQUIRES)
  CMAKE_PARSE_ARGUMENTS(make_library "${options}" "${one_value_args}" "${multi_value_args}" ${ARGN})
  if (APPLE)
    if (make_library_MAC_REQUIRES)
      set(make_library_REQUIRES ${make_library_REQUIRES} ${make_library_MAC_REQUIRES})
    endif()
  else()
    if (make_library_LINUX_REQUIRES)
      set(make_library_REQUIRES ${make_library_REQUIRES} ${make_library_LINUX_REQUIRES})
    endif()
  endif()

  if (NOT EXISTS ${CMAKE_SOURCE_DIR}/dummy.cpp)
    file(WRITE ${CMAKE_SOURCE_DIR}/dummy.cpp "")
  endif()
  add_library(${NAME} STATIC ${CMAKE_SOURCE_DIR}/dummy.cpp)
  make_target_impl("${NAME}" ""
      "${make_library_REQUIRES}" TRUE FALSE FALSE FALSE)

    set_target_properties(${NAME} PROPERTIES EMPTY_LIBRARY TRUE)
endmacro()

# This is an external function
# Usage:
#    make_binary(NAME target
#                SOURCES a.cpp b.cpp
#                REQUIRES libx liby
#                MAC_REQUIRES libz libzz
#                LINUX_REQUIRES libk libj)
# Example:
#
# make_binary(NAME wscmd
#              SOURCES
#                wscmd.cpp
#              REQUIRES
#                fileio
#             )
#
# This generates a binary with the provided target name.
#
# NAME and SOURCES must be specified.
# REQUIRES lists all dependent libraries. These can be:
#   - other libraries built by the the turicreate build system
#   - builtin libraries
#   - system libraries
# MAC_REQUIRES lists all dependent libraries which are included only on Mac.
# LINUX_REQUIRES lists all dependent libraries which are included only on Linux.
# OUTPUT_NAME is the final output name of the target. Defaults to the target name
# if not specified
#
# All other targets which depends on this library (using the "requires" function)
# will automatically include all recursive dependencies.
#
# Boost, pthread is always added as a default dependency. OpenMP is added
# when possible.
function (make_executable NAME)
  set(one_value_args COMPILE_FLAGS OUTPUT_NAME)
  set(multi_value_args
    SOURCES REQUIRES MAC_REQUIRES LINUX_REQUIRES
    COMPILE_FLAGS_EXTRA COMPILE_FLAGS_EXTRA_CLANG COMPILE_FLAGS_EXTRA_GCC)
  CMAKE_PARSE_ARGUMENTS(make_library "" "${one_value_args}" "${multi_value_args}" ${ARGN})
  if(NOT make_library_SOURCES)
    MESSAGE(FATAL_ERROR "make_executable call with no sources")
  endif()

  if (APPLE)
    if (make_library_MAC_REQUIRES)
      set(make_library_REQUIRES ${make_library_REQUIRES} ${make_library_MAC_REQUIRES})
    endif()
  else()
    if (make_library_LINUX_REQUIRES)
      set(make_library_REQUIRES ${make_library_REQUIRES} ${make_library_LINUX_REQUIRES})
    endif()
  endif()

  add_executable(${NAME} ${make_library_SOURCES})

  if (make_library_COMPILE_FLAGS_EXTRA)
    target_compile_options(${NAME} PRIVATE ${make_library_COMPILE_FLAGS_EXTRA})
  endif()

  if (CLANG)
    if (make_library_COMPILE_FLAGS_EXTRA_CLANG)
      target_compile_options(${NAME} PRIVATE ${make_library_COMPILE_FLAGS_EXTRA_CLANG})
    endif()
  endif()

  if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
    if (make_library_COMPILE_FLAGS_EXTRA_GCC)
      target_compile_options(${NAME} PRIVATE ${make_library_COMPILE_FLAGS_EXTRA_GCC})
    endif()
  endif()

  make_target_impl("${NAME}" "${make_library_COMPILE_FLAGS}"
    "${make_library_REQUIRES}" FALSE FALSE FALSE FALSE)
  if (make_library_OUTPUT_NAME)
          message(STATUS "make_executable ${NAME} ===> ${make_library_OUTPUT_NAME}")
          set_target_properties(${NAME} PROPERTIES OUTPUT_NAME ${make_library_OUTPUT_NAME})
  endif()
  # this is really annoying
  # There really isn't a clean way to this, but on Mac Anaconda's libpython2.7.dylib
  # has it's install name set to just libpython2.7.dylib and not @rapth/libpython2.7.dylib
  # We need to patch this.
  if (APPLE)
          add_custom_command(TARGET ${NAME} POST_BUILD
                  COMMAND install_name_tool $<TARGET_FILE:${NAME}> -change libpython2.7.dylib @rpath/libpython2.7.dylib)
  endif()

  if(NOT CLANG)
    if (NOT WIN32)
      # set_property(TARGET ${NAME} APPEND_STRING PROPERTY LINK_FLAGS "-static-libstdc++")
    endif()
  endif()
endfunction()


function (make_boost_test NAME)
  set (SOURCES ${NAME})
  set(args ${ARGN})
  make_executable(${NAME}test SOURCES ${SOURCES} ${args})
  target_link_libraries(${NAME}test boost_test)

  add_test(${NAME} ${NAME}test)
endfunction()

# Same as target_compile_options, but tests for whether the flag is known
# to the compiler before proceeding.
function (target_optional_compile_flag NAME FLAG)
  check_cxx_compiler_flag(${FLAG} __has_flag)
  if (${__has_flag})
    target_compile_options(${NAME} PRIVATE ${FLAG})
  endif()
endfunction()

# This is an external function
# Usage:
#    make_copy_target(target
#                TARGETS [list of targets]
#                FILES [list of files (absolute path)]
#                DIRECTORIES [list of directories (absolute path)
# Example:
# make_copy_target(NAME target
#                TARGETS a b
#                FILES ${CMAKE_SOURCE_DIR}/pika/a.txt
#             )
#
# This copies all files produced by targets in TARGETS to the output binary
# directory as well as all files in FILES.
#
# TARGETS may reference an existing copy_target in which case all files copied
# by the copy target will also be copied.
#
# For instance:
# make_copy_target(NAME spark_pipe_wrapper
#                FILES ${CMAKE_CURRENT_SOURCE_DIR}/spark_pipe_wrapper.py)
#
# Then in some other location
#
# make_copy_target(NAME release_binaries
#                TARGETS spark_pipe_wrapper)
#
# Warning: The recursive reference is slightly brittle since it requires the
# referenced target to exist prior to referencing it.
# This is potentially fixable with complicated generator expressions. but... urgh.
macro (make_copy_target NAME)
  set(multi_value_args TARGETS FILES DIRECTORIES)
  CMAKE_PARSE_ARGUMENTS(copy "" "" "${multi_value_args}" ${ARGN})
  ADD_CUSTOM_TARGET(${NAME} ALL)
  set(all_target_locations)
  set(RSYNC_SOURCES ${copy_FILES} ${copy_DIRECTORIES})
  find_program(RSYNC rsync)
  if("${RSYNC}" STREQUAL "")
    # No rsync present; copy the old fashioned way with multiple cp commands
    foreach(binary ${copy_FILES})
            # Switched away from cmake -E commands because they don't work right
            # on Windows. Also Windows doesn't overwrite when copying by default
            if(WIN32)
              add_custom_command(TARGET ${NAME} POST_BUILD
                COMMENT "remove old ${binary}"
                COMMAND rm -f ${CMAKE_CURRENT_BINARY_DIR}/`basename ${binary}` )
            endif()
            add_custom_command(TARGET ${NAME} POST_BUILD
                    COMMENT "copy ${binary}"
                    COMMAND cp ${binary} ${CMAKE_CURRENT_BINARY_DIR})
    endforeach()

    foreach(binary ${copy_DIRECTORIES})
            # Switched away from cmake -E commands because they don't work right
            # on Windows. Also Windows doesn't overwrite when copying by default
            add_custom_command(TARGET ${NAME} POST_BUILD
                    COMMENT "remove old ${binary}"
                    COMMAND rm -rf ${CMAKE_CURRENT_BINARY_DIR}/`basename ${binary}` )
            add_custom_command(TARGET ${NAME} POST_BUILD
                    COMMENT "copy ${binary}"
                    COMMAND cp -r ${binary} ${CMAKE_CURRENT_BINARY_DIR})
    endforeach()
  else()
    if((NOT "${copy_FILES}" STREQUAL "") OR (NOT "${copy_DIRECTORIES}" STREQUAL ""))
      add_custom_command(TARGET ${NAME} POST_BUILD
        COMMENT "copying ${NAME}"
        COMMAND ${RSYNC} -a ${RSYNC_SOURCES} ${CMAKE_CURRENT_BINARY_DIR}
      )
    endif()
  endif()

  # Make sure the files and directories get included in all_target_locations
  foreach(binary ${copy_FILES})
          list(APPEND all_target_locations ${binary})
  endforeach()
  foreach(binary ${copy_DIRECTORIES})
          list(APPEND all_target_locations ${binary})
  endforeach()

  if (NOT "${copy_TARGETS}" STREQUAL "")
    add_dependencies(${NAME} ${copy_TARGETS})
  endif()

  foreach(target ${copy_TARGETS})
          if (TARGET ${target})
                  get_property(CUSTOM_LOCATION TARGET ${target} PROPERTY COPY_FILES)
          else()
                  set(${CUSTOM_LOCATION} "")
          endif()
          if (NOT "${CUSTOM_LOCATION}" STREQUAL "")
                  foreach(location ${CUSTOM_LOCATION})
                          add_custom_command(TARGET ${NAME} POST_BUILD
                                  COMMENT "copy ${location}"
                                  DEPENDS ${target}
                                  COMMAND ${CMAKE_COMMAND} -E
                                  copy_if_different ${location} ${CMAKE_CURRENT_BINARY_DIR})
                          list(APPEND all_target_locations ${location})
                  endforeach()
          else()
                  add_custom_command(TARGET ${NAME} POST_BUILD
                          COMMENT "copying target of ${target}"
                          DEPENDS ${target}
                          COMMAND ${CMAKE_COMMAND} -E
                          copy_if_different $<TARGET_FILE:${target}> ${CMAKE_CURRENT_BINARY_DIR})
                  list(APPEND all_target_locations $<TARGET_FILE:${target}>)
          endif()
  endforeach()
  message(STATUS "Adding Copy Target ${NAME} is copying:  ${all_target_locations}")
  set_property(TARGET ${NAME} PROPERTY COPY_FILES "${all_target_locations}")
endmacro()

include(SharedLibraryFromStatic)
include(MergeStaticLibraries)

add_custom_target(external_dependencies)

include(ExternalProject)
file(GLOB packages "${DEPS_CMAKE}/ExternalProject*.cmake")
foreach(package ${packages})
        get_filename_component(packagename "${package}" NAME_WE)
        #package is of the form ExternalProjectXXX"
        include(${package})
        STRING(SUBSTRING "${packagename}" 15 -1 depname)
        string(TOLOWER ${depname} depname)
        set(package_${depname} requires_${depname} CACHE STRING "Package map")
        add_dependencies(external_dependencies ex_${depname})
endforeach()

include_directories(src)
include_directories(SYSTEM src/external)
include_directories(SYSTEM src/external/aws-sdk-cpp/aws-cpp-sdk-core/include)
include_directories(SYSTEM src/external/aws-sdk-cpp/aws-cpp-sdk-s3/include)
include_directories(SYSTEM src/external/google)
include_directories(src/platform)

if(EXISTS ${CMAKE_SOURCE_DIR}/extensions)
  include_directories(SYSTEM extensions)
  add_subdirectory(extensions)
endif()

# for build-time generated source code
include_directories(SYSTEM ${CMAKE_BINARY_DIR}/src)

if(EXISTS ${CMAKE_SOURCE_DIR}/subtree)
  include_directories(SYSTEM subtree)
  add_subdirectory(subtree)
endif()

# Collate all the object targets shared among static and shared library targets
# These are used by C API, unity_shared, etc.
set(_TC_DEFAULT_SERVER_INITIALIZER
  "${CMAKE_SOURCE_DIR}/src/capi/default_server_initializer.cpp"
)
set(_TC_COMMON_OBJECTS
  "$<TARGET_OBJECTS:capi>"

  # Also list all dependencies (including transitive dependencies)

  # External dependencies (built from src/external)
  "$<TARGET_OBJECTS:libjson>"
  "$<TARGET_OBJECTS:lz4>"
  "$<TARGET_OBJECTS:protobuf>"
  "$<TARGET_OBJECTS:uuid>"
  "$<TARGET_OBJECTS:xgboost>"
  "$<TARGET_OBJECTS:coremltools_mlmodel>"

  # Then our own source code
  "$<TARGET_OBJECTS:annotation>"
  "$<TARGET_OBJECTS:cancel_serverside_ops>"
  "$<TARGET_OBJECTS:crash_handler>"
  "$<TARGET_OBJECTS:cppipc>"
  "$<TARGET_OBJECTS:flexible_type>"
  "$<TARGET_OBJECTS:globals>"
  "$<TARGET_OBJECTS:fileio>"
  "$<TARGET_OBJECTS:image_io>"
  "$<TARGET_OBJECTS:image_type>"
  "$<TARGET_OBJECTS:logger>"
  "$<TARGET_OBJECTS:minipsutil>"
  "$<TARGET_OBJECTS:ml_data>"
  "$<TARGET_OBJECTS:nanosockets>"
  "$<TARGET_OBJECTS:network>"
  "$<TARGET_OBJECTS:optimization>"
  "$<TARGET_OBJECTS:parallel>"
  "$<TARGET_OBJECTS:process>"
  "$<TARGET_OBJECTS:pylambda>"
  "$<TARGET_OBJECTS:random>"
  "$<TARGET_OBJECTS:serialization>"
  "$<TARGET_OBJECTS:sframe>"
  "$<TARGET_OBJECTS:sframe_query_engine>"
  "$<TARGET_OBJECTS:sgraph>"
  "$<TARGET_OBJECTS:shmipc>"
  "$<TARGET_OBJECTS:stack_trace>"
  "$<TARGET_OBJECTS:startup_teardown>"
  "$<TARGET_OBJECTS:supervised_learning>"
  "$<TARGET_OBJECTS:table_printer>"
  "$<TARGET_OBJECTS:timer>"
  "$<TARGET_OBJECTS:unity>"
  "$<TARGET_OBJECTS:unity_activity_classification>"
  "$<TARGET_OBJECTS:unity_core>"
  "$<TARGET_OBJECTS:unity_coreml_model_export>"
  "$<TARGET_OBJECTS:unity_clustering>"
  "$<TARGET_OBJECTS:unity_drawing_classifier>"
  "$<TARGET_OBJECTS:unity_evaluation>"
  "$<TARGET_OBJECTS:unity_factorization>"
  "$<TARGET_OBJECTS:unity_feature_engineering>"
  "$<TARGET_OBJECTS:unity_graph_analytics>"
  "$<TARGET_OBJECTS:unity_image_export>"
  "$<TARGET_OBJECTS:unity_ml_data_2>"
  "$<TARGET_OBJECTS:unity_ml_model>"
  "$<TARGET_OBJECTS:unity_nearest_neighbors>"
  "$<TARGET_OBJECTS:unity_neural_net>"
  "$<TARGET_OBJECTS:unity_object_detection>"
  "$<TARGET_OBJECTS:unity_one_shot_object_detection>"
  "$<TARGET_OBJECTS:unity_pattern_mining>"
  "$<TARGET_OBJECTS:unity_prototype>"
  "$<TARGET_OBJECTS:unity_recsys>"
  "$<TARGET_OBJECTS:unity_sgd>"
  "$<TARGET_OBJECTS:unity_sparse_similarity>"
  "$<TARGET_OBJECTS:unity_text>"
  "$<TARGET_OBJECTS:unity_util>"
  "$<TARGET_OBJECTS:util>"
  "$<TARGET_OBJECTS:visualization>"
)

# TODO we can remove the requirements from here when target_link_libraries
# works with OBJECT library targets (requires CMake 3.12)
# See https://gitlab.kitware.com/cmake/cmake/issues/14778
set(_TC_COMMON_REQUIREMENTS
  # External dependencies (from deps)
  boost
  bz2
  curl
  eigen
  libjpeg
  libpng
  openssl
  nanomsg
)

if(APPLE)
  set(_TC_COMMON_OBJECTS
    ${_TC_COMMON_OBJECTS}
    "$<TARGET_OBJECTS:image_deep_feature_extractor>"
    "$<TARGET_OBJECTS:platform_config>"
  )
  set(_TC_COMMON_REQUIREMENTS
    ${_TC_COMMON_REQUIREMENTS}
    ${CORE_ML}
    ${CORE_VIDEO}
    ${FOUNDATION}
  )
  if(NOT TC_BUILD_IOS)
    set(_TC_COMMON_OBJECTS
      ${_TC_COMMON_OBJECTS}
      "$<TARGET_OBJECTS:vega_renderer>"
    )
    set(_TC_COMMON_REQUIREMENTS
      ${_TC_COMMON_REQUIREMENTS}
      ${APPKIT}
      ${CORE_GRAPHICS}
      ${JAVASCRIPT_CORE}
    )
    if(HAS_MPS)
      set(_TC_COMMON_OBJECTS
        ${_TC_COMMON_OBJECTS}
        "$<TARGET_OBJECTS:tcmps>"
      )
      set(_TC_COMMON_REQUIREMENTS
        ${_TC_COMMON_REQUIREMENTS}
        ${ACCELERATE}
        ${CORE_IMAGE}
        ${METAL}
        ${METAL_PERFORMANCE_SHADERS}
      )
    endif()
  endif()
endif()
if(NOT TC_BUILD_IOS)
  # object targets that should be included in all APPLE and non-APPLE
  # platforms except for iOS
  set(_TC_COMMON_OBJECTS
    ${_TC_COMMON_OBJECTS}
    "$<TARGET_OBJECTS:aws-sdk-cpp>"
  )
endif()

add_subdirectory(src)
add_subdirectory(test)
