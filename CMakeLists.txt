# We require a recent version of cmake and automatically install a compatible
# version when using the cmake lists, if one is not already present.
cmake_minimum_required(VERSION 3.12.0)

# Libraries linked via full path no longer produce linker search paths.
cmake_policy(SET CMP0003 NEW)
# Preprocessor definition values are now escaped automatically.
cmake_policy(SET CMP0005 NEW)
# quoted string won't be interpreted as variable, unless explicitly
# dereferenced.
cmake_policy(SET CMP0054 NEW)
# for cmake 3.0
cmake_policy(SET CMP0045 NEW)
cmake_policy(SET CMP0046 NEW)
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

list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")

# Configure the compiler
include(SetupCompiler)
setupcompiler()
include(CompilerFlags)

if(${TC_BUILD_IOS})
  add_definitions(-DTC_BUILD_IOS)

  if(${TC_BUILD_REMOTEFS})
    message(ERROR "RemoteFS must be disabled for building iOS.")
  endif()
endif()

if(NOT ${TC_BUILD_REMOTEFS})
  add_compiler_source_define(TC_DISABLE_REMOTEFS)
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

# Compile specif defines for boost.
add_compiler_source_define(FUSION_MAX_VECTOR_SIZE=20)
add_compiler_source_define(BOOST_SPIRIT_THREAD_SAFE)
add_compiler_source_define(BOOST_THREAD_ONCE_ATOMIC)
add_compiler_source_define(BOOST_FUSION_INVOKE_MAX_ARITY=12)
add_compiler_source_define(BOOST_FUSION_INVOKE_PROCEDURE_MAX_ARITY=12)
add_compiler_source_define(BOOST_FUSION_INVOKE_FUNCTION_OBJECT_MAX_ARITY=12)
add_compiler_source_define(BOOST_RESULT_OF_USE_TR1_WITH_DECLTYPE_FALLBACK)

# Legal define for Eigen.
add_compiler_source_define(EIGEN_MPL2_ONLY)

# Defines to avoid symbol collisions
add_compiler_source_define(google=_tc_google)

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
check_and_set_compiler_flag(-Wno-nullability-completeness)
check_and_set_compiler_flag(-Wno-shadow-ivar)
check_and_set_compiler_flag(-fpeel-loops RELEASE)
check_and_set_compiler_flag(-funswitch-loops RELEASE)
check_and_set_compiler_flag(-ftracer RELEASE)

# Turn on ARC globally.
check_and_set_compiler_flag(-fobjc-arc)

# This flag needs to be set after all other warning flags, or it may give false
# positives.
check_and_set_compiler_flag(-Wno-unknown-warning-option)

# Set the different optimization flags
if(DEBUG_OPT_FOR_SIZE)
  set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -Os -g")
  set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -Os -g")
else()
  set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -O0 -fno-inline -g")
  set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -O0 -fno-inline -g")
endif()

if(RELEASE_OPT_FOR_SIZE)
  set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -Os -DNDEBUG")
  set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -Os -DNDEBUG")
else()
  set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -O3 -DNDEBUG")
  set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3 -DNDEBUG")
endif()

# Set up the 128 bit integer support; defines INT128_FLAGS
include(FindInt128)
find_int128_types()

set(_compiler_int128_defs ${INT128_FLAGS})
separate_arguments(_compiler_int128_defs)
list(FILTER _compiler_int128_defs INCLUDE REGEX "^-D.*")
foreach(_def ${_compiler_int128_defs})
  string(REGEX REPLACE "^-D(.*)$" "\\1" _set_arg ${_def})
  add_compiler_source_define(${_set_arg})
