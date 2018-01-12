project(Turi)


# We require the most recent version of cmake and automatically
# install the correct version when using the cmake lists
cmake_minimum_required(VERSION 2.8.3)

# Libraries linked via full path no longer produce linker search paths.
cmake_policy(SET CMP0003 NEW)
# Preprocessor definition values are now escaped automatically.
cmake_policy(SET CMP0005 NEW)
# for cmake 3.0
if(${CMAKE_MAJOR_VERSION} GREATER 2)
cmake_policy(SET CMP0045 OLD)
cmake_policy(SET CMP0046 OLD)
cmake_policy(SET CMP0042 NEW)
endif()

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
  
  if (PYTHON_VERSION)
        set(PYTHON_VERSION ${PYTHON_VERSION} CACHE STRING "Target Python Version")
  else()
        set(PYTHON_VERSION "python2.7")
  endif()

  include(UseCython)

else()
  message("Skipping python libraries.")
endif()


include(CheckLibraryExists)
include(CheckFunctionExists)
include(ExternalProject)
include(CheckCXXSourceCompiles)



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

if(WIN32)
  add_definitions(-DWINVER=0x0600)
  add_definitions(-D_WIN32_WINNT=0x0600)
endif()
add_definitions(-DCURL_STATICLIB)
add_definitions(-DIN_TURI_SOURCE_TREE)
add_definitions(-DFUSION_MAX_VECTOR_SIZE=20)
add_definitions(-DBOOST_SPIRIT_THREAD_SAFE)

#**************************************************************************/
#*                                                                        */
#*                          Apple Legal Defines                           */
#*                                                                        */
#**************************************************************************/
add_definitions(-DEIGEN_MPL2_ONLY)

#**************************************************************************/
#*                                                                        */
#*                      Force Disable OpenMP                              */
#*                                                                        */
#**************************************************************************/
add_library(openmp INTERFACE)
target_compile_definitions(openmp INTERFACE __NO_OPENMP__)
message(WARNING "OpenMP Libraries were not found")

#**************************************************************************/
#*                                                                        */
#*             Adapt Compiler and Linker Flags to the system              */
#*                                                                        */
#**************************************************************************/
if (CLANG)
set(CPP11_FLAGS "-std=c++11 -stdlib=libc++ -Wno-deprecated-register -Wno-enum-compare -Wno-conversion-null -Wno-constant-logical-operand -Wno-parentheses-equality -ftemplate-depth=900" CACHE STRING "C++11 enabling flags")
else()
set(CPP11_FLAGS "-std=c++11 -Wno-enum-compare -Wno-conversion-null -ftemplate-depth=900" CACHE STRING "C++11 enabling flags")
endif()
# Shared compiler flags used by all builds (debug, profile, release)
# Allow COMPILER_FLAGS to be used as options from the ./configure without
# forcing all the other options I want to add to be lost
set(C_REAL_COMPILER_FLAGS "-Wall -pthread -g ${COMPILER_FLAGS}" CACHE STRING "common compiler options")
set(CPP_REAL_COMPILER_FLAGS "-Wall -pthread -g ${CPP11_FLAGS} ${COMPILER_FLAGS}" CACHE STRING "common compiler options")

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



include(CheckCXXCompilerFlag)
if (NOT MARCH)
  set(MARCH "x86-64")
endif()

# check if MTUNE is set
if (NOT MTUNE)
  #set to native if supported
  check_cxx_compiler_flag(-mtune=native HAS_MTUNE_NATIVE)
  if(HAS_MTUNE_NATIVE)
    set(MTUNE "native")
  else()
    set(MTUNE "x86-64")
  endif()
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

set(DEBUG_OPTIMIZATION_LEVEL -O0)
set(RELEASE_OPTIMIZATION_LEVEL -O3)

if(WIN32)
        # on windows due to some string table limitations + template
        # insanities, on -O0 we run out of string table entries or something
        # like that.
        set(DEBUG_OPTIMIZATION_LEVEL -O2)
