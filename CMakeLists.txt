# We require a recent version of cmake and automatically install a compatible
# version when using the cmake lists, if one is not already present.
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
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# ##############################################################################
# Set the base configuration for a number of things.

project(Turi)
enable_language(C)
enable_language(CXX)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED 1)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# **************************************************************************/
# * */
# * Global Link, Include and Define Flags              */
# * */
# **************************************************************************/

set(CMAKE_MODULE_PATH "${CMAKE_MODULE_PATH};${CMAKE_SOURCE_DIR}/cmake")

# Configure the compiler
include(SetupCompiler)
setupcompiler()

if(WIN32)
  add_definitions(-DWINVER=0x0600)
  add_definitions(-D_WIN32_WINNT=0x0600)
endif()

if(${TC_BUILD_IOS})
  add_definitions(-DTC_BUILD_IOS)

  if(${TC_BUILD_REMOTEFS})
    message(ERROR "RemoteFS must be disabled for building iOS.")
  endif()

endif()

if(NOT ${TC_BUILD_REMOTEFS})
  add_definitions(-DTC_DISABLE_REMOTEFS)
endif()

# Determine where additional Turi specific cmake modules are defined
set(DEPS_CMAKE ${CMAKE_SOURCE_DIR}/deps/cmake)
set(CMAKE_MODULE_PATH "${CMAKE_MODULE_PATH};${DEPS_CMAKE}")
set(CMAKE_PREFIX_PATH
    "${CMAKE_SOURCE_DIR}/deps/local:${CMAKE_SOURCE_DIR}/deps/env/"
    CACHE STRING "CMAKE PREFIX PATH" FORCE)
set(CMAKE_LIBRARY_PATH
    "${CMAKE_SOURCE_DIR}/deps/local/lib:${CMAKE_SOURCE_DIR}/deps/env/lib:${CMAKE_SOURCE_DIR}/deps/env/local/lib:${CMAKE_LIBRARY_PATH}"
    CACHE STRING "CMAKE Library Path" FORCE)
set(ENV{PATH}
    "${CMAKE_SOURCE_DIR}/deps/local/bin:${CMAKE_SOURCE_DIR}/deps/env/bin:$ENV{PATH}"
)

# Test for python support.  Needs the path environment variable to be set to
# find cython reliably
if(TC_BUILD_PYTHON)
  message("Building python libraries.")

  if(NOT EXISTS ${CMAKE_SOURCE_DIR}/deps/env/bin/python)
    message(
      FATAL_ERROR "Internal error: Python toolchain expected; not present.")
  endif()

  include_directories(${CMAKE_SOURCE_DIR}/deps/env/include)
  include(UseCython)

  add_definitions(-DTC_HAS_PYTHON=1)
  add_definitions(-DTC_BUILD_VISUALIZATION_CLIENT=1)
else()
  message("Skipping python libraries.")
endif()

# Add global defines
add_definitions(-DTC_BUILD_PATH_BASE="${CMAKE_SOURCE_DIR}")
add_definitions(-DCURL_STATICLIB)
add_definitions(-DIN_TURI_SOURCE_TREE)
add_definitions(-DFUSION_MAX_VECTOR_SIZE=20)
add_definitions(-DBOOST_SPIRIT_THREAD_SAFE)
add_definitions(-DBOOST_THREAD_ONCE_ATOMIC)

# These determine the maximum number of arguments for extension functions
add_definitions(-DBOOST_FUSION_INVOKE_MAX_ARITY=12)
add_definitions(-DBOOST_FUSION_INVOKE_PROCEDURE_MAX_ARITY=12)
add_definitions(-DBOOST_FUSION_INVOKE_FUNCTION_OBJECT_MAX_ARITY=12)

# Workaround for https://svn.boost.org/trac10/ticket/10443 in Boost 1.68.0
add_definitions(-DBOOST_RESULT_OF_USE_TR1_WITH_DECLTYPE_FALLBACK)

# Legal define for Eigen.
add_definitions(-DEIGEN_MPL2_ONLY)

# Defines to avoid symbol collisions
add_definitions(-Dgoogle=_tc_google)

