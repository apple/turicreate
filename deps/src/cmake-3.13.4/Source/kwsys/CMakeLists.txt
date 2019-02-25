# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing#kwsys for details.

# The Kitware System Library is intended to be included in other
# projects.  It is completely configurable in that the library's
# namespace can be configured and the components that are included can
# be selected invididually.

# Typical usage is to import the kwsys directory tree into a
# subdirectory under a parent project and enable the classes that will
# be used.  All classes are disabled by default.  The CMake listfile
# above this one configures the library as follows:
#
#  SET(KWSYS_NAMESPACE foosys)
#  SET(KWSYS_USE_Directory 1)    # Enable Directory class.
#  SUBDIRS(kwsys)
#
# Optional settings are as follows:
#
#  KWSYS_HEADER_ROOT = The directory into which to generate the kwsys headers.
#                      A directory called "${KWSYS_NAMESPACE}" will be
#                      created under this root directory to hold the files.
#  KWSYS_SPLIT_OBJECTS_FROM_INTERFACE
#                    = Instead of creating a single ${KWSYS_NAMESPACE} library
#                      target, create three separate targets:
#                        ${KWSYS_NAMESPACE}
#                          - An INTERFACE library only containing usage
#                            requirements.
#                        ${KWSYS_NAMESPACE}_objects
#                          - An OBJECT library for the built kwsys objects.
#                            Note: This is omitted from the install rules
#                        ${KWSYS_NAMESPACE}_private
#                          - An INTERFACE library combining both that is
#                            appropriate for use with PRIVATE linking in
#                            target_link_libraries. Because of how interface
#                            properties propagate, this target is not suitable
#                            for use with PUBLIC or INTERFACE linking.
#  KWSYS_ALIAS_TARGET = The name of an alias target to create to the actual target.
#
#    Example:
#
#      SET(KWSYS_HEADER_ROOT ${PROJECT_BINARY_DIR})
#      INCLUDE_DIRECTORIES(${PROJECT_BINARY_DIR})
#
# Optional settings to setup install rules are as follows:
#
#  KWSYS_INSTALL_BIN_DIR     = The installation target directories into
#  KWSYS_INSTALL_LIB_DIR       which the libraries and headers from
#  KWSYS_INSTALL_INCLUDE_DIR   kwsys should be installed by a "make install".
#                              The values should be specified relative to
#                              the installation prefix and NOT start with '/'.
#  KWSYS_INSTALL_DOC_DIR     = The installation target directory for documentation
#                              such as copyright information.
#
#  KWSYS_INSTALL_COMPONENT_NAME_RUNTIME     = Name of runtime and development
#  KWSYS_INSTALL_COMPONENT_NAME_DEVELOPMENT   installation components.
#                                             If not given the install rules
#                                             will not be in any component.
#
#  KWSYS_INSTALL_EXPORT_NAME = The EXPORT option value for install(TARGETS) calls.
#
#    Example:
#
#      SET(KWSYS_INSTALL_BIN_DIR bin)
#      SET(KWSYS_INSTALL_LIB_DIR lib)
#      SET(KWSYS_INSTALL_INCLUDE_DIR include)
#      SET(KWSYS_INSTALL_COMPONENT_NAME_RUNTIME Runtime)
#      SET(KWSYS_INSTALL_COMPONENT_NAME_DEVELOPMENT Development)

# Once configured, kwsys should be used as follows from C or C++ code:
#
#  #include <foosys/Directory.hxx>
#   ...
#  foosys::Directory directory;
#

# NOTE: This library is intended for internal use by Kitware-driven
# projects.  In order to keep it simple no attempt will be made to
# maintain backward compatibility when changes are made to KWSys.
# When an incompatible change is made Kitware's projects that use
# KWSys will be fixed, but no notification will necessarily be sent to
# any outside mailing list and no documentation of the change will be
# written.

CMAKE_MINIMUM_REQUIRED(VERSION 2.6.3 FATAL_ERROR)
FOREACH(p
    CMP0022 # CMake 2.8, Define link interface - required by android_mk export
    CMP0025 # CMake 3.0, Compiler id for Apple Clang is now AppleClang.
    CMP0042 # CMake 3.0, MACOSX_RPATH is enabled by default.
    CMP0048 # CMake 3.0, Let the project command manage version variables.
    CMP0056 # CMake 3.2, Honor link flags in try_compile() source-file signature.
    CMP0063 # CMake 3.3, Honor visibility properties for all target types.
    )
  IF(POLICY ${p})
    CMAKE_POLICY(SET ${p} NEW)
  ENDIF()
ENDFOREACH()
SET(CMAKE_LEGACY_CYGWIN_WIN32 0)

IF(CMAKE_VERSION VERSION_LESS 3.0)
  SET(KWSYS_SPLIT_OBJECTS_FROM_INTERFACE 0)
ENDIF()

#-----------------------------------------------------------------------------
# If a namespace is not specified, use "kwsys" and enable testing.
# This should be the case only when kwsys is not included inside
# another project and is being tested.
IF(NOT KWSYS_NAMESPACE)
  SET(KWSYS_NAMESPACE "kwsys")
  SET(KWSYS_STANDALONE 1)
ENDIF()

#-----------------------------------------------------------------------------
# The project name is that of the specified namespace.
PROJECT(${KWSYS_NAMESPACE})

# Tell CMake how to follow dependencies of sources in this directory.
SET_PROPERTY(DIRECTORY
  PROPERTY IMPLICIT_DEPENDS_INCLUDE_TRANSFORM
  "KWSYS_HEADER(%)=<${KWSYS_NAMESPACE}/%>"
  )

# Select library components.
IF(KWSYS_STANDALONE OR CMake_SOURCE_DIR)
  SET(KWSYS_ENABLE_C 1)
  # Enable all components.
  SET(KWSYS_USE_Base64 1)
  SET(KWSYS_USE_Directory 1)
  SET(KWSYS_USE_DynamicLoader 1)
  SET(KWSYS_USE_Encoding 1)
  SET(KWSYS_USE_Glob 1)
  SET(KWSYS_USE_MD5 1)
  SET(KWSYS_USE_Process 1)
  SET(KWSYS_USE_RegularExpression 1)
  SET(KWSYS_USE_System 1)
  SET(KWSYS_USE_SystemTools 1)
  SET(KWSYS_USE_CommandLineArguments 1)
  SET(KWSYS_USE_Terminal 1)
  SET(KWSYS_USE_IOStream 1)
  SET(KWSYS_USE_FStream 1)
  SET(KWSYS_USE_String 1)
  SET(KWSYS_USE_SystemInformation 1)
  SET(KWSYS_USE_ConsoleBuf 1)
ENDIF()

# Enforce component dependencies.
IF(KWSYS_USE_SystemTools)
  SET(KWSYS_USE_Directory 1)
  SET(KWSYS_USE_FStream 1)
  SET(KWSYS_USE_Encoding 1)
ENDIF()
IF(KWSYS_USE_Glob)
  SET(KWSYS_USE_Directory 1)
  SET(KWSYS_USE_SystemTools 1)
  SET(KWSYS_USE_RegularExpression 1)
  SET(KWSYS_USE_FStream 1)
  SET(KWSYS_USE_Encoding 1)
ENDIF()
IF(KWSYS_USE_Process)
  SET(KWSYS_USE_System 1)
  SET(KWSYS_USE_Encoding 1)
ENDIF()
IF(KWSYS_USE_SystemInformation)
  SET(KWSYS_USE_Process 1)
ENDIF()
IF(KWSYS_USE_System)
  SET(KWSYS_USE_Encoding 1)
ENDIF()
IF(KWSYS_USE_Directory)
  SET(KWSYS_USE_Encoding 1)
ENDIF()
IF(KWSYS_USE_FStream)
  SET(KWSYS_USE_Encoding 1)
ENDIF()
IF(KWSYS_USE_ConsoleBuf)
  SET(KWSYS_USE_Encoding 1)
ENDIF()

# Setup the large file support default.
IF(KWSYS_LFS_DISABLE)
  SET(KWSYS_LFS_REQUESTED 0)
ELSE()
  SET(KWSYS_LFS_REQUESTED 1)
ENDIF()

# Specify default 8 bit encoding for Windows
IF(NOT KWSYS_ENCODING_DEFAULT_CODEPAGE)
  SET(KWSYS_ENCODING_DEFAULT_CODEPAGE CP_ACP)
ENDIF()

# Enable testing if building standalone.
IF(KWSYS_STANDALONE)
  INCLUDE(Dart)
  MARK_AS_ADVANCED(BUILD_TESTING DART_ROOT TCL_TCLSH)
  IF(BUILD_TESTING)
    ENABLE_TESTING()
  ENDIF()
ENDIF()

# Choose default shared/static build if not specified.
IF(NOT DEFINED KWSYS_BUILD_SHARED)
  SET(KWSYS_BUILD_SHARED ${BUILD_SHARED_LIBS})
ENDIF()

# Include helper macros.
INCLUDE(${CMAKE_CURRENT_SOURCE_DIR}/kwsysPlatformTests.cmake)
INCLUDE(CheckTypeSize)