endif()

# Set mac os minimum version
# if (APPLE)
#  set(PLATFORM_FLAGS "-mmacosx-version-min=10.8")
#else()
#  set(PLATFORM_FLAGS "")
#endif()

# Set the debug flags
set(CMAKE_C_FLAGS_DEBUG
  "${DEBUG_OPTIMIZATION_LEVEL} ${PLATFORM_FLAGS} -Wno-attributes -march=${MARCH} -Winit-self ${PROFILING_FLAGS} ${C_REAL_COMPILER_FLAGS} -fno-inline ${ALTERNATE_LINKER} -DHAVE_PTHREAD=1"
  CACHE STRING "compiler options" FORCE)
set(CMAKE_CXX_FLAGS_DEBUG
  "${DEBUG_OPTIMIZATION_LEVEL} ${PLATFORM_FLAGS} ${DISABLE_WARNING_FLAGS} -Wno-attributes -march=${MARCH} -Winit-self ${PROFILING_FLAGS} ${CPP_REAL_COMPILER_FLAGS} ${INT128_FLAGS} ${ALTERNATE_LINKER} -fno-inline -DHAVE_PTHREAD=1"
  CACHE STRING "compiler options" FORCE)

set(CMAKE_C_FLAGS_RELEASE
  "${RELEASE_OPTIMIZATION_LEVEL} ${PLATFORM_FLAGS} -Wno-attributes -march=${MARCH} -mtune=${MTUNE} ${PROFILING_FLAGS} ${C_REAL_COMPILER_FLAGS} -DNDEBUG ${ALTERNATE_LINKER} -DHAVE_PTHREAD=1 -DARMA_NO_DEBUG"
  CACHE STRING "compiler options" FORCE)
set(CMAKE_CXX_FLAGS_RELEASE
  "${RELEASE_OPTIMIZATION_LEVEL} ${PLATFORM_FLAGS} ${EXTRA_OPTIMIZATION_FLAGS} ${DISABLE_WARNING_FLAGS} ${PROFILING_FLAGS} -Wno-attributes -march=${MARCH} -mtune=${MTUNE} ${CPP_REAL_COMPILER_FLAGS} ${INT128_FLAGS} ${ALTERNATE_LINKER} -DNDEBUG -DHAVE_PTHREAD=1 -DARMA_NO_DEBUG"
  CACHE STRING "compiler options" FORCE)


#**************************************************************************/
#*                                                                        */
#*       Some C++ Implementation Oddities between libc++ and stdc++       */
#*                                                                        */
#**************************************************************************/
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
  message(ERROR "Unknown build type: " ${CMAKE_BUILD_TYPE} "!. Rerun ./configure")
endif()








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

add_custom_target(external_dependencies)

include(ExternalProject)
file(GLOB packages "${DEPS_CMAKE}/ExternalProject*.cmake")
foreach(package ${packages})
        message(STATUS "We found local package: ${package}")
        get_filename_component(packagename "${package}" NAME_WE)
        #package is of the form ExternalProjectXXX"
        include(${package})
        STRING(SUBSTRING "${packagename}" 15 -1 depname)
        message(STATUS "We found local package definition: ${depname}")
        string(TOLOWER ${depname} depname)
        set(package_${depname} requires_${depname} CACHE STRING "Package map")
        add_dependencies(external_dependencies ex_${depname})