# **************************************************************************/
# * */
# * Adapt Compiler and Linker Flags to the system              */
# * */
# **************************************************************************/

include(CompilerFlags)

# Set some universal flags that we don't need to test for
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fPIC")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -fPIC")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Werror")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Werror")

# Go through and add in all the relevant build flags as appropriate
check_and_set_compiler_flag(-ftemplate-depth=900 CXX)
check_and_set_compiler_flag(-Wno-c++11-narrowing CXX)
check_and_set_compiler_flag(-Wno-stdlibcxx-not-found CXX RESTRICT_CLANG)

check_and_set_compiler_flag(-Wno-unused-command-line-argument)
check_and_set_compiler_flag(-Wno-deprecated-declarations)
check_and_set_compiler_flag(-Wno-implicit-function-declaration)
check_and_set_compiler_flag(-Wno-deprecated-register)
check_and_set_compiler_flag(-Qunused-arguments)
check_and_set_compiler_flag(-Wno-mismatched-tags)
check_and_set_compiler_flag(-Wno-enum-compare)
check_and_set_compiler_flag(-Wno-conversion-null)
check_and_set_compiler_flag(-Wno-constant-logical-operand)
check_and_set_compiler_flag(-Wno-parentheses-equality)
check_and_set_compiler_flag(-Wno-unknown-pragmas)
check_and_set_compiler_flag(-Wno-unused-local-typedefs)
check_and_set_compiler_flag(-Wno-attributes)
check_and_set_compiler_flag(-Wno-tautological-compare)

check_and_set_compiler_flag(-fpeel-loops RELEASE)
check_and_set_compiler_flag(-funswitch-loops RELEASE)
check_and_set_compiler_flag(-ftracer RELEASE)

if(APPLE)
  # This triggers a bug in clang; the 10.13 symbol ___chkstk_darwin is missing
  # in 10.13, and the code generated doesn't run on that, even with the 10.12
  # target. The solution to this is to disable the stack checking globally,
  # which seems to be enabled by default in clang 11 and later for objective C++
  # code.
  check_and_set_compiler_flag(-fno-stack-check)
  check_and_set_compiler_flag(-fno-stack-protector)
endif()

# Turn on ARC globally.
check_and_set_compiler_flag(-fobjc-arc)

# This flag needs to be set after all other warning flags, or it may give false
# positives.
check_and_set_compiler_flag(-Wno-unknown-warning-option)

# Set the different optimization flags
if(${DEBUG_OPT_FOR_SIZE})
  set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -Os -g")
  set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -Os -g")
else()
  set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -O0 -fno-inline -g")
  set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -O0 -fno-inline -g")
endif()

if(${RELEASE_OPT_FOR_SIZE})
  set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -Os -DNDEBUG")
  set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -Os -DNDEBUG")
else()
  set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -O3 -DNDEBUG")
  set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3 -DNDEBUG")
endif()

# Set up the 128 bit integer support; defines INT128_FLAGS
include(FindInt128)
find_int128_types()

# Set up defines around a few compiler and language oddities that we have to
# understand.
include(CompilerOddities)
set_compiler_specific_flags()

# Set up threads.
set(THREADS_PREFER_PTHREAD_FLAG TRUE)
set(CMAKE_THREAD_PREFER_PTHREAD TRUE)
find_package(Threads REQUIRED)
if(CMAKE_USE_PTHREADS_INIT)
  add_definitions(-DHAVE_PTHREAD)
endif()

# Get framework dependencies.
if(APPLE)
  find_library(ACCELERATE NAMES Accelerate)
  find_library(CORE_GRAPHICS CoreGraphics)
  find_library(JAVASCRIPT_CORE JavaScriptCore)
  find_library(FOUNDATION Foundation)
  find_library(CORE_IMAGE NAMES CoreImage)
  find_library(CORE_ML CoreML)
  find_library(CORE_VIDEO CoreVideo)
  find_library(METAL NAMES Metal)
  find_library(METAL_PERFORMANCE_SHADERS NAMES MetalPerformanceShaders)
  set(_TC_APPLE_DEPENDENCIES
      ${ACCELERATE}
      ${CORE_GRAPHICS}
      ${JAVASCRIPT_CORE}
      ${FOUNDATION}
      ${CORE_IMAGE}
      ${CORE_ML}
      ${CORE_VIDEO}
      ${METAL}
      ${METAL_PERFORMANCE_SHADERS})

  if(NOT ${TC_BUILD_IOS})
    find_library(APPKIT AppKit)
    set(_TC_APPLE_DEPENDENCIES ${_TC_APPLE_DEPENDENCIES} ${APPKIT})
  endif()