# Do full dependency headers.
INCLUDE_REGULAR_EXPRESSION("^.*$")

# Use new KWSYS_INSTALL_*_DIR variable names to control installation.
# Take defaults from the old names.  Note that there was no old name
# for the bin dir, so we take the old lib dir name so DLLs will be
# installed in a compatible way for old code.
IF(NOT KWSYS_INSTALL_INCLUDE_DIR)
  STRING(REGEX REPLACE "^/" "" KWSYS_INSTALL_INCLUDE_DIR
    "${KWSYS_HEADER_INSTALL_DIR}")
ENDIF()
IF(NOT KWSYS_INSTALL_LIB_DIR)
  STRING(REGEX REPLACE "^/" "" KWSYS_INSTALL_LIB_DIR
    "${KWSYS_LIBRARY_INSTALL_DIR}")
ENDIF()
IF(NOT KWSYS_INSTALL_BIN_DIR)
  STRING(REGEX REPLACE "^/" "" KWSYS_INSTALL_BIN_DIR
    "${KWSYS_LIBRARY_INSTALL_DIR}")
ENDIF()

# Setup header install rules.
SET(KWSYS_INSTALL_INCLUDE_OPTIONS)
IF(KWSYS_INSTALL_COMPONENT_NAME_DEVELOPMENT)
  SET(KWSYS_INSTALL_INCLUDE_OPTIONS ${KWSYS_INSTALL_INCLUDE_OPTIONS}
    COMPONENT ${KWSYS_INSTALL_COMPONENT_NAME_DEVELOPMENT}
    )
ENDIF()

# Setup library install rules.
SET(KWSYS_INSTALL_LIBRARY_RULE)
SET(KWSYS_INSTALL_NAMELINK_RULE)
IF(KWSYS_INSTALL_LIB_DIR)
  IF(KWSYS_INSTALL_EXPORT_NAME)
    LIST(APPEND KWSYS_INSTALL_LIBRARY_RULE EXPORT ${KWSYS_INSTALL_EXPORT_NAME})
  ENDIF()
  # Install the shared library to the lib directory.
  SET(KWSYS_INSTALL_LIBRARY_RULE ${KWSYS_INSTALL_LIBRARY_RULE}
    LIBRARY DESTINATION ${KWSYS_INSTALL_LIB_DIR} NAMELINK_SKIP
    )
  # Assign the shared library to the runtime component.
  IF(KWSYS_INSTALL_COMPONENT_NAME_RUNTIME)
    SET(KWSYS_INSTALL_LIBRARY_RULE ${KWSYS_INSTALL_LIBRARY_RULE}
      COMPONENT ${KWSYS_INSTALL_COMPONENT_NAME_RUNTIME}
      )
  ENDIF()
  IF(KWSYS_BUILD_SHARED)
    SET(KWSYS_INSTALL_NAMELINK_RULE ${KWSYS_INSTALL_NAMELINK_RULE}
      LIBRARY DESTINATION ${KWSYS_INSTALL_LIB_DIR} NAMELINK_ONLY
      )
    # Assign the namelink to the development component.
    IF(KWSYS_INSTALL_COMPONENT_NAME_DEVELOPMENT)
      SET(KWSYS_INSTALL_NAMELINK_RULE ${KWSYS_INSTALL_NAMELINK_RULE}
        COMPONENT ${KWSYS_INSTALL_COMPONENT_NAME_DEVELOPMENT}
        )
    ENDIF()
  ENDIF()

  # Install the archive to the lib directory.
  SET(KWSYS_INSTALL_LIBRARY_RULE ${KWSYS_INSTALL_LIBRARY_RULE}
    ARCHIVE DESTINATION ${KWSYS_INSTALL_LIB_DIR}
    )
  # Assign the archive to the development component.
  IF(KWSYS_INSTALL_COMPONENT_NAME_DEVELOPMENT)
    SET(KWSYS_INSTALL_LIBRARY_RULE ${KWSYS_INSTALL_LIBRARY_RULE}
      COMPONENT ${KWSYS_INSTALL_COMPONENT_NAME_DEVELOPMENT}
      )
  ENDIF()
ENDIF()
IF(KWSYS_INSTALL_BIN_DIR)
  # Install the runtime library to the bin directory.
  SET(KWSYS_INSTALL_LIBRARY_RULE ${KWSYS_INSTALL_LIBRARY_RULE}
    RUNTIME DESTINATION ${KWSYS_INSTALL_BIN_DIR}
    )
  # Assign the runtime library to the runtime component.
  IF(KWSYS_INSTALL_COMPONENT_NAME_RUNTIME)
    SET(KWSYS_INSTALL_LIBRARY_RULE ${KWSYS_INSTALL_LIBRARY_RULE}
      COMPONENT ${KWSYS_INSTALL_COMPONENT_NAME_RUNTIME}
      )
  ENDIF()
ENDIF()

# Do not support old KWSYS_*a_INSTALL_DIR variable names.
SET(KWSYS_HEADER_INSTALL_DIR)
SET(KWSYS_LIBRARY_INSTALL_DIR)

# Generated source files will need this header.
STRING(COMPARE EQUAL "${PROJECT_SOURCE_DIR}" "${PROJECT_BINARY_DIR}"
  KWSYS_IN_SOURCE_BUILD)
IF(NOT KWSYS_IN_SOURCE_BUILD)
  CONFIGURE_FILE(${PROJECT_SOURCE_DIR}/kwsysPrivate.h
    ${PROJECT_BINARY_DIR}/kwsysPrivate.h COPYONLY IMMEDIATE)
ENDIF()

# Select plugin module file name convention.
IF(NOT KWSYS_DynamicLoader_PREFIX)
  SET(KWSYS_DynamicLoader_PREFIX ${CMAKE_SHARED_MODULE_PREFIX})
ENDIF()
IF(NOT KWSYS_DynamicLoader_SUFFIX)
  SET(KWSYS_DynamicLoader_SUFFIX ${CMAKE_SHARED_MODULE_SUFFIX})
ENDIF()

#-----------------------------------------------------------------------------
# We require ANSI support from the C compiler.  Add any needed flags.
IF(CMAKE_ANSI_CFLAGS)
  SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${CMAKE_ANSI_CFLAGS}")
ENDIF()

#-----------------------------------------------------------------------------
# Adjust compiler flags for some platforms.
IF(NOT CMAKE_COMPILER_IS_GNUCXX)
  IF(CMAKE_SYSTEM MATCHES "OSF1-V.*")
    STRING(REGEX MATCH "-timplicit_local"
      KWSYS_CXX_FLAGS_HAVE_IMPLICIT_LOCAL "${CMAKE_CXX_FLAGS}")
    STRING(REGEX MATCH "-no_implicit_include"
      KWSYS_CXX_FLAGS_HAVE_NO_IMPLICIT_INCLUDE "${CMAKE_CXX_FLAGS}")
    IF(NOT KWSYS_CXX_FLAGS_HAVE_IMPLICIT_LOCAL)
      SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -timplicit_local")
    ENDIF()
    IF(NOT KWSYS_CXX_FLAGS_HAVE_NO_IMPLICIT_INCLUDE)
      SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -no_implicit_include")
    ENDIF()
  ENDIF()
  IF(CMAKE_SYSTEM MATCHES "HP-UX")
    SET(KWSYS_PLATFORM_CXX_TEST_EXTRA_FLAGS "+p")
    IF(CMAKE_CXX_COMPILER_ID MATCHES "HP")
      # it is known that version 3.85 fails and 6.25 works without these flags
      IF(CMAKE_CXX_COMPILER_VERSION VERSION_LESS 4)
        # use new C++ library and improved template support
        SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -AA +hpxstd98")
      ENDIF()
    ENDIF()
  ENDIF()
ENDIF()
IF(KWSYS_STANDALONE)
  IF(CMAKE_CXX_COMPILER_ID STREQUAL SunPro)
    IF(NOT CMAKE_CXX_COMPILER_VERSION VERSION_LESS 5.13)
      SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++03")
    ELSE()
      SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -library=stlport4")
    ENDIF()
  ENDIF()
ENDIF()

#-----------------------------------------------------------------------------
# Configure Large File Support.
KWSYS_PLATFORM_CXX_TEST(KWSYS_CXX_HAS_CSTDIO
  "Checking whether header cstdio is available" DIRECT)
SET(KWSYS_LFS_AVAILABLE 0)
IF(KWSYS_LFS_REQUESTED)
  # Large File Support is requested.
  SET(KWSYS_LFS_REQUESTED 1)

  # Check for large file support.
  SET(KWSYS_PLATFORM_CXX_TEST_DEFINES
    -DKWSYS_CXX_HAS_CSTDIO=${KWSYS_CXX_HAS_CSTDIO})
  KWSYS_PLATFORM_CXX_TEST_RUN(KWSYS_LFS_WORKS
    "Checking for Large File Support" DIRECT)
  SET(KWSYS_PLATFORM_CXX_TEST_DEFINES)

  IF(KWSYS_LFS_WORKS)
    SET(KWSYS_LFS_AVAILABLE 1)
  ENDIF()