endforeach()

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
macro(make_target_impl NAME FLAGS REQUIREMENTS IS_LIBRARY SHARED SHARED_ALL_DEFINED)
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
#                 [SHARED] [OUTPUT_NAME xxxx])
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
#
# All other targets which depends on this library (using the "requires" function)
# will automatically include all recursive dependencies.
#
# Boost, pthread is always added as a default dependency. OpenMP is added
# when possible.
macro(make_library NAME)
  set(options SHARED EXTERNAL_VISIBILITY SHARED_ALL_DEFINED DEAD_STRIP)
  set(one_value_args COMPILE_FLAGS OUTPUT_NAME EXPORT_LINUX_MAP_FILE EXPORT_OSX_MAP_FILE)
  set(multi_value_args SOURCES REQUIRES MAC_REQUIRES LINUX_REQUIRES)
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
    message(STATUS "Shared Library: " ${NAME})
    add_library(${NAME} SHARED ${make_library_SOURCES})
  else()
    add_library(${NAME} STATIC ${make_library_SOURCES})
  endif()

  make_target_impl("${NAME}" "${make_library_COMPILE_FLAGS}"
    "${make_library_REQUIRES}" TRUE "${make_library_SHARED}" "${make_library_SHARED_ALL_DEFINED}")

  if (make_library_OUTPUT_NAME)
          message(STATUS "make_library ${NAME} ===> ${make_library_OUTPUT_NAME}")
          set_target_properties(${NAME} PROPERTIES OUTPUT_NAME ${make_library_OUTPUT_NAME})
  endif()

  if (${make_library_EXTERNAL_VISIBILITY})
    # do nothing
    message(STATUS "External Visibility: " ${NAME})
    target_compile_options(${NAME} PRIVATE "-fvisibility=default")
    target_compile_options(${NAME} PRIVATE "-fvisibility-inlines-hidden")
  else()
    target_compile_options(${NAME} PRIVATE "-fvisibility=hidden")
    target_compile_options(${NAME} PRIVATE "-fvisibility-inlines-hidden")
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
      "${make_library_REQUIRES}" TRUE FALSE FALSE)

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
  set(multi_value_args SOURCES REQUIRES MAC_REQUIRES LINUX_REQUIRES)
  CMAKE_PARSE_ARGUMENTS(make_library "" "${one_value_args}" "${multi_value_args}" ${ARGN})
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

  add_executable(${NAME} ${make_library_SOURCES})
  make_target_impl("${NAME}" "${make_library_COMPILE_FLAGS}"
    "${make_library_REQUIRES}" FALSE FALSE FALSE)
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
          list(APPEND all_target_locations ${binary})
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
                                  COMMAND ${CMAKE_COMMAND} -E
                                  copy ${location} ${CMAKE_CURRENT_BINARY_DIR})
                          list(APPEND all_target_locations ${location})
                  endforeach()
          else()
                  add_custom_command(TARGET ${NAME} POST_BUILD
                          COMMENT "copying target of ${target}"
                          COMMAND ${CMAKE_COMMAND} -E
                          copy $<TARGET_FILE:${target}> ${CMAKE_CURRENT_BINARY_DIR})
                  list(APPEND all_target_locations $<TARGET_FILE:${target}>)
          endif()
  endforeach()
  message(STATUS "Adding Copy Target ${NAME} is copying:  ${all_target_locations}")
  set_property(TARGET ${NAME} PROPERTY COPY_FILES "${all_target_locations}")
endmacro()

include(SharedLibraryFromStatic)
include(MergeStaticLibraries)


include_directories(SYSTEM src)
include_directories(SYSTEM src/external)
include_directories(SYSTEM src/old)
include_directories(SYSTEM src/platform)

# for build-time generated source code
include_directories(SYSTEM ${CMAKE_BINARY_DIR}/src)

if(EXISTS ${CMAKE_SOURCE_DIR}/subtree)
  include_directories(SYSTEM subtree)
  add_subdirectory(subtree)
endif()

add_subdirectory(src)
add_subdirectory(test)


#/**************************************************************************/
#/*                                                                        */
#/*                         Python UnitTest Target                         */
#/*                                                                        */
#/**************************************************************************/
ADD_CUSTOM_TARGET(nosetest
  DEPENDS binaries python_source cython_targets
  COMMAND "./run_python_test.sh" ${CMAKE_BUILD_TYPE}
  WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/scripts
  )