endif()

if(APPLE)
  # if mac, but not using clang, we should use the clang linker anyway since the
  # gcc linker seems to cause problems. Especially with -march=native
  #
  # see: http://stackoverflow.com/questions/9840207/how-to-use-avx-pclmulqdq-on-
  # mac-os-x-lion
  if(NOT CLANG)
    set(ALTERNATE_LINKER "-Wa,-q")
  endif()
endif()

# Install time rpath gets highest priority, we should always use rpath relative
# to the installation location first.
if(WIN32)
  set(CMAKE_EXE_LINKER_FLAGS
      "${CMAKE_EXE_LINKER_FLAGS} -Wl,-rpath,${CMAKE_SOURCE_DIR}/deps/local/bin")
  set(CMAKE_EXE_LINKER_FLAGS
      "${CMAKE_EXE_LINKER_FLAGS} -Wl,-rpath,/mingw64/bin")
  set(CMAKE_MODULE_LINKER_FLAGS
      "${CMAKE_MODULE_LINKER_FLAGS} -Wl,-rpath,/mingw64/bin")
elseif(APPLE)
  # For whatever reason just adding -rpath=@loader_path has issues on 10.9 but
  # not 10.10 Here are two possible alternatives that "may" fix the issue. If
  # they fix it I am not sure which line was the one which actually worked
  set(CMAKE_EXE_LINKER_FLAGS
      "${CMAKE_EXE_LINKER_FLAGS} -Wl,-rpath,@loader_path/.")
  set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,-rpath,.")
  set(CMAKE_SHARED_LINKER_FLAGS
      "${CMAKE_SHARED_LINKER_FLAGS} -Wl,-rpath,@loader_path -Wl,-rpath,@loader_path/.."
  )
  set(CMAKE_MODULE_LINKER_FLAGS
      "${CMAKE_MODULE_LINKER_FLAGS} -Wl,-rpath,@loader_path -Wl,-rpath,@loader_path/.."
  )
else()
  # LINUX
  set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,-rpath,\\$ORIGIN")
  set(CMAKE_SHARED_LINKER_FLAGS
      "${CMAKE_SHARED_LINKER_FLAGS} -Wl,-rpath,\\$ORIGIN -Wl,-rpath,\\$ORIGIN/.."
  )
  set(CMAKE_MODULE_LINKER_FLAGS
      "${CMAKE_MODULE_LINKER_FLAGS} -Wl,-rpath,\\$ORIGIN -Wl,-rpath,\\$ORIGIN/.."
  )
endif()

# Common install locations for deps linked libraries
set(CMAKE_EXE_LINKER_FLAGS
    "${CMAKE_EXE_LINKER_FLAGS} -Wl,-rpath,${CMAKE_SOURCE_DIR}/deps/local/lib64")
set(CMAKE_SHARED_LINKER_FLAGS
    "${CMAKE_SHARED_LINKER_FLAGS} -Wl,-rpath,${CMAKE_SOURCE_DIR}/deps/local/lib64"
)
set(CMAKE_MODULE_LINKER_FLAGS
    "${CMAKE_MODULE_LINKER_FLAGS} -Wl,-rpath,${CMAKE_SOURCE_DIR}/deps/local/lib64"
)

set(CMAKE_EXE_LINKER_FLAGS
    "${CMAKE_EXE_LINKER_FLAGS} -Wl,-rpath,${CMAKE_SOURCE_DIR}/deps/local/lib")