ELSE()
  # Large File Support is not requested.
  SET(KWSYS_LFS_REQUESTED 0)
ENDIF()

#-----------------------------------------------------------------------------
# Configure the standard library header wrappers based on compiler's
# capabilities and parent project's request.  Enforce 0/1 as only
# possible values for configuration into Configure.hxx.

# Check existence and uniqueness of long long and __int64.
KWSYS_PLATFORM_CXX_TEST(KWSYS_CXX_HAS_LONG_LONG
  "Checking whether C++ compiler has 'long long'" DIRECT)
KWSYS_PLATFORM_CXX_TEST(KWSYS_CXX_HAS___INT64
  "Checking whether C++ compiler has '__int64'" DIRECT)
IF(KWSYS_CXX_HAS___INT64)
  KWSYS_PLATFORM_CXX_TEST(KWSYS_CXX_SAME_LONG_AND___INT64
    "Checking whether long and __int64 are the same type" DIRECT)
  IF(KWSYS_CXX_HAS_LONG_LONG)
    KWSYS_PLATFORM_CXX_TEST(KWSYS_CXX_SAME_LONG_LONG_AND___INT64
      "Checking whether long long and __int64 are the same type" DIRECT)
  ENDIF()
ENDIF()

# Enable the "long long" type if it is available.  It is standard in
# C99 and C++03 but not in earlier standards.
IF(KWSYS_CXX_HAS_LONG_LONG)
  SET(KWSYS_USE_LONG_LONG 1)
ELSE()
  SET(KWSYS_USE_LONG_LONG 0)
ENDIF()

# Enable the "__int64" type if it is available and unique.  It is not
# standard.
SET(KWSYS_USE___INT64 0)
IF(KWSYS_CXX_HAS___INT64)
  IF(NOT KWSYS_CXX_SAME_LONG_AND___INT64)
    IF(NOT KWSYS_CXX_SAME_LONG_LONG_AND___INT64)
      SET(KWSYS_USE___INT64 1)
    ENDIF()
  ENDIF()
ENDIF()

IF(KWSYS_USE_Encoding)
  # Look for type size helper macros.
  KWSYS_PLATFORM_CXX_TEST(KWSYS_STL_HAS_WSTRING
    "Checking whether wstring is available" DIRECT)
ENDIF()

IF(KWSYS_USE_IOStream)
  # Determine whether iostreams support long long.
  IF(KWSYS_CXX_HAS_LONG_LONG)
    KWSYS_PLATFORM_CXX_TEST(KWSYS_IOS_HAS_ISTREAM_LONG_LONG
      "Checking if istream supports long long" DIRECT)
    KWSYS_PLATFORM_CXX_TEST(KWSYS_IOS_HAS_OSTREAM_LONG_LONG
      "Checking if ostream supports long long" DIRECT)
  ELSE()
    SET(KWSYS_IOS_HAS_ISTREAM_LONG_LONG 0)
    SET(KWSYS_IOS_HAS_OSTREAM_LONG_LONG 0)
  ENDIF()
  IF(KWSYS_CXX_HAS___INT64)
    KWSYS_PLATFORM_CXX_TEST(KWSYS_IOS_HAS_ISTREAM___INT64
      "Checking if istream supports __int64" DIRECT)
    KWSYS_PLATFORM_CXX_TEST(KWSYS_IOS_HAS_OSTREAM___INT64
      "Checking if ostream supports __int64" DIRECT)
  ELSE()
    SET(KWSYS_IOS_HAS_ISTREAM___INT64 0)
    SET(KWSYS_IOS_HAS_OSTREAM___INT64 0)
  ENDIF()
ENDIF()

IF(KWSYS_NAMESPACE MATCHES "^kwsys$")
  SET(KWSYS_NAME_IS_KWSYS 1)
ELSE()
  SET(KWSYS_NAME_IS_KWSYS 0)
ENDIF()

IF(KWSYS_BUILD_SHARED)
  SET(KWSYS_BUILD_SHARED 1)
  SET(KWSYS_LIBRARY_TYPE SHARED)
ELSE()
  SET(KWSYS_BUILD_SHARED 0)
  SET(KWSYS_LIBRARY_TYPE STATIC)
ENDIF()

#-----------------------------------------------------------------------------
# Configure some implementation details.

KWSYS_PLATFORM_C_TEST(KWSYS_C_HAS_PTRDIFF_T
  "Checking whether C compiler has ptrdiff_t in stddef.h" DIRECT)
KWSYS_PLATFORM_C_TEST(KWSYS_C_HAS_SSIZE_T
  "Checking whether C compiler has ssize_t in unistd.h" DIRECT)
IF(KWSYS_USE_Process)
  KWSYS_PLATFORM_C_TEST(KWSYS_C_HAS_CLOCK_GETTIME_MONOTONIC
    "Checking whether C compiler has clock_gettime" DIRECT)
ENDIF()

SET_SOURCE_FILES_PROPERTIES(ProcessUNIX.c System.c PROPERTIES
  COMPILE_FLAGS "-DKWSYS_C_HAS_PTRDIFF_T=${KWSYS_C_HAS_PTRDIFF_T} -DKWSYS_C_HAS_SSIZE_T=${KWSYS_C_HAS_SSIZE_T} -DKWSYS_C_HAS_CLOCK_GETTIME_MONOTONIC=${KWSYS_C_HAS_CLOCK_GETTIME_MONOTONIC}"
  )

IF(DEFINED KWSYS_PROCESS_USE_SELECT)
  GET_PROPERTY(ProcessUNIX_FLAGS SOURCE ProcessUNIX.c PROPERTY COMPILE_FLAGS)
  SET_PROPERTY(SOURCE ProcessUNIX.c PROPERTY COMPILE_FLAGS "${ProcessUNIX_FLAGS} -DKWSYSPE_USE_SELECT=${KWSYSPE_USE_SELECT}")
ENDIF()

IF(KWSYS_USE_DynamicLoader)
  GET_PROPERTY(KWSYS_SUPPORTS_SHARED_LIBS GLOBAL PROPERTY TARGET_SUPPORTS_SHARED_LIBS)
  IF(KWSYS_SUPPORTS_SHARED_LIBS)
    SET(KWSYS_SUPPORTS_SHARED_LIBS 1)
  ELSE()
    SET(KWSYS_SUPPORTS_SHARED_LIBS 0)
  ENDIF()
  SET_PROPERTY(SOURCE DynamicLoader.cxx APPEND PROPERTY COMPILE_DEFINITIONS
    KWSYS_SUPPORTS_SHARED_LIBS=${KWSYS_SUPPORTS_SHARED_LIBS})
ENDIF()

IF(KWSYS_USE_SystemTools)
  KWSYS_PLATFORM_CXX_TEST(KWSYS_CXX_HAS_SETENV
    "Checking whether CXX compiler has setenv" DIRECT)
  KWSYS_PLATFORM_CXX_TEST(KWSYS_CXX_HAS_UNSETENV
    "Checking whether CXX compiler has unsetenv" DIRECT)
  KWSYS_PLATFORM_CXX_TEST(KWSYS_CXX_HAS_ENVIRON_IN_STDLIB_H
    "Checking whether CXX compiler has environ in stdlib.h" DIRECT)
  KWSYS_PLATFORM_CXX_TEST(KWSYS_CXX_HAS_UTIMES
    "Checking whether CXX compiler has utimes" DIRECT)
  KWSYS_PLATFORM_CXX_TEST(KWSYS_CXX_HAS_UTIMENSAT
    "Checking whether CXX compiler has utimensat" DIRECT)
  KWSYS_PLATFORM_CXX_TEST(KWSYS_CXX_STAT_HAS_ST_MTIM
    "Checking whether CXX compiler struct stat has st_mtim member" DIRECT)
  KWSYS_PLATFORM_CXX_TEST(KWSYS_CXX_STAT_HAS_ST_MTIMESPEC
    "Checking whether CXX compiler struct stat has st_mtimespec member" DIRECT)
  SET_PROPERTY(SOURCE SystemTools.cxx APPEND PROPERTY COMPILE_DEFINITIONS
    KWSYS_CXX_HAS_SETENV=${KWSYS_CXX_HAS_SETENV}
    KWSYS_CXX_HAS_UNSETENV=${KWSYS_CXX_HAS_UNSETENV}
    KWSYS_CXX_HAS_ENVIRON_IN_STDLIB_H=${KWSYS_CXX_HAS_ENVIRON_IN_STDLIB_H}
    KWSYS_CXX_HAS_UTIMES=${KWSYS_CXX_HAS_UTIMES}
    KWSYS_CXX_HAS_UTIMENSAT=${KWSYS_CXX_HAS_UTIMENSAT}
    KWSYS_CXX_STAT_HAS_ST_MTIM=${KWSYS_CXX_STAT_HAS_ST_MTIM}
    KWSYS_CXX_STAT_HAS_ST_MTIMESPEC=${KWSYS_CXX_STAT_HAS_ST_MTIMESPEC}
    )
  IF(NOT WIN32)
    IF(KWSYS_STANDALONE)
      OPTION(KWSYS_SYSTEMTOOLS_SUPPORT_WINDOWS_SLASHES "If true, Windows paths will be supported on Unix as well" ON)
    ENDIF()
    IF(KWSYS_SYSTEMTOOLS_SUPPORT_WINDOWS_SLASHES)
      SET_PROPERTY(SOURCE SystemTools.cxx testSystemTools.cxx APPEND PROPERTY COMPILE_DEFINITIONS
        KWSYS_SYSTEMTOOLS_SUPPORT_WINDOWS_SLASHES
        )
    ENDIF()
  ENDIF()

  # Disable getpwnam for static linux builds since it depends on shared glibc
  GET_PROPERTY(SHARED_LIBS_SUPPORTED GLOBAL PROPERTY TARGET_SUPPORTS_SHARED_LIBS)
  IF(CMAKE_SYSTEM_NAME MATCHES "Linux" AND NOT SHARED_LIBS_SUPPORTED)
    SET_PROPERTY(SOURCE SystemTools.cxx APPEND PROPERTY COMPILE_DEFINITIONS
      HAVE_GETPWNAM=0
      )
  ENDIF()