endforeach()

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
  add_compiler_source_define(HAVE_PTHREAD)
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
  set(TC_APPLE_DEPENDENCIES
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
    set(TC_APPLE_DEPENDENCIES ${TC_APPLE_DEPENDENCIES} ${APPKIT})
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

link_directories(${CMAKE_SOURCE_DIR}/deps/local/lib)
link_directories(${CMAKE_SOURCE_DIR}/deps/local/lib64)
include_directories(SYSTEM ${CMAKE_SOURCE_DIR}/deps/local/include/)

include_directories(src)
include_directories(${CMAKE_BINARY_DIR}/src)

set(TC_EXTERNAL_INCLUDE_SUBDIRS
    ./
    boost/boost_1_68_0/
    libpng/libpng-1.6.37/
    libjpeg/jpeg-8d/
    bzip2/
    google
    nanomsg/nanomsg-1.0.0/src
    zlib/zlib-1.2.11/
    coremltools_wrap/)

if(${TC_BUILD_REMOTEFS})
  set(TC_EXTERNAL_INCLUDE_SUBDIRS
      ${TC_EXTERNAL_INCLUDE_SUBDIRS} aws-sdk-cpp/aws-cpp-sdk-core/include
      aws-sdk-cpp/aws-cpp-sdk-s3/include)
endif()

foreach(_dir ${TC_EXTERNAL_INCLUDE_SUBDIRS})
  include_directories(SYSTEM ${CMAKE_SOURCE_DIR}/src/external/${_dir})
endforeach()

add_custom_target(external_dependencies)


if(TC_BUILD_REMOTEFS)
  if(NOT TC_EXTERNAL_DEPS_PREBUILT)
    # cmake libs
    include(ExternalProject)
    # Include all the external dependencies.
    file(GLOB packages "${DEPS_CMAKE}/ExternalProject*.cmake")

    foreach(package ${packages})
      get_filename_component(packagename "${package}" NAME_WE)
      # package is of the form ExternalProjectXXX"
      include(${package})
      string(SUBSTRING "${packagename}" 15 -1 depname)
      string(TOLOWER ${depname} depname)
      add_dependencies(external_dependencies ex_${depname})
    endforeach()

    # message(FATAL_ERROR "stop")

  endif()

  set(TC_EXTERNAL_DEPENDENCIES openssl libxml2 curl)
endif()

set(TC_EXTERNAL_DEPENDENCIES ${TC_EXTERNAL_DEPENDENCIES} Threads::Threads
                             ${CMAKE_DL_LIBS})

# Collate all the object targets shared among static and shared library targets
# These are used by C API, unity_shared, etc.

if(NOT TC_DEFAULT_SERVER_INITIALIZER)
  set(TC_DEFAULT_SERVER_INITIALIZER
      "${CMAKE_SOURCE_DIR}/src/capi/default_server_initializer.cpp")
endif()

# ##############################################################################
#
# Write out all the definitions into a common header file.

set(TC_COMMON_HEADER_FILE ${CMAKE_SOURCE_DIR}/src/turi_common.h)

# Write out all the definitions into a single file
separate_arguments(TC_COMPILER_SOURCE_DEFINES)

set(_file_contents
    "
// AUTOGENERATED DEFINITIONS
// DO NOT EDIT
#ifndef TC_CODE_DEFINES
#define TC_CODE_DEFINES

")

foreach(_def ${TC_COMPILER_SOURCE_DEFINES})

  string(REGEX REPLACE "^([^\\=]+)(=|$)((.*)$|)" "\\1" _variable ${_def})
  string(REGEX REPLACE "^([^\\=]+)(=|$)((.*)$|)" "#define \\1 \\3" _expression
                       ${_def})

  set(_file_contents
      "${_file_contents}
#ifndef ${_variable}
${_expression}
#endif
")

endforeach()

set(_file_contents
    "${_file_contents}
#endif // End ifdef TC_CODE_DEFINES
")

file(WRITE ${TC_COMMON_HEADER_FILE} "${_file_contents}")

# ##############################################################################

# Add in src/external as a subdirectory.  The rest of the directories are
# handled directly below
add_subdirectory(src/external)

set(TC_DEPENDENCIES
    z
    libjson
    libjpeg
    libpng
    eigen
    sparsehash
    protobuf
    libbz2
    lz4
    uuid
    xgboost
    coremltools_mlmodel
    boost
    nanomsg)

if(${TC_BUILD_REMOTEFS})
  set(TC_DEPENDENCIES ${TC_DEPENDENCIES} aws-sdk-cpp)
endif()

# ##############################################################################
# Also set up the visualization.  This contains a lot of custom scripts, so we
# preserve it.

add_subdirectory(src/visualization)
set(TC_DEPENDENCIES ${TC_DEPENDENCIES} visualization vega_renderer annotation)

# ##############################################################################
# Add all the framework dependencies

if(APPLE)
  macro(AddFrameworkDependency _name)
    find_library(_fwrk_path_${_name} ${_name} REQUIRED)
    set(TC_DEPENDENCIES ${TC_DEPENDENCIES} "-framework ${_name}")
  endmacro()

  addframeworkdependency(Accelerate)
  addframeworkdependency(CoreGraphics)
  addframeworkdependency(JavaScriptCore)
  addframeworkdependency(Foundation)
  addframeworkdependency(CoreImage)
  addframeworkdependency(CoreML)
  addframeworkdependency(CoreVideo)
  addframeworkdependency(Metal)
  addframeworkdependency(MetalPerformanceShaders)
  if(NOT ${TC_BUILD_IOS})
    addframeworkdependency(AppKit)
  endif()
endif()

file(
  GLOB_RECURSE
  _tc_core_files
  CONFIGURE_DEPENDS
  ${CMAKE_SOURCE_DIR}/src/core/*.h
  ${CMAKE_SOURCE_DIR}/src/core/*.c
  ${CMAKE_SOURCE_DIR}/src/core/*.hpp
  ${CMAKE_SOURCE_DIR}/src/core/*.cpp
  ${CMAKE_SOURCE_DIR}/src/core/*.m
  ${CMAKE_SOURCE_DIR}/src/core/*.mm
  ${CMAKE_SOURCE_DIR}/src/ml/*.h
  ${CMAKE_SOURCE_DIR}/src/ml/*.c
  ${CMAKE_SOURCE_DIR}/src/ml/*.hpp
  ${CMAKE_SOURCE_DIR}/src/ml/*.cpp
  ${CMAKE_SOURCE_DIR}/src/ml/*.m
  ${CMAKE_SOURCE_DIR}/src/ml/*.mm
  ${CMAKE_SOURCE_DIR}/src/model_server/*.h
  ${CMAKE_SOURCE_DIR}/src/model_server/*.c
  ${CMAKE_SOURCE_DIR}/src/model_server/*.hpp
  ${CMAKE_SOURCE_DIR}/src/model_server/*.cpp
  ${CMAKE_SOURCE_DIR}/src/model_server/*.m
  ${CMAKE_SOURCE_DIR}/src/model_server/*.mm
  ${CMAKE_SOURCE_DIR}/src/toolkits/*.h
  ${CMAKE_SOURCE_DIR}/src/toolkits/*.c
  ${CMAKE_SOURCE_DIR}/src/toolkits/*.hpp
  ${CMAKE_SOURCE_DIR}/src/toolkits/*.cpp
  ${CMAKE_SOURCE_DIR}/src/toolkits/*.m
  ${CMAKE_SOURCE_DIR}/src/toolkits/*.mm
  ${CMAKE_SOURCE_DIR}/src/capi/*.h
  ${CMAKE_SOURCE_DIR}/src/capi/*.c
  ${CMAKE_SOURCE_DIR}/src/capi/*.hpp
  ${CMAKE_SOURCE_DIR}/src/capi/*.cpp
  ${CMAKE_SOURCE_DIR}/src/capi/*.m
  ${CMAKE_SOURCE_DIR}/src/capi/*.mm)

# Set up some specific source dependencies.
list(FILTER _tc_core_files EXCLUDE REGEX ".*/model_server/extensions/.*")

add_library(TuriCore SHARED ${_tc_core_files})
target_link_libraries(TuriCore ${TC_DEPENDENCIES} ${TC_EXTERNAL_DEPENDENCIES})
add_dependencies(TuriCore ${TC_EXTERNAL_DEPENDENCIES})

# set_property(TARGET TuriCore APPEND_STRING PROPERTY LINK_FLAGS " -undefined
# dynamic_lookup")
source_group(TREE ${CMAKE_SOURCE_DIR} FILES ${_tc_core_files})

set(TC_TARGET_DIR "${CMAKE_SOURCE_DIR}/targets/")

install(TARGETS TuriCore LIBRARY DESTINATION ${TC_TARGET_DIR}/lib/)
install(
  DIRECTORY src/
  DESTINATION ${TC_TARGET_DIR}/include/
  MESSAGE_NEVER FILES_MATCHING
  REGEX "^.*\.((h)|(hpp)|(ipp)|(sparsehash/.*))$")

install(
  CODE "execute_process( \
    COMMAND ${CMAKE_COMMAND} -E create_symlink \
    ${TC_TARGET_DIR}/include/external/boost/boost_1_68_0/boost \
    ${TC_TARGET_DIR}/include/boost)")

install(
  CODE "execute_process( \
    COMMAND ${CMAKE_COMMAND} -E create_symlink \
    ${TC_TARGET_DIR}/include/external/sparsehash/ \
    ${TC_TARGET_DIR}/include/sparsehash)")