set(CMAKE_SHARED_LINKER_FLAGS
    "${CMAKE_SHARED_LINKER_FLAGS} -Wl,-rpath,${CMAKE_SOURCE_DIR}/deps/local/lib"
)
set(CMAKE_MODULE_LINKER_FLAGS
    "${CMAKE_MODULE_LINKER_FLAGS} -Wl,-rpath,${CMAKE_SOURCE_DIR}/deps/local/lib"
)

# **************************************************************************/
# * */
# * Report Final Flags                                */
# * */
# **************************************************************************/
message("CMAKE_BUILD_TYPE= ${CMAKE_BUILD_TYPE}.")

if(${CMAKE_BUILD_TYPE} MATCHES "Release")
  message(
    STATUS
      "Release build with C++ flags: ${CMAKE_CXX_FLAGS} ${CMAKE_CXX_FLAGS_RELEASE}"
  )
  message(
    STATUS
      "Release build with C flags: ${CMAKE_C_FLAGS} ${CMAKE_C_FLAGS_RELEASE}")
elseif(${CMAKE_BUILD_TYPE} MATCHES "Debug")
  message(
    STATUS
      "Debug build with C++ flags: ${CMAKE_CXX_FLAGS} ${CMAKE_CXX_FLAGS_DEBUG}")
  message(
    STATUS "Debug build with C flags: ${CMAKE_C_FLAGS} ${CMAKE_C_FLAGS_DEBUG}")
else()
  message(FATAL_ERROR "Unknown build type: " ${CMAKE_BUILD_TYPE}
                      "!. Rerun ./configure")
endif()

# ##############################################################################
# ##############################################################################
# ##############################################################################
# ##############################################################################
# ##############################################################################
# ##############################################################################

# We are done with the system configuration. Now everything else below here is
# about getting dependencies and macros and various build behaviors

# ##############################################################################
# ##############################################################################
# ##############################################################################
# ##############################################################################
# ##############################################################################
# ##############################################################################

# some useful utilities
include(copy_file)
include(CMakeParseArguments)
include(eval)
include(MakeLibrary)
include(MakeExecutable)
include(MakeTest)
include(BuildUtilities)

include_directories(SYSTEM ${CMAKE_SOURCE_DIR}/deps/local/include)
include_directories(SYSTEM ${CMAKE_SOURCE_DIR}/src/external/boost/boost_1_68_0/)
include_directories(SYSTEM
                    ${CMAKE_SOURCE_DIR}/src/external/libpng/libpng-1.6.37/)
include_directories(SYSTEM ${CMAKE_SOURCE_DIR}/src/external/libjpeg/jpeg-8d/)
include_directories(SYSTEM ${CMAKE_SOURCE_DIR}/src/external/bzip2/)
include_directories(SYSTEM ${CMAKE_SOURCE_DIR}/src/external/zlib/zlib-1.2.11/)
include_directories(SYSTEM ${CMAKE_SOURCE_DIR}/src/external/coremltools_wrap/)

link_directories(${CMAKE_SOURCE_DIR}/deps/local/lib)
link_directories(${CMAKE_SOURCE_DIR}/deps/local/lib64)

include_directories(src)
include_directories(src/core/system/platform)
include_directories(SYSTEM src/external)

if(${TC_BUILD_REMOTEFS})
  include_directories(SYSTEM src/external/aws-sdk-cpp/aws-cpp-sdk-core/include)
  include_directories(SYSTEM src/external/aws-sdk-cpp/aws-cpp-sdk-s3/include)
endif()

include_directories(SYSTEM src/external/google)
include_directories(SYSTEM src/external/nanomsg/nanomsg-1.0.0/src)

# Include all the external dependencies.
include(ExternalProject)

add_custom_target(external_dependencies)

file(GLOB packages "${DEPS_CMAKE}/ExternalProject*.cmake")
foreach(package ${packages})
  get_filename_component(packagename "${package}" NAME_WE)
  # package is of the form ExternalProjectXXX"
  include(${package})
  string(SUBSTRING "${packagename}" 15 -1 depname)
  string(TOLOWER ${depname} depname)
  set(package_${depname}
      requires_${depname}
      CACHE STRING "Package map")
  add_dependencies(external_dependencies ex_${depname})
endforeach()