ENDIF()

IF(KWSYS_USE_SystemInformation)
  SET_PROPERTY(SOURCE SystemInformation.cxx APPEND PROPERTY
    COMPILE_DEFINITIONS SIZEOF_VOID_P=${CMAKE_SIZEOF_VOID_P})
  IF(NOT CYGWIN)
    INCLUDE(CheckIncludeFiles)
    CHECK_INCLUDE_FILES("sys/types.h;ifaddrs.h" KWSYS_SYS_HAS_IFADDRS_H)
    IF(KWSYS_SYS_HAS_IFADDRS_H)
      SET_PROPERTY(SOURCE SystemInformation.cxx APPEND PROPERTY
        COMPILE_DEFINITIONS KWSYS_SYS_HAS_IFADDRS_H=1)
    ENDIF()
  ENDIF()
  IF(WIN32)
    INCLUDE(CheckSymbolExists)
    SET(CMAKE_REQUIRED_LIBRARIES Psapi)
    CHECK_SYMBOL_EXISTS(GetProcessMemoryInfo "windows.h;psapi.h" KWSYS_SYS_HAS_PSAPI)
    UNSET(CMAKE_REQUIRED_LIBRARIES)
    IF(KWSYS_SYS_HAS_PSAPI)
      SET_PROPERTY(SOURCE SystemInformation.cxx APPEND PROPERTY
        COMPILE_DEFINITIONS KWSYS_SYS_HAS_PSAPI=1)
      IF(MSVC70 OR MSVC71)
        # Suppress LNK4089: all references to 'PSAPI.DLL' discarded by /OPT:REF
        SET(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /IGNORE:4089")
      ENDIF()
    ENDIF()
  ENDIF()
  IF(CMAKE_SYSTEM MATCHES "HP-UX")
    CHECK_INCLUDE_FILES("sys/mpctl.h" KWSYS_SYS_HAS_MPCTL_H)
    IF(KWSYS_SYS_HAS_MPCTL_H)
      SET_PROPERTY(SOURCE SystemInformation.cxx APPEND PROPERTY
        COMPILE_DEFINITIONS KWSYS_SYS_HAS_MPCTL_H=1)
    ENDIF()
  ENDIF()
  IF(CMAKE_SYSTEM MATCHES "BSD")
    CHECK_INCLUDE_FILES("machine/cpu.h" KWSYS_SYS_HAS_MACHINE_CPU_H)
    IF(KWSYS_SYS_HAS_MACHINE_CPU_H)
      SET_PROPERTY(SOURCE SystemInformation.cxx APPEND PROPERTY
        COMPILE_DEFINITIONS KWSYS_SYS_HAS_MACHINE_CPU_H=1)
    ENDIF()
  ENDIF()
  IF(KWSYS_LFS_AVAILABLE AND NOT KWSYS_LFS_DISABLE)
    SET(KWSYS_PLATFORM_CXX_TEST_DEFINES -DKWSYS_HAS_LFS=1)
  ENDIF()
  KWSYS_PLATFORM_CXX_TEST(KWSYS_CXX_HAS_RLIMIT64
    "Checking whether CXX compiler has rlimit64" DIRECT)
  SET(KWSYS_PLATFORM_CXX_TEST_DEFINES)
  IF(KWSYS_CXX_HAS_RLIMIT64)
    SET_PROPERTY(SOURCE SystemInformation.cxx APPEND PROPERTY
      COMPILE_DEFINITIONS KWSYS_CXX_HAS_RLIMIT64=1)
  ENDIF()
  KWSYS_PLATFORM_CXX_TEST(KWSYS_CXX_HAS_ATOL
    "Checking whether CXX compiler has atol" DIRECT)
  IF(KWSYS_CXX_HAS_ATOL)
    SET_PROPERTY(SOURCE SystemInformation.cxx APPEND PROPERTY
      COMPILE_DEFINITIONS KWSYS_CXX_HAS_ATOL=1)
  ENDIF()
  KWSYS_PLATFORM_CXX_TEST(KWSYS_CXX_HAS_ATOLL
    "Checking whether CXX compiler has atoll" DIRECT)
  IF(KWSYS_CXX_HAS_ATOLL)
    SET_PROPERTY(SOURCE SystemInformation.cxx APPEND PROPERTY
      COMPILE_DEFINITIONS KWSYS_CXX_HAS_ATOLL=1)
  ENDIF()
  KWSYS_PLATFORM_CXX_TEST(KWSYS_CXX_HAS__ATOI64
    "Checking whether CXX compiler has _atoi64" DIRECT)
  IF(KWSYS_CXX_HAS__ATOI64)
    SET_PROPERTY(SOURCE SystemInformation.cxx APPEND PROPERTY
      COMPILE_DEFINITIONS KWSYS_CXX_HAS__ATOI64=1)
  ENDIF()
  IF(UNIX)
    INCLUDE(CheckIncludeFileCXX)
    # check for simple stack trace
    # usually it's in libc but on FreeBSD
    # it's in libexecinfo
    FIND_LIBRARY(EXECINFO_LIB "execinfo")
    MARK_AS_ADVANCED(EXECINFO_LIB)
    IF (NOT EXECINFO_LIB)
      SET(EXECINFO_LIB "")
    ENDIF()
    CHECK_INCLUDE_FILE_CXX("execinfo.h" KWSYS_CXX_HAS_EXECINFOH)
    IF (KWSYS_CXX_HAS_EXECINFOH)
      # we have the backtrace header check if it
      # can be used with this compiler
      SET(KWSYS_PLATFORM_CXX_TEST_LINK_LIBRARIES ${EXECINFO_LIB})
      KWSYS_PLATFORM_CXX_TEST(KWSYS_CXX_HAS_BACKTRACE
         "Checking whether backtrace works with this C++ compiler" DIRECT)
      SET(KWSYS_PLATFORM_CXX_TEST_LINK_LIBRARIES)
      IF (KWSYS_CXX_HAS_BACKTRACE)
        # backtrace is supported by this system and compiler.
        # now check for the more advanced capabilities.
        SET_PROPERTY(SOURCE SystemInformation.cxx APPEND PROPERTY
          COMPILE_DEFINITIONS KWSYS_SYSTEMINFORMATION_HAS_BACKTRACE=1)
        # check for symbol lookup using dladdr
        CHECK_INCLUDE_FILE_CXX("dlfcn.h" KWSYS_CXX_HAS_DLFCNH)
        IF (KWSYS_CXX_HAS_DLFCNH)
          # we have symbol lookup libraries and headers
          # check if they can be used with this compiler
          SET(KWSYS_PLATFORM_CXX_TEST_LINK_LIBRARIES ${CMAKE_DL_LIBS})
            KWSYS_PLATFORM_CXX_TEST(KWSYS_CXX_HAS_DLADDR
            "Checking whether dladdr works with this C++ compiler" DIRECT)
          SET(KWSYS_PLATFORM_CXX_TEST_LINK_LIBRARIES)
          IF (KWSYS_CXX_HAS_DLADDR)
            # symbol lookup is supported by this system
            # and compiler.
            SET_PROPERTY(SOURCE SystemInformation.cxx APPEND PROPERTY
              COMPILE_DEFINITIONS KWSYS_SYSTEMINFORMATION_HAS_SYMBOL_LOOKUP=1)
          ENDIF()
        ENDIF()
        # c++ demangling support
        # check for cxxabi headers
        CHECK_INCLUDE_FILE_CXX("cxxabi.h" KWSYS_CXX_HAS_CXXABIH)
        IF (KWSYS_CXX_HAS_CXXABIH)
          # check if cxxabi can be used with this
          # system and compiler.
          KWSYS_PLATFORM_CXX_TEST(KWSYS_CXX_HAS_CXXABI
            "Checking whether cxxabi works with this C++ compiler" DIRECT)
          IF (KWSYS_CXX_HAS_CXXABI)
            # c++ demangle using cxxabi is supported with
            # this system and compiler
            SET_PROPERTY(SOURCE SystemInformation.cxx APPEND PROPERTY
              COMPILE_DEFINITIONS KWSYS_SYSTEMINFORMATION_HAS_CPP_DEMANGLE=1)
          ENDIF()
        ENDIF()
        # basic backtrace works better with release build
        # don't bother with advanced features for release
        SET_PROPERTY(SOURCE SystemInformation.cxx APPEND PROPERTY
          COMPILE_DEFINITIONS_DEBUG KWSYS_SYSTEMINFORMATION_HAS_DEBUG_BUILD=1)
        SET_PROPERTY(SOURCE SystemInformation.cxx APPEND PROPERTY
          COMPILE_DEFINITIONS_RELWITHDEBINFO KWSYS_SYSTEMINFORMATION_HAS_DEBUG_BUILD=1)
      ENDIF()
    ENDIF()
  ENDIF()
  IF(BORLAND)
    KWSYS_PLATFORM_CXX_TEST(KWSYS_CXX_HAS_BORLAND_ASM
      "Checking whether Borland CXX compiler supports assembler instructions" DIRECT)
    IF(KWSYS_CXX_HAS_BORLAND_ASM)
      SET_PROPERTY(SOURCE SystemInformation.cxx APPEND PROPERTY
        COMPILE_DEFINITIONS KWSYS_CXX_HAS_BORLAND_ASM=1)
      KWSYS_PLATFORM_CXX_TEST(KWSYS_CXX_HAS_BORLAND_ASM_CPUID
        "Checking whether Borland CXX compiler supports CPUID assembler instruction" DIRECT)
      IF(KWSYS_CXX_HAS_BORLAND_ASM_CPUID)
        SET_PROPERTY(SOURCE SystemInformation.cxx APPEND PROPERTY
          COMPILE_DEFINITIONS KWSYS_CXX_HAS_BORLAND_ASM_CPUID=1)
      ENDIF()
    ENDIF()
  ENDIF()
  IF(KWSYS_USE___INT64)
    SET_PROPERTY(SOURCE SystemInformation.cxx testSystemInformation.cxx APPEND PROPERTY
      COMPILE_DEFINITIONS KWSYS_USE___INT64=1)
  ENDIF()
  IF(KWSYS_USE_LONG_LONG)
    SET_PROPERTY(SOURCE SystemInformation.cxx testSystemInformation.cxx APPEND PROPERTY
      COMPILE_DEFINITIONS KWSYS_USE_LONG_LONG=1)
  ENDIF()
  IF(KWSYS_IOS_HAS_OSTREAM_LONG_LONG)
    SET_PROPERTY(SOURCE SystemInformation.cxx testSystemInformation.cxx APPEND PROPERTY
      COMPILE_DEFINITIONS KWSYS_IOS_HAS_OSTREAM_LONG_LONG=1)
  ENDIF()
  IF(KWSYS_IOS_HAS_OSTREAM___INT64)
    SET_PROPERTY(SOURCE SystemInformation.cxx testSystemInformation.cxx APPEND PROPERTY
      COMPILE_DEFINITIONS KWSYS_IOS_HAS_OSTREAM___INT64=1)
  ENDIF()
  IF(KWSYS_BUILD_SHARED)
    SET_PROPERTY(SOURCE SystemInformation.cxx APPEND PROPERTY
      COMPILE_DEFINITIONS KWSYS_BUILD_SHARED=1)
  ENDIF()

  IF(UNIX AND NOT CYGWIN)
    KWSYS_PLATFORM_CXX_TEST(KWSYS_CXX_HAS_GETLOADAVG
      "Checking whether CXX compiler has getloadavg" DIRECT)
    IF(KWSYS_CXX_HAS_GETLOADAVG)
      SET_PROPERTY(SOURCE SystemInformation.cxx APPEND PROPERTY
        COMPILE_DEFINITIONS KWSYS_CXX_HAS_GETLOADAVG=1)
    ENDIF()
  ENDIF()
ENDIF()

IF(KWSYS_USE_FStream)
  KWSYS_PLATFORM_CXX_TEST(KWSYS_CXX_HAS_EXT_STDIO_FILEBUF_H
    "Checking whether <ext/stdio_filebuf.h> is available" DIRECT)
ENDIF()

#-----------------------------------------------------------------------------
# Choose a directory for the generated headers.
IF(NOT KWSYS_HEADER_ROOT)
  SET(KWSYS_HEADER_ROOT "${PROJECT_BINARY_DIR}")
ENDIF()
SET(KWSYS_HEADER_DIR "${KWSYS_HEADER_ROOT}/${KWSYS_NAMESPACE}")
INCLUDE_DIRECTORIES(${KWSYS_HEADER_ROOT})

#-----------------------------------------------------------------------------
IF(KWSYS_INSTALL_DOC_DIR)
  # Assign the license to the runtime component since it must be
  # distributed with binary forms of this software.
  IF(KWSYS_INSTALL_COMPONENT_NAME_RUNTIME)
    SET(KWSYS_INSTALL_LICENSE_OPTIONS ${KWSYS_INSTALL_LICENSE_OPTIONS}
      COMPONENT ${KWSYS_INSTALL_COMPONENT_NAME_RUNTIME}
      )
  ENDIF()

  # Install the license under the documentation directory.
  INSTALL(FILES ${CMAKE_CURRENT_SOURCE_DIR}/Copyright.txt
    DESTINATION ${KWSYS_INSTALL_DOC_DIR}/${KWSYS_NAMESPACE}
    ${KWSYS_INSTALL_LICENSE_OPTIONS})
ENDIF()

#-----------------------------------------------------------------------------
# Build a list of classes and headers we need to implement the
# selected components.  Initialize with required components.
SET(KWSYS_CLASSES)
SET(KWSYS_H_FILES Configure SharedForward)
SET(KWSYS_HXX_FILES Configure String)

IF(NOT CMake_SOURCE_DIR)
  SET(KWSYS_HXX_FILES ${KWSYS_HXX_FILES}
    hashtable hash_fun hash_map hash_set
    )
ENDIF()

# Add selected C++ classes.
SET(cppclasses
  Directory DynamicLoader Encoding Glob RegularExpression SystemTools
  CommandLineArguments IOStream FStream SystemInformation ConsoleBuf
  )
FOREACH(cpp ${cppclasses})
  IF(KWSYS_USE_${cpp})
    # Use the corresponding class.
    SET(KWSYS_CLASSES ${KWSYS_CLASSES} ${cpp})

    # Load component-specific CMake code.
    IF(EXISTS ${PROJECT_SOURCE_DIR}/kwsys${cpp}.cmake)
      INCLUDE(${PROJECT_SOURCE_DIR}/kwsys${cpp}.cmake)
    ENDIF()
  ENDIF()
ENDFOREACH()

# Add selected C components.
FOREACH(c
    Process Base64 Encoding MD5 Terminal System String
    )
  IF(KWSYS_USE_${c})
    # Use the corresponding header file.
    SET(KWSYS_H_FILES ${KWSYS_H_FILES} ${c})

    # Load component-specific CMake code.
    IF(EXISTS ${PROJECT_SOURCE_DIR}/kwsys${c}.cmake)
      INCLUDE(${PROJECT_SOURCE_DIR}/kwsys${c}.cmake)
    ENDIF()
  ENDIF()
ENDFOREACH()

#-----------------------------------------------------------------------------
# Build a list of sources for the library based on components that are
# included.
SET(KWSYS_C_SRCS)
SET(KWSYS_CXX_SRCS)

# Add the proper sources for this platform's Process implementation.
IF(KWSYS_USE_Process)
  IF(NOT UNIX)
    # Use the Windows implementation.
    SET(KWSYS_C_SRCS ${KWSYS_C_SRCS} ProcessWin32.c)
  ELSE()
    # Use the UNIX implementation.
    SET(KWSYS_C_SRCS ${KWSYS_C_SRCS} ProcessUNIX.c)
  ENDIF()
ENDIF()

# Add selected C sources.
FOREACH(c Base64 Encoding MD5 Terminal System String)
  IF(KWSYS_USE_${c})
    IF(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/${c}C.c)
      LIST(APPEND KWSYS_C_SRCS ${c}C.c)
    ELSE()
      LIST(APPEND KWSYS_C_SRCS ${c}.c)
    ENDIF()
  ENDIF()
ENDFOREACH()

# Configure headers of C++ classes and construct the list of sources.
FOREACH(c ${KWSYS_CLASSES})
  # Add this source to the list of source files for the library.
  IF(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/${c}CXX.cxx)
    LIST(APPEND KWSYS_CXX_SRCS ${c}CXX.cxx)
  ELSEIF(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/${c}.cxx)
    LIST(APPEND KWSYS_CXX_SRCS ${c}.cxx)
  ENDIF()

  # Configure the header for this class.
  CONFIGURE_FILE(${PROJECT_SOURCE_DIR}/${c}.hxx.in ${KWSYS_HEADER_DIR}/${c}.hxx
                 @ONLY IMMEDIATE)
  SET(KWSYS_CXX_SRCS ${KWSYS_CXX_SRCS} ${KWSYS_HEADER_DIR}/${c}.hxx)

  # Create an install target for the header.
  IF(KWSYS_INSTALL_INCLUDE_DIR)
    INSTALL(FILES ${KWSYS_HEADER_DIR}/${c}.hxx
      DESTINATION ${KWSYS_INSTALL_INCLUDE_DIR}/${KWSYS_NAMESPACE}
      ${KWSYS_INSTALL_INCLUDE_OPTIONS})
  ENDIF()
ENDFOREACH()

# Configure C headers.
FOREACH(h ${KWSYS_H_FILES})
  # Configure the header into the given directory.
  CONFIGURE_FILE(${PROJECT_SOURCE_DIR}/${h}.h.in ${KWSYS_HEADER_DIR}/${h}.h
                 @ONLY IMMEDIATE)
  SET(KWSYS_C_SRCS ${KWSYS_C_SRCS} ${KWSYS_HEADER_DIR}/${h}.h)

  # Create an install target for the header.
  IF(KWSYS_INSTALL_INCLUDE_DIR)
    INSTALL(FILES ${KWSYS_HEADER_DIR}/${h}.h
      DESTINATION ${KWSYS_INSTALL_INCLUDE_DIR}/${KWSYS_NAMESPACE}
      ${KWSYS_INSTALL_INCLUDE_OPTIONS})
  ENDIF()
ENDFOREACH()

# Configure other C++ headers.
FOREACH(h ${KWSYS_HXX_FILES})
  # Configure the header into the given directory.
  CONFIGURE_FILE(${PROJECT_SOURCE_DIR}/${h}.hxx.in ${KWSYS_HEADER_DIR}/${h}.hxx
                 @ONLY IMMEDIATE)
  SET(KWSYS_CXX_SRCS ${KWSYS_CXX_SRCS} ${KWSYS_HEADER_DIR}/${h}.hxx)

  # Create an install target for the header.
  IF(KWSYS_INSTALL_INCLUDE_DIR)
    INSTALL(FILES ${KWSYS_HEADER_DIR}/${h}.hxx
      DESTINATION ${KWSYS_INSTALL_INCLUDE_DIR}/${KWSYS_NAMESPACE}
      ${KWSYS_INSTALL_INCLUDE_OPTIONS})
  ENDIF()
ENDFOREACH()

#-----------------------------------------------------------------------------
# Add the library with the configured name and list of sources.
IF(KWSYS_C_SRCS OR KWSYS_CXX_SRCS)
  IF(KWSYS_SPLIT_OBJECTS_FROM_INTERFACE)
    SET(KWSYS_TARGET_INTERFACE ${KWSYS_NAMESPACE})
    SET(KWSYS_TARGET_OBJECT ${KWSYS_NAMESPACE}_objects)
    SET(KWSYS_TARGET_LINK ${KWSYS_NAMESPACE}_private)
    SET(KWSYS_TARGET_INSTALL ${KWSYS_TARGET_INTERFACE} ${KWSYS_TARGET_LINK})
    SET(KWSYS_LINK_DEPENDENCY INTERFACE)
    ADD_LIBRARY(${KWSYS_TARGET_OBJECT} OBJECT
      ${KWSYS_C_SRCS} ${KWSYS_CXX_SRCS})
    IF(KWSYS_BUILD_SHARED)
      SET_PROPERTY(TARGET ${KWSYS_TARGET_OBJECT} PROPERTY
        POSITION_INDEPENDENT_CODE TRUE)
    ENDIF()
    ADD_LIBRARY(${KWSYS_TARGET_INTERFACE} INTERFACE)
    ADD_LIBRARY(${KWSYS_TARGET_LINK} INTERFACE)
    TARGET_LINK_LIBRARIES(${KWSYS_TARGET_LINK} INTERFACE
      ${KWSYS_TARGET_INTERFACE})
    TARGET_SOURCES(${KWSYS_TARGET_LINK} INTERFACE
      $<TARGET_OBJECTS:${KWSYS_TARGET_OBJECT}>)
  ELSE()
    SET(KWSYS_TARGET_INTERFACE ${KWSYS_NAMESPACE})
    SET(KWSYS_TARGET_OBJECT ${KWSYS_NAMESPACE})
    SET(KWSYS_TARGET_LINK ${KWSYS_NAMESPACE})
    set(KWSYS_TARGET_INSTALL ${KWSYS_TARGET_LINK})
    SET(KWSYS_LINK_DEPENDENCY PUBLIC)
    ADD_LIBRARY(${KWSYS_TARGET_INTERFACE} ${KWSYS_LIBRARY_TYPE}
      ${KWSYS_C_SRCS} ${KWSYS_CXX_SRCS})
  ENDIF()
  if (KWSYS_ALIAS_TARGET)
    add_library(${KWSYS_ALIAS_TARGET} ALIAS ${KWSYS_TARGET_INTERFACE})
  endif ()
  SET_TARGET_PROPERTIES(${KWSYS_TARGET_OBJECT} PROPERTIES
    C_CLANG_TIDY ""
    CXX_CLANG_TIDY ""
    C_INCLUDE_WHAT_YOU_USE ""
    CXX_INCLUDE_WHAT_YOU_USE ""
    LABELS "${KWSYS_LABELS_LIB}")
  IF(KWSYS_USE_DynamicLoader)
    IF(UNIX)
      TARGET_LINK_LIBRARIES(${KWSYS_TARGET_INTERFACE} ${KWSYS_LINK_DEPENDENCY}
        ${CMAKE_DL_LIBS})
    ENDIF()
  ENDIF()

  IF(KWSYS_USE_SystemInformation)
    IF(WIN32)
      TARGET_LINK_LIBRARIES(${KWSYS_TARGET_INTERFACE} ${KWSYS_LINK_DEPENDENCY} ws2_32)
      IF(KWSYS_SYS_HAS_PSAPI)
        TARGET_LINK_LIBRARIES(${KWSYS_TARGET_INTERFACE} ${KWSYS_LINK_DEPENDENCY}
          Psapi)
      ENDIF()
    ELSEIF(UNIX)
      IF (EXECINFO_LIB AND KWSYS_CXX_HAS_BACKTRACE)
        # backtrace on FreeBSD is not in libc
        TARGET_LINK_LIBRARIES(${KWSYS_TARGET_INTERFACE} ${KWSYS_LINK_DEPENDENCY}
          ${EXECINFO_LIB})
      ENDIF()
      IF (KWSYS_CXX_HAS_DLADDR)
        # for symbol lookup using dladdr
        TARGET_LINK_LIBRARIES(${KWSYS_TARGET_INTERFACE} ${KWSYS_LINK_DEPENDENCY}
          ${CMAKE_DL_LIBS})
      ENDIF()
      IF (CMAKE_SYSTEM_NAME STREQUAL "SunOS")
        TARGET_LINK_LIBRARIES(${KWSYS_TARGET_INTERFACE} ${KWSYS_LINK_DEPENDENCY}
          socket)
      ENDIF()
    ENDIF()
  ENDIF()

  # Apply user-defined target properties to the library.
  IF(KWSYS_PROPERTIES_CXX)
    SET_TARGET_PROPERTIES(${KWSYS_TARGET_INTERFACE} PROPERTIES
      ${KWSYS_PROPERTIES_CXX})
  ENDIF()

  # Set up include usage requirement
  IF(COMMAND TARGET_INCLUDE_DIRECTORIES)
    TARGET_INCLUDE_DIRECTORIES(${KWSYS_TARGET_INTERFACE} INTERFACE
      $<BUILD_INTERFACE:${KWSYS_HEADER_ROOT}>)
    IF(KWSYS_INSTALL_INCLUDE_DIR)
      TARGET_INCLUDE_DIRECTORIES(${KWSYS_TARGET_INTERFACE} INTERFACE
        $<INSTALL_INTERFACE:${KWSYS_INSTALL_INCLUDE_DIR}>)
    ENDIF()
  ENDIF()

  # Create an install target for the library.
  IF(KWSYS_INSTALL_LIBRARY_RULE)
    INSTALL(TARGETS ${KWSYS_TARGET_INSTALL} ${KWSYS_INSTALL_LIBRARY_RULE})
  ENDIF()
  IF(KWSYS_INSTALL_NAMELINK_RULE)
    INSTALL(TARGETS ${KWSYS_TARGET_INSTALL} ${KWSYS_INSTALL_NAMELINK_RULE})
  ENDIF()
ENDIF()

# Add a C-only library if requested.
IF(KWSYS_ENABLE_C AND KWSYS_C_SRCS)
 IF(KWSYS_SPLIT_OBJECTS_FROM_INTERFACE)
    SET(KWSYS_TARGET_C_INTERFACE ${KWSYS_NAMESPACE}_c)
    SET(KWSYS_TARGET_C_OBJECT ${KWSYS_NAMESPACE}_c_objects)
    SET(KWSYS_TARGET_C_LINK ${KWSYS_NAMESPACE}_c_private)
    SET(KWSYS_TARGET_C_INSTALL
      ${KWSYS_TARGET_C_INTERFACE} ${KWSYS_TARGET_C_LINK})
    SET(KWSYS_LINK_DEPENDENCY INTERFACE)
    ADD_LIBRARY(${KWSYS_TARGET_C_OBJECT} OBJECT ${KWSYS_C_SRCS})
    IF(KWSYS_BUILD_SHARED)
      SET_PROPERTY(TARGET ${KWSYS_TARGET_C_OBJECT} PROPERTY
        POSITION_INDEPENDENT_CODE TRUE)
    ENDIF()
    ADD_LIBRARY(${KWSYS_TARGET_C_INTERFACE} INTERFACE)
    ADD_LIBRARY(${KWSYS_TARGET_C_LINK} INTERFACE)
    TARGET_LINK_LIBRARIES(${KWSYS_TARGET_C_LINK} INTERFACE
      ${KWSYS_TARGET_C_INTERFACE})
    TARGET_SOURCES(${KWSYS_TARGET_C_LINK} INTERFACE
      $<TARGET_OBJECTS:${KWSYS_TARGET_C_OBJECT}>)
  ELSE()
    SET(KWSYS_TARGET_C_INTERFACE ${KWSYS_NAMESPACE}_c)
    SET(KWSYS_TARGET_C_OBJECT ${KWSYS_NAMESPACE}_c)
    SET(KWSYS_TARGET_C_LINK ${KWSYS_NAMESPACE}_c)
    SET(KWSYS_TARGET_C_INSTALL ${KWSYS_TARGET_C_LINK})
    SET(KWSYS_LINK_DEPENDENCY PUBLIC)
    ADD_LIBRARY(${KWSYS_TARGET_C_INTERFACE} ${KWSYS_LIBRARY_TYPE}
      ${KWSYS_C_SRCS})
  ENDIF()
  SET_TARGET_PROPERTIES(${KWSYS_TARGET_C_OBJECT} PROPERTIES
    LABELS "${KWSYS_LABELS_LIB}")

  # Apply user-defined target properties to the library.
  IF(KWSYS_PROPERTIES_C)
    SET_TARGET_PROPERTIES(${KWSYS_TARGET_C_INTERFACE} PROPERTIES
      ${KWSYS_PROPERTIES_C})
  ENDIF()

  # Set up include usage requirement
  IF(COMMAND TARGET_INCLUDE_DIRECTORIES)
    TARGET_INCLUDE_DIRECTORIES(${KWSYS_TARGET_C_INTERFACE} INTERFACE
      $<BUILD_INTERFACE:${KWSYS_HEADER_ROOT}>)
    IF(KWSYS_INSTALL_INCLUDE_DIR)
      TARGET_INCLUDE_DIRECTORIES(${KWSYS_TARGET_C_INTERFACE} INTERFACE
        $<INSTALL_INTERFACE:${KWSYS_INSTALL_INCLUDE_DIR}>)
    ENDIF()
  ENDIF()

  # Create an install target for the library.
  IF(KWSYS_INSTALL_LIBRARY_RULE)
    INSTALL(TARGETS ${KWSYS_TARGET_C_INSTALL})
  ENDIF()
ENDIF()

# For building kwsys itself, we use a macro defined on the command
# line to configure the namespace in the C and C++ source files.
ADD_DEFINITIONS("-DKWSYS_NAMESPACE=${KWSYS_NAMESPACE}")

# Disable deprecation warnings for standard C functions.
IF(MSVC OR (WIN32 AND CMAKE_C_COMPILER_ID STREQUAL "Intel"))
  ADD_DEFINITIONS(
    -D_CRT_NONSTDC_NO_DEPRECATE
    -D_CRT_SECURE_NO_DEPRECATE
    -D_CRT_SECURE_NO_WARNINGS
    -D_SCL_SECURE_NO_DEPRECATE
    )
ENDIF()

IF(WIN32)
  # Help enforce the use of wide Windows apis.
  ADD_DEFINITIONS(-DUNICODE -D_UNICODE)
ENDIF()

IF(KWSYS_USE_String)
  # Activate code in "String.c".  See the comment in the source.
  SET_SOURCE_FILES_PROPERTIES(String.c PROPERTIES
    COMPILE_FLAGS "-DKWSYS_STRING_C")
ENDIF()

IF(KWSYS_USE_Encoding)
  # Set default 8 bit encoding in "EndcodingC.c".
  SET_PROPERTY(SOURCE EncodingC.c EncodingCXX.cxx APPEND PROPERTY COMPILE_DEFINITIONS
    KWSYS_ENCODING_DEFAULT_CODEPAGE=${KWSYS_ENCODING_DEFAULT_CODEPAGE})
ENDIF()

#-----------------------------------------------------------------------------
# Setup testing if not being built as part of another project.
IF(KWSYS_STANDALONE OR CMake_SOURCE_DIR)
  IF(BUILD_TESTING)
    # Compute the location of executables.
    SET(EXEC_DIR "${CMAKE_CURRENT_BINARY_DIR}")
    IF(EXECUTABLE_OUTPUT_PATH)
      SET(EXEC_DIR "${EXECUTABLE_OUTPUT_PATH}")
    ENDIF()

    # C tests
    SET(KWSYS_C_TESTS
      testEncode.c
      testTerminal.c
      )
    IF(KWSYS_STANDALONE)
      SET(KWSYS_C_TESTS ${KWSYS_C_TESTS} testFail.c)
    ENDIF()
    CREATE_TEST_SOURCELIST(
      KWSYS_C_TEST_SRCS ${KWSYS_NAMESPACE}TestsC.c
      ${KWSYS_C_TESTS}
      )
    ADD_EXECUTABLE(${KWSYS_NAMESPACE}TestsC ${KWSYS_C_TEST_SRCS})
    SET_PROPERTY(TARGET ${KWSYS_NAMESPACE}TestsC PROPERTY LABELS ${KWSYS_LABELS_EXE})
    TARGET_LINK_LIBRARIES(${KWSYS_NAMESPACE}TestsC ${KWSYS_TARGET_C_LINK})
    FOREACH(testfile ${KWSYS_C_TESTS})
      get_filename_component(test "${testfile}" NAME_WE)
      ADD_TEST(kwsys.${test} ${EXEC_DIR}/${KWSYS_NAMESPACE}TestsC ${test} ${KWSYS_TEST_ARGS_${test}})
      SET_PROPERTY(TEST kwsys.${test} PROPERTY LABELS ${KWSYS_LABELS_TEST})
    ENDFOREACH()

    # C++ tests
    IF(NOT WATCOM AND NOT CMake_SOURCE_DIR)
      SET(KWSYS_CXX_TESTS
        testHashSTL.cxx
        )
    ENDIF()
    SET(KWSYS_CXX_TESTS ${KWSYS_CXX_TESTS}
      testConfigure.cxx
      testSystemTools.cxx
      testCommandLineArguments.cxx
      testCommandLineArguments1.cxx
      testDirectory.cxx
      )
    IF(KWSYS_STL_HAS_WSTRING)
      SET(KWSYS_CXX_TESTS ${KWSYS_CXX_TESTS}
        testEncoding.cxx
        )
    ENDIF()
    IF(KWSYS_USE_FStream)
      SET(KWSYS_CXX_TESTS ${KWSYS_CXX_TESTS}
        testFStream.cxx
        )
    ENDIF()
    IF(KWSYS_USE_ConsoleBuf)
      ADD_EXECUTABLE(testConsoleBufChild testConsoleBufChild.cxx)
      SET_PROPERTY(TARGET testConsoleBufChild PROPERTY C_CLANG_TIDY "")
      SET_PROPERTY(TARGET testConsoleBufChild PROPERTY CXX_CLANG_TIDY "")
      SET_PROPERTY(TARGET testConsoleBufChild PROPERTY C_INCLUDE_WHAT_YOU_USE "")
      SET_PROPERTY(TARGET testConsoleBufChild PROPERTY CXX_INCLUDE_WHAT_YOU_USE "")
      SET_PROPERTY(TARGET testConsoleBufChild PROPERTY LABELS ${KWSYS_LABELS_EXE})
      TARGET_LINK_LIBRARIES(testConsoleBufChild ${KWSYS_TARGET_LINK})
      SET(KWSYS_CXX_TESTS ${KWSYS_CXX_TESTS}
        testConsoleBuf.cxx
        )
      IF("x${CMAKE_CXX_COMPILER_ID}" STREQUAL "xMSVC" AND
         CMAKE_CXX_COMPILER_VERSION VERSION_GREATER "19.0.23506")
        set_property(SOURCE testConsoleBuf.cxx testConsoleBufChild.cxx PROPERTY COMPILE_FLAGS /utf-8)
      ENDIF()
      SET_PROPERTY(SOURCE testConsoleBuf.cxx APPEND PROPERTY COMPILE_DEFINITIONS
        KWSYS_ENCODING_DEFAULT_CODEPAGE=${KWSYS_ENCODING_DEFAULT_CODEPAGE})
    ENDIF()
    IF(KWSYS_USE_SystemInformation)
      SET(KWSYS_CXX_TESTS ${KWSYS_CXX_TESTS} testSystemInformation.cxx)
    ENDIF()
    IF(KWSYS_USE_DynamicLoader)
      SET(KWSYS_CXX_TESTS ${KWSYS_CXX_TESTS} testDynamicLoader.cxx)
      # If kwsys contains the DynamicLoader, need extra library
      ADD_LIBRARY(${KWSYS_NAMESPACE}TestDynload MODULE testDynload.c)
      SET_PROPERTY(TARGET ${KWSYS_NAMESPACE}TestDynload PROPERTY LABELS ${KWSYS_LABELS_LIB})
      ADD_DEPENDENCIES(${KWSYS_NAMESPACE}TestDynload ${KWSYS_TARGET_INTERFACE})
    ENDIF()
    CREATE_TEST_SOURCELIST(
      KWSYS_CXX_TEST_SRCS ${KWSYS_NAMESPACE}TestsCxx.cxx
      ${KWSYS_CXX_TESTS}
      )
    ADD_EXECUTABLE(${KWSYS_NAMESPACE}TestsCxx ${KWSYS_CXX_TEST_SRCS})
    SET_PROPERTY(TARGET ${KWSYS_NAMESPACE}TestsCxx PROPERTY C_CLANG_TIDY "")
    SET_PROPERTY(TARGET ${KWSYS_NAMESPACE}TestsCxx PROPERTY CXX_CLANG_TIDY "")
    SET_PROPERTY(TARGET ${KWSYS_NAMESPACE}TestsCxx PROPERTY C_INCLUDE_WHAT_YOU_USE "")
    SET_PROPERTY(TARGET ${KWSYS_NAMESPACE}TestsCxx PROPERTY CXX_INCLUDE_WHAT_YOU_USE "")
    SET_PROPERTY(TARGET ${KWSYS_NAMESPACE}TestsCxx PROPERTY LABELS ${KWSYS_LABELS_EXE})
    TARGET_LINK_LIBRARIES(${KWSYS_NAMESPACE}TestsCxx ${KWSYS_TARGET_LINK})

    SET(TEST_SYSTEMTOOLS_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
    SET(TEST_SYSTEMTOOLS_BINARY_DIR "${CMAKE_CURRENT_BINARY_DIR}")
    CONFIGURE_FILE(
      ${PROJECT_SOURCE_DIR}/testSystemTools.h.in
      ${PROJECT_BINARY_DIR}/testSystemTools.h)
    INCLUDE_DIRECTORIES(${PROJECT_BINARY_DIR})

    IF(CTEST_TEST_KWSYS)
      CONFIGURE_FILE("${CMAKE_CURRENT_SOURCE_DIR}/ExtraTest.cmake.in"
        "${CMAKE_CURRENT_BINARY_DIR}/ExtraTest.cmake")
      SET_DIRECTORY_PROPERTIES(PROPERTIES TEST_INCLUDE_FILE "${CMAKE_CURRENT_BINARY_DIR}/ExtraTest.cmake")
    ENDIF()

    SET(KWSYS_TEST_ARGS_testCommandLineArguments
      --another-bool-variable
      --long3=opt
      --set-bool-arg1
      -SSS ken brad bill andy
      --some-bool-variable=true
      --some-double-variable12.5
      --some-int-variable 14
      "--some-string-variable=test string with space"
      --some-multi-argument 5 1 8 3 7 1 3 9 7 1
      -N 12.5 -SS=andy -N 1.31 -N 22
      -SS=bill -BBtrue -SS=brad
      -BBtrue
      -BBfalse
      -SS=ken
      -A
      -C=test
      --long2 hello
      )
    SET(KWSYS_TEST_ARGS_testCommandLineArguments1
      --ignored
      -n 24
      --second-ignored
      "-m=test value"
      third-ignored
      -p
      some junk at the end
      )
    FOREACH(testfile ${KWSYS_CXX_TESTS})
      get_filename_component(test "${testfile}" NAME_WE)
      ADD_TEST(kwsys.${test} ${EXEC_DIR}/${KWSYS_NAMESPACE}TestsCxx ${test} ${KWSYS_TEST_ARGS_${test}})
      SET_PROPERTY(TEST kwsys.${test} PROPERTY LABELS ${KWSYS_LABELS_TEST})
    ENDFOREACH()

    # Process tests.
    ADD_EXECUTABLE(${KWSYS_NAMESPACE}TestProcess testProcess.c)
    SET_PROPERTY(TARGET ${KWSYS_NAMESPACE}TestProcess PROPERTY LABELS ${KWSYS_LABELS_EXE})
    TARGET_LINK_LIBRARIES(${KWSYS_NAMESPACE}TestProcess ${KWSYS_TARGET_C_LINK})
    IF(NOT CYGWIN)
      SET(KWSYS_TEST_PROCESS_7 7)
    ENDIF()
    FOREACH(n 1 2 3 4 5 6 ${KWSYS_TEST_PROCESS_7} 9 10)
      ADD_TEST(kwsys.testProcess-${n} ${EXEC_DIR}/${KWSYS_NAMESPACE}TestProcess ${n})
      SET_PROPERTY(TEST kwsys.testProcess-${n} PROPERTY LABELS ${KWSYS_LABELS_TEST})
      SET_TESTS_PROPERTIES(kwsys.testProcess-${n} PROPERTIES TIMEOUT 120)
    ENDFOREACH()

    SET(testProcess_COMPILE_FLAGS "")
    # Some Apple compilers produce bad optimizations in this source.
    IF(APPLE AND CMAKE_C_COMPILER_ID MATCHES "^(GNU|LLVM)$")
      SET(testProcess_COMPILE_FLAGS "${testProcess_COMPILE_FLAGS} -O0")
    ELSEIF(CMAKE_C_COMPILER_ID STREQUAL "XL")
      # Tell IBM XL not to warn about our test infinite loop
      IF(CMAKE_SYSTEM MATCHES "Linux.*ppc64le"
         AND CMAKE_C_COMPILER_VERSION VERSION_LESS "16.1.0"
         AND NOT CMAKE_C_COMPILER_VERSION VERSION_LESS "13.1.1")
        # v13.1.[1-6] on Linux ppc64le is clang based and does not accept
        # the -qsuppress option, so just suppress all warnings.
        SET(testProcess_COMPILE_FLAGS "${testProcess_COMPILE_FLAGS} -w")
      ELSE()
        SET(testProcess_COMPILE_FLAGS "${testProcess_COMPILE_FLAGS} -qsuppress=1500-010")
      ENDIF()
    ENDIF()
    IF(CMAKE_C_FLAGS MATCHES "-fsanitize=")
      SET(testProcess_COMPILE_FLAGS "${testProcess_COMPILE_FLAGS} -DCRASH_USING_ABORT")
    ENDIF()
    SET_PROPERTY(SOURCE testProcess.c PROPERTY COMPILE_FLAGS "${testProcess_COMPILE_FLAGS}")

    # Test SharedForward
    CONFIGURE_FILE(${PROJECT_SOURCE_DIR}/testSharedForward.c.in
                   ${PROJECT_BINARY_DIR}/testSharedForward.c @ONLY IMMEDIATE)
    ADD_EXECUTABLE(${KWSYS_NAMESPACE}TestSharedForward
                   ${PROJECT_BINARY_DIR}/testSharedForward.c)
    SET_PROPERTY(TARGET ${KWSYS_NAMESPACE}TestSharedForward PROPERTY LABELS ${KWSYS_LABELS_EXE})
    ADD_DEPENDENCIES(${KWSYS_NAMESPACE}TestSharedForward ${KWSYS_TARGET_C_LINK})
    ADD_TEST(kwsys.testSharedForward ${EXEC_DIR}/${KWSYS_NAMESPACE}TestSharedForward 1)
    SET_PROPERTY(TEST kwsys.testSharedForward PROPERTY LABELS ${KWSYS_LABELS_TEST})

    # Configure some test properties.
    IF(KWSYS_STANDALONE)
      # We expect test to fail
      SET_TESTS_PROPERTIES(kwsys.testFail PROPERTIES WILL_FAIL ON)
      GET_TEST_PROPERTY(kwsys.testFail WILL_FAIL wfv)
      SET_TESTS_PROPERTIES(kwsys.testFail PROPERTIES MEASUREMENT "Some Key=Some Value")
      MESSAGE(STATUS "GET_TEST_PROPERTY returned: ${wfv}")
    ENDIF()

    # Set up ctest custom configuration file.
    CONFIGURE_FILE(${PROJECT_SOURCE_DIR}/CTestCustom.cmake.in
                   ${PROJECT_BINARY_DIR}/CTestCustom.cmake @ONLY)

    # Suppress known consistent failures on buggy systems.
    IF(KWSYS_TEST_BOGUS_FAILURES)
      SET_TESTS_PROPERTIES(${KWSYS_TEST_BOGUS_FAILURES} PROPERTIES WILL_FAIL ON)
    ENDIF()

  ENDIF()
ENDIF()