set(_TC_EXTERNAL_DEPENDENCIES openssl libxml2 curl Threads::Threads
                              ${CMAKE_DL_LIBS})

# Collate all the object targets shared among static and shared library targets
# These are used by C API, unity_shared, etc.

if(NOT _TC_DEFAULT_SERVER_INITIALIZER)
  set(_TC_DEFAULT_SERVER_INITIALIZER
      "${CMAKE_SOURCE_DIR}/src/capi/default_server_initializer.cpp")
endif()

if(NOT TC_DISABLE_OBJECT_BUILDS)

  set(_TC_COMMON_OBJECTS
      # External dependencies (built from src/external)
      "$<TARGET_OBJECTS:libjpeg>"
      "$<TARGET_OBJECTS:libjson>"
      "$<TARGET_OBJECTS:libpng>"
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
      "$<TARGET_OBJECTS:startup_teardown>"
      "$<TARGET_OBJECTS:supervised_learning>"
      "$<TARGET_OBJECTS:table_printer>"
      "$<TARGET_OBJECTS:timer>"
      "$<TARGET_OBJECTS:unity_sframe>"
      "$<TARGET_OBJECTS:gl_sframe>"
      "$<TARGET_OBJECTS:unity_sketches>"
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
      "$<TARGET_OBJECTS:unity_style_transfer>"
      "$<TARGET_OBJECTS:unity_text>"
      "$<TARGET_OBJECTS:unity_util>"
      "$<TARGET_OBJECTS:util>"
      "$<TARGET_OBJECTS:visualization>")

  # TODO we can remove the requirements from here when target_link_libraries
  # works with OBJECT library targets (requires CMake 3.12) See
  # https://gitlab.kitware.com/cmake/cmake/issues/14778
  set(_TC_COMMON_REQUIREMENTS # External dependencies (from deps)
      ${_TC_EXTERNAL_DEPENDENCIES} nanomsg boost libbz2 z)

  if(APPLE)
    set(_TC_COMMON_REQUIREMENTS # External dependencies (from deps)
        ${_TC_COMMON_REQUIREMENTS} ${_TC_APPLE_DEPENDENCIES})
  endif()

  set(_TC_COMMON_OBJECTS ${_TC_COMMON_OBJECTS} "$<TARGET_OBJECTS:annotation>"
                         "$<TARGET_OBJECTS:visualization>")

  if(APPLE)
    set(_TC_COMMON_OBJECTS
        ${_TC_COMMON_OBJECTS} "$<TARGET_OBJECTS:image_deep_feature_extractor>"
        "$<TARGET_OBJECTS:platform_config>")
    set(_TC_COMMON_REQUIREMENTS ${_TC_COMMON_REQUIREMENTS} ${CORE_ML}
                                ${CORE_VIDEO} ${FOUNDATION})
    if(NOT ${TC_BUILD_IOS})
      set(_TC_COMMON_OBJECTS ${_TC_COMMON_OBJECTS}
                             "$<TARGET_OBJECTS:vega_renderer>")
      set(_TC_COMMON_REQUIREMENTS ${_TC_COMMON_REQUIREMENTS} ${APPKIT}
                                  ${CORE_GRAPHICS} ${JAVASCRIPT_CORE})
      if(HAS_MPS)
        set(_TC_COMMON_OBJECTS ${_TC_COMMON_OBJECTS} "$<TARGET_OBJECTS:tcmps>")
        set(_TC_COMMON_REQUIREMENTS
            ${_TC_COMMON_REQUIREMENTS} ${ACCELERATE} ${CORE_IMAGE} ${METAL}
            ${METAL_PERFORMANCE_SHADERS})
      endif()
    endif()
  endif()

  if(TC_BUILD_REMOTEFS)
    # object targets that should be included in all APPLE and non-APPLE
    # platforms except for iOS
    set(_TC_COMMON_OBJECTS ${_TC_COMMON_OBJECTS})
    list(APPEND _TC_COMMON_REQUIREMENTS aws-cpp-sdk)
  endif()

endif() # End disable object builds

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

add_subdirectory(src)
add_subdirectory(test)
