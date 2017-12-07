PROJECT(libarchive C)
#
SET(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/build/cmake")
if(NOT CMAKE_RUNTIME_OUTPUT_DIRECTORY)
  set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${libarchive_BINARY_DIR}/bin)
endif()

# On MacOS, prefer MacPorts libraries to system libraries.
# I haven't come up with a compelling argument for this to be conditional.
list(APPEND CMAKE_PREFIX_PATH /opt/local)
# Enable @rpath in the install name.
# detail in "cmake  --help-policy CMP0042"
SET(CMAKE_MACOSX_RPATH ON)

#
# Version - read from 'version' file.
#
FILE(STRINGS ${CMAKE_CURRENT_SOURCE_DIR}/build/version _version)
STRING(REGEX REPLACE
 "^([0-9])[0-9][0-9][0-9][0-9][0-9][0-9][a-z]*$" "\\1" _major ${_version})
STRING(REGEX REPLACE
 "^[0-9]([0-9][0-9][0-9])[0-9][0-9][0-9][a-z]*$" "\\1" _minor ${_version})
STRING(REGEX REPLACE
 "^[0-9][0-9][0-9][0-9]([0-9][0-9][0-9])[a-z]*$" "\\1" _revision ${_version})
STRING(REGEX REPLACE
 "^[0-9][0-9][0-9][0-9][0-9][0-9][0-9]([a-z]*)$" "\\1" _quality ${_version})
SET(_version_number ${_major}${_minor}${_revision})
STRING(REGEX REPLACE "[0]*([^0]*[0-9])$" "\\1" _trimmed_minor ${_minor})
STRING(REGEX REPLACE "[0]*([^0]*[0-9])$" "\\1" _trimmed_revision ${_revision})
#
SET(VERSION                    "${_major}.${_trimmed_minor}.${_trimmed_revision}${_quality}")
SET(BSDCPIO_VERSION_STRING     "${VERSION}")
SET(BSDTAR_VERSION_STRING      "${VERSION}")
SET(BSDCAT_VERSION_STRING      "${VERSION}")
SET(LIBARCHIVE_VERSION_NUMBER  "${_version_number}")
SET(LIBARCHIVE_VERSION_STRING  "${VERSION}")

# INTERFACE_VERSION increments with every release
# libarchive 2.7 == interface version 9 = 2 + 7
# libarchive 2.8 == interface version 10 = 2 + 8
# libarchive 2.9 == interface version 11 = 2 + 9
# libarchive 3.0 == interface version 12
# libarchive 3.1 == interface version 13
math(EXPR INTERFACE_VERSION  "13 + ${_minor}")

# Set SOVERSION == Interface version
# ?? Should there be more here ??
SET(SOVERSION "${INTERFACE_VERSION}")

# Enalbe CMAKE_PUSH_CHECK_STATE() and CMAKE_POP_CHECK_STATE() macros
# saving and restoring the state of the variables.
INCLUDE(${CMake_SOURCE_DIR}/Modules/CMakePushCheckState.cmake)

# Initialize the state of the variables. This initialization is not
# necessary but this shows you what value the variables initially have.
SET(CMAKE_REQUIRED_DEFINITIONS)
SET(CMAKE_REQUIRED_INCLUDES)
SET(CMAKE_REQUIRED_LIBRARIES)
SET(CMAKE_REQUIRED_FLAGS)

# Disable warnings to avoid changing 3rd party code.
IF(CMAKE_C_COMPILER_ID MATCHES
    "^(GNU|Clang|AppleClang|XL|VisualAge|SunPro|MIPSpro|HP|Intel)$")
  SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -w")
ELSEIF(CMAKE_C_COMPILER_ID STREQUAL "PathScale")
  SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -woffall")
ENDIF()

# Enable CTest/CDash support
include(CTest)

OPTION(ENABLE_NETTLE "Enable use of Nettle" ON)
OPTION(ENABLE_OPENSSL "Enable use of OpenSSL" ON)
OPTION(ENABLE_LZO "Enable the use of the system LZO library if found" OFF)
OPTION(ENABLE_LZMA "Enable the use of the system LZMA library if found" ON)

OPTION(ENABLE_ZLIB "Enable the use of the system ZLIB library if found" ON)
OPTION(ENABLE_BZip2 "Enable the use of the system BZip2 library if found" ON)
OPTION(ENABLE_LIBXML2 "Enable the use of the system libxml2 library if found" ON)
OPTION(ENABLE_EXPAT "Enable the use of the system EXPAT library if found" ON)
OPTION(ENABLE_PCREPOSIX "Enable the use of the system PCREPOSIX library if found" ON)
OPTION(ENABLE_LibGCC "Enable the use of the system LibGCC library if found" ON)
# CNG is used for encrypt/decrypt Zip archives on Windows.
OPTION(ENABLE_CNG "Enable the use of CNG(Crypto Next Generation)" ON)

OPTION(ENABLE_XATTR "Enable extended attribute support" ON)
OPTION(ENABLE_ACL "Enable ACL support" ON)
OPTION(ENABLE_ICONV "Enable iconv support" ON)

IF(WIN32)
  #ELSEIF(WINDOWS_VERSION STREQUAL "WINXP")
  SET(NTDDI_VERSION 0x05010000)
  SET(_WIN32_WINNT 0x0501)
  SET(WINVER 0x0501)
ENDIF(WIN32)

set(HAVE_PTHREAD_H 0) # no threads in CMake

IF("${CMAKE_C_PLATFORM_ID}" MATCHES "^(HP-UX)$")
  ADD_DEFINITIONS(-D_XOPEN_SOURCE=500) # Ask wchar.h for mbstate_t
ENDIF()

#
INCLUDE(CheckCSourceCompiles)
INCLUDE(CheckCSourceRuns)
INCLUDE(CheckFileOffsetBits)
INCLUDE(CheckFuncs)
INCLUDE(CheckHeaderDirent)
INCLUDE(CheckIncludeFile)
INCLUDE(CheckIncludeFiles)
INCLUDE(CheckLibraryExists)
INCLUDE(CheckStructHasMember)
INCLUDE(CheckSymbolExists)
INCLUDE(CheckTypeExists)
INCLUDE(CheckTypeSize)

#
# Generate list.h
#
MACRO (GENERATE_LIST_H _listfile _cmlist __list_sources)
  SET(_argv ${ARGV})
  # Remove _listfile and _cmlist from _argv
  LIST(REMOVE_AT _argv 0 1)
  IF (NOT EXISTS "${_listfile}" OR
     ${_cmlist} IS_NEWER_THAN "${_listfile}")

    MESSAGE(STATUS "Generating ${_listfile}")
    FILE(WRITE ${_listfile} "")
    FOREACH (testfile ${_argv})
      IF (testfile MATCHES "^test_[^/]+[.]c$")
        FILE(STRINGS ${testfile} testvar REGEX "^DEFINE_TEST")
        FOREACH (deftest ${testvar})
          FILE(APPEND ${_listfile} "${deftest}\n")
        ENDFOREACH (deftest)
      ENDIF (testfile MATCHES "^test_[^/]+[.]c$")
    ENDFOREACH (testfile)

  ENDIF (NOT EXISTS "${_listfile}" OR
     ${_cmlist} IS_NEWER_THAN "${_listfile}")
ENDMACRO (GENERATE_LIST_H)
#
# Generate installation rules for man pages.
#
MACRO (INSTALL_MAN __mans)
  FOREACH (_man ${ARGV})
    STRING(REGEX REPLACE "^.+[.]([1-9])" "\\1" _mansect ${_man})
    INSTALL(FILES ${_man} DESTINATION "share/man/man${_mansect}")
  ENDFOREACH (_man)
ENDMACRO (INSTALL_MAN __mans)
#
# Find out what macro is needed to use libraries on Windows.
#
MACRO (TRY_MACRO_FOR_LIBRARY INCLUDES LIBRARIES
       TRY_TYPE SAMPLE_SOURCE MACRO_LIST)
  IF(WIN32 AND NOT CYGWIN)
    CMAKE_PUSH_CHECK_STATE()	# Save the state of the variables
    SET(CMAKE_REQUIRED_INCLUDES ${INCLUDES})
    SET(CMAKE_REQUIRED_LIBRARIES ${LIBRARIES})
    FOREACH(VAR ${MACRO_LIST})
      # Clear ${VAR} from CACHE If the libraries which ${VAR} was
      # checked with are changed.
      SET(VAR_WITH_LIB "${VAR}_WITH_LIB")
      GET_PROPERTY(PREV_VAR_WITH_LIB VARIABLE PROPERTY ${VAR_WITH_LIB})
      IF(NOT "${PREV_VAR_WITH_LIB}" STREQUAL "${LIBRARIES}")
        UNSET(${VAR} CACHE)
      ENDIF(NOT "${PREV_VAR_WITH_LIB}" STREQUAL "${LIBRARIES}")
      # Check if the library can be used with the macro.
      IF("${TRY_TYPE}" MATCHES "COMPILES")
        CHECK_C_SOURCE_COMPILES("${SAMPLE_SOURCE}" ${VAR})
      ELSEIF("${TRY_TYPE}" MATCHES "RUNS")
        CHECK_C_SOURCE_RUNS("${SAMPLE_SOURCE}" ${VAR})
      ELSE("${TRY_TYPE}" MATCHES "COMPILES")
        MESSAGE(FATAL_ERROR "UNKNOWN KEYWORD \"${TRY_TYPE}\" FOR TRY_TYPE")
      ENDIF("${TRY_TYPE}" MATCHES "COMPILES")
      # Save the libraries which ${VAR} is checked with.
      SET(${VAR_WITH_LIB} "${LIBRARIES}" CACHE INTERNAL
          "Macro ${VAR} is checked with")
    ENDFOREACH(VAR)
    CMAKE_POP_CHECK_STATE()	# Restore the state of the variables
  ENDIF(WIN32 AND NOT CYGWIN)
ENDMACRO (TRY_MACRO_FOR_LIBRARY)
#
# Check compress/decompress libraries
#
IF(WIN32 AND NOT CMAKE_CL_64 AND NOT CYGWIN)
  # GnuWin32 is only for Win32, not Win64.
  SET(__GNUWIN32PATH "C:/Program Files/GnuWin32")
ENDIF(WIN32 AND NOT CMAKE_CL_64 AND NOT CYGWIN)
IF(DEFINED __GNUWIN32PATH AND EXISTS "${__GNUWIN32PATH}")
  # You have to add a path availabel DLL file into PATH environment variable.
  # Maybe DLL path is "C:/Program Files/GnuWin32/bin".
  # The zlib and the bzip2 Setup program have installed programs and DLLs into
  # "C:/Program Files/GnuWin32" by default.
  # This is convenience setting for Windows.
  SET(CMAKE_PREFIX_PATH ${__GNUWIN32PATH} $(CMAKE_PREFIX_PATH))
  #
  # If you didn't use Setup program or installed into nonstandard path,
  # cmake cannot find out your zlib or bzip2 libraries and include files,
  # you should execute cmake with  -DCMAKE_PREFIX_PATH option.
  #   e.g.
  #     cmake -DCMAKE_PREFIX_PATH=<your-GnuWin32-path> <path-to-source>
  #
  # If compiling error occurred in zconf.h, You may need patch to zconf.h.
  #--- zconf.h.orig	2005-07-21 00:40:26.000000000
  #+++ zconf.h	2009-01-19 11:39:10.093750000
  #@@ -286,7 +286,7 @@
  #
  # #if 1           /* HAVE_UNISTD_H -- this line is updated by ./configure */
  # #  include <sys/types.h> /* for off_t */
  #-#  include <unistd.h>    /* for SEEK_* and off_t */
  #+#  include <stdio.h>    /* for SEEK_* and off_t */
  # #  ifdef VMS
  # #    include <unixio.h>   /* for off_t */
  # #  endif
ENDIF(DEFINED __GNUWIN32PATH AND EXISTS "${__GNUWIN32PATH}")

SET(ADDITIONAL_LIBS "")
#
# Find ZLIB
#
IF(ENABLE_ZLIB)
  FIND_PACKAGE(ZLIB)
ELSE()
  SET(ZLIB_FOUND FALSE) # Override cached value
ENDIF()
IF(ZLIB_FOUND)
  SET(HAVE_LIBZ 1)
  SET(HAVE_ZLIB_H 1)
  INCLUDE_DIRECTORIES(${ZLIB_INCLUDE_DIR})
  LIST(APPEND ADDITIONAL_LIBS ${ZLIB_LIBRARIES})
  IF(WIN32 AND NOT CYGWIN)
    #
    # Test if ZLIB_WINAPI macro is needed to use.
    #
    TRY_MACRO_FOR_LIBRARY(
      "${ZLIB_INCLUDE_DIR}" "${ZLIB_LIBRARIES}"
      RUNS
      "#include <zlib.h>\nint main() {uLong f = zlibCompileFlags(); return (f&(1U<<10))?0:-1; }"
      ZLIB_WINAPI)
    IF(ZLIB_WINAPI)
      ADD_DEFINITIONS(-DZLIB_WINAPI)
    ELSE(ZLIB_WINAPI)
      # Test if a macro is needed for the library.
      TRY_MACRO_FOR_LIBRARY(
        "${ZLIB_INCLUDE_DIR}" "${ZLIB_LIBRARIES}"
        COMPILES
        "#include <zlib.h>\nint main() {return zlibVersion()?1:0; }"
        "ZLIB_DLL;WITHOUT_ZLIB_DLL")
      IF(ZLIB_DLL)
        ADD_DEFINITIONS(-DZLIB_DLL)
      ENDIF(ZLIB_DLL)
    ENDIF(ZLIB_WINAPI)
  ENDIF(WIN32 AND NOT CYGWIN)
ELSE(ZLIB_FOUND)
  MESSAGE(FATAL_ERROR "CMake requires zlib to be available to libarchive")
ENDIF(ZLIB_FOUND)
#
# Find BZip2
#
IF(ENABLE_BZip2)
  FIND_PACKAGE(BZip2)
ELSE()
  SET(BZIP2_FOUND FALSE) # Override cached value
ENDIF()
IF(BZIP2_FOUND)
  SET(HAVE_LIBBZ2 1)
  SET(HAVE_BZLIB_H 1)
  INCLUDE_DIRECTORIES(${BZIP2_INCLUDE_DIR})
  LIST(APPEND ADDITIONAL_LIBS ${BZIP2_LIBRARIES})
  # Test if a macro is needed for the library.
  TRY_MACRO_FOR_LIBRARY(
    "${BZIP2_INCLUDE_DIR}" "${BZIP2_LIBRARIES}"
    COMPILES
    "#include <bzlib.h>\nint main() {return BZ2_bzlibVersion()?1:0; }"
    "USE_BZIP2_DLL;USE_BZIP2_STATIC")
  IF(USE_BZIP2_DLL)
    ADD_DEFINITIONS(-DUSE_BZIP2_DLL)
  ELSEIF(USE_BZIP2_STATIC)
    ADD_DEFINITIONS(-DUSE_BZIP2_STATIC)
  ENDIF(USE_BZIP2_DLL)
ENDIF(BZIP2_FOUND)
MARK_AS_ADVANCED(CLEAR BZIP2_INCLUDE_DIR)
MARK_AS_ADVANCED(CLEAR BZIP2_LIBRARIES)


#
# Find LZMA
#
IF(ENABLE_LZMA)
  FIND_PACKAGE(LibLZMA)
ELSE()
  SET(LIBZMA_FOUND FALSE) # Override cached value
ENDIF()

IF(LIBLZMA_FOUND)
  SET(HAVE_LIBLZMA 1)
  SET(HAVE_LZMA_H 1)
  INCLUDE_DIRECTORIES(${LIBLZMA_INCLUDE_DIRS})
  LIST(APPEND ADDITIONAL_LIBS ${LIBLZMA_LIBRARIES})
  IF(CMAKE_USE_SYSTEM_LIBLZMA)
    # Test if a macro is needed for the library.
    TRY_MACRO_FOR_LIBRARY(
      "${LIBLZMA_INCLUDE_DIRS}" "${LIBLZMA_LIBRARIES}"
      COMPILES
      "#include <lzma.h>\nint main() {return (int)lzma_version_number(); }"
      "WITHOUT_LZMA_API_STATIC;LZMA_API_STATIC")
    IF(NOT WITHOUT_LZMA_API_STATIC AND LZMA_API_STATIC)
      ADD_DEFINITIONS(-DLZMA_API_STATIC)
    ENDIF(NOT WITHOUT_LZMA_API_STATIC AND LZMA_API_STATIC)
  ELSE()
    ADD_DEFINITIONS(-DLZMA_API_STATIC)
  ENDIF()
ELSE(LIBLZMA_FOUND)
# LZMA not found and will not be used.
ENDIF(LIBLZMA_FOUND)
IF(0) # CMake does not need LZO2 support in libarchive
#
# Find LZO2
#
IF(ENABLE_LZO)
  IF (LZO2_INCLUDE_DIR)
    # Already in cache, be silent
    SET(LZO2_FIND_QUIETLY TRUE)
  ENDIF (LZO2_INCLUDE_DIR)

  FIND_PATH(LZO2_INCLUDE_DIR lzo/lzoconf.h)
  FIND_LIBRARY(LZO2_LIBRARY NAMES lzo2 liblzo2)
  INCLUDE(FindPackageHandleStandardArgs)
  FIND_PACKAGE_HANDLE_STANDARD_ARGS(LZO2 DEFAULT_MSG LZO2_LIBRARY LZO2_INCLUDE_DIR)
ELSE(ENABLE_LZO)
  SET(LIBZMA_FOUND FALSE) # Override cached value
ENDIF(ENABLE_LZO)
IF(LZO2_FOUND)
  SET(HAVE_LIBLZO2 1)
  SET(HAVE_LZO_LZOCONF_H 1)
  SET(HAVE_LZO_LZO1X_H 1)
  INCLUDE_DIRECTORIES(${LZO2_INCLUDE_DIR})
  LIST(APPEND ADDITIONAL_LIBS ${LZO2_LIBRARY})
  #
  # TODO: test for static library.
  #
ENDIF(LZO2_FOUND)
MARK_AS_ADVANCED(CLEAR LZO2_INCLUDE_DIR)
MARK_AS_ADVANCED(CLEAR LZO2_LIBRARY)
ENDIF()
IF(0) # CMake does not need LZ4 support in libarchive
#
# Find LZ4
#
IF (LZ4_INCLUDE_DIR)
  # Already in cache, be silent
  SET(LZ4_FIND_QUIETLY TRUE)
ENDIF (LZ4_INCLUDE_DIR)

FIND_PATH(LZ4_INCLUDE_DIR lz4.h)
FIND_LIBRARY(LZ4_LIBRARY NAMES lz4 liblz4)
INCLUDE(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(LZ4 DEFAULT_MSG LZ4_LIBRARY LZ4_INCLUDE_DIR)
IF(LZ4_FOUND)
  SET(HAVE_LIBLZ4 1)
  SET(HAVE_LZ4_H 1)
  CMAKE_PUSH_CHECK_STATE()	# Save the state of the variables
  SET(CMAKE_REQUIRED_INCLUDES ${LZ4_INCLUDE_DIR})
  CHECK_INCLUDE_FILES("lz4hc.h" HAVE_LZ4HC_H)
  CMAKE_POP_CHECK_STATE()	# Restore the state of the variables
  INCLUDE_DIRECTORIES(${LZ4_INCLUDE_DIR})
  LIST(APPEND ADDITIONAL_LIBS ${LZ4_LIBRARY})
  #
  # TODO: test for static library.
  #
ENDIF(LZ4_FOUND)
MARK_AS_ADVANCED(CLEAR LZ4_INCLUDE_DIR)
MARK_AS_ADVANCED(CLEAR LZ4_LIBRARY)
ENDIF()

#
# Check headers
#
CHECK_HEADER_DIRENT()

SET(INCLUDES "")
MACRO (LA_CHECK_INCLUDE_FILE header var)
      CHECK_INCLUDE_FILES("${INCLUDES};${header}" ${var})
      IF (${var})
      	 SET(INCLUDES ${INCLUDES} ${header})
      ENDIF (${var})
ENDMACRO (LA_CHECK_INCLUDE_FILE)

# Some FreeBSD headers assume sys/types.h was already included.
LA_CHECK_INCLUDE_FILE("sys/types.h" HAVE_SYS_TYPES_H)

# Alphabetize the rest unless there's a compelling reason
IF(ENABLE_ACL)
  LA_CHECK_INCLUDE_FILE("acl/libacl.h" HAVE_ACL_LIBACL_H)
ELSE(ENABLE_ACL)
  SET(HAVE_ACL_LIBACL_H FALSE)
ENDIF(ENABLE_ACL)
LA_CHECK_INCLUDE_FILE("ctype.h" HAVE_CTYPE_H)
LA_CHECK_INCLUDE_FILE("copyfile.h" HAVE_COPYFILE_H)
LA_CHECK_INCLUDE_FILE("direct.h" HAVE_DIRECT_H)
LA_CHECK_INCLUDE_FILE("dlfcn.h" HAVE_DLFCN_H)
LA_CHECK_INCLUDE_FILE("errno.h" HAVE_ERRNO_H)
LA_CHECK_INCLUDE_FILE("ext2fs/ext2_fs.h" HAVE_EXT2FS_EXT2_FS_H)

CHECK_C_SOURCE_COMPILES("#include <sys/ioctl.h>
#include <ext2fs/ext2_fs.h>
int main(void) { return EXT2_IOC_GETFLAGS; }" HAVE_WORKING_EXT2_IOC_GETFLAGS)

LA_CHECK_INCLUDE_FILE("fcntl.h" HAVE_FCNTL_H)
LA_CHECK_INCLUDE_FILE("grp.h" HAVE_GRP_H)
LA_CHECK_INCLUDE_FILE("inttypes.h" HAVE_INTTYPES_H)
LA_CHECK_INCLUDE_FILE("io.h" HAVE_IO_H)
LA_CHECK_INCLUDE_FILE("langinfo.h" HAVE_LANGINFO_H)
LA_CHECK_INCLUDE_FILE("limits.h" HAVE_LIMITS_H)
LA_CHECK_INCLUDE_FILE("linux/types.h" HAVE_LINUX_TYPES_H)
LA_CHECK_INCLUDE_FILE("linux/fiemap.h" HAVE_LINUX_FIEMAP_H)
LA_CHECK_INCLUDE_FILE("linux/fs.h" HAVE_LINUX_FS_H)

CHECK_C_SOURCE_COMPILES("#include <sys/ioctl.h>
#include <linux/fs.h>
int main(void) { return FS_IOC_GETFLAGS; }" HAVE_WORKING_FS_IOC_GETFLAGS)

LA_CHECK_INCLUDE_FILE("linux/magic.h" HAVE_LINUX_MAGIC_H)
LA_CHECK_INCLUDE_FILE("locale.h" HAVE_LOCALE_H)
LA_CHECK_INCLUDE_FILE("memory.h" HAVE_MEMORY_H)
LA_CHECK_INCLUDE_FILE("paths.h" HAVE_PATHS_H)
LA_CHECK_INCLUDE_FILE("poll.h" HAVE_POLL_H)
LA_CHECK_INCLUDE_FILE("process.h" HAVE_PROCESS_H)
LA_CHECK_INCLUDE_FILE("pthread.h" HAVE_PTHREAD_H)
LA_CHECK_INCLUDE_FILE("pwd.h" HAVE_PWD_H)
LA_CHECK_INCLUDE_FILE("readpassphrase.h" HAVE_READPASSPHRASE_H)
LA_CHECK_INCLUDE_FILE("regex.h" HAVE_REGEX_H)
LA_CHECK_INCLUDE_FILE("signal.h" HAVE_SIGNAL_H)
LA_CHECK_INCLUDE_FILE("spawn.h" HAVE_SPAWN_H)
LA_CHECK_INCLUDE_FILE("stdarg.h" HAVE_STDARG_H)
LA_CHECK_INCLUDE_FILE("stdint.h" HAVE_STDINT_H)
LA_CHECK_INCLUDE_FILE("stdlib.h" HAVE_STDLIB_H)
LA_CHECK_INCLUDE_FILE("string.h" HAVE_STRING_H)
LA_CHECK_INCLUDE_FILE("strings.h" HAVE_STRINGS_H)
LA_CHECK_INCLUDE_FILE("sys/acl.h" HAVE_SYS_ACL_H)
LA_CHECK_INCLUDE_FILE("sys/cdefs.h" HAVE_SYS_CDEFS_H)
LA_CHECK_INCLUDE_FILE("sys/ioctl.h" HAVE_SYS_IOCTL_H)
LA_CHECK_INCLUDE_FILE("sys/mkdev.h" HAVE_SYS_MKDEV_H)
LA_CHECK_INCLUDE_FILE("sys/mount.h" HAVE_SYS_MOUNT_H)
LA_CHECK_INCLUDE_FILE("sys/param.h" HAVE_SYS_PARAM_H)
LA_CHECK_INCLUDE_FILE("sys/poll.h" HAVE_SYS_POLL_H)
LA_CHECK_INCLUDE_FILE("sys/select.h" HAVE_SYS_SELECT_H)
LA_CHECK_INCLUDE_FILE("sys/stat.h" HAVE_SYS_STAT_H)
LA_CHECK_INCLUDE_FILE("sys/statfs.h" HAVE_SYS_STATFS_H)
LA_CHECK_INCLUDE_FILE("sys/statvfs.h" HAVE_SYS_STATVFS_H)
LA_CHECK_INCLUDE_FILE("sys/time.h" HAVE_SYS_TIME_H)
LA_CHECK_INCLUDE_FILE("sys/utime.h" HAVE_SYS_UTIME_H)
LA_CHECK_INCLUDE_FILE("sys/utsname.h" HAVE_SYS_UTSNAME_H)
LA_CHECK_INCLUDE_FILE("sys/vfs.h" HAVE_SYS_VFS_H)
LA_CHECK_INCLUDE_FILE("sys/wait.h" HAVE_SYS_WAIT_H)
LA_CHECK_INCLUDE_FILE("time.h" HAVE_TIME_H)
LA_CHECK_INCLUDE_FILE("unistd.h" HAVE_UNISTD_H)
LA_CHECK_INCLUDE_FILE("utime.h" HAVE_UTIME_H)
LA_CHECK_INCLUDE_FILE("wchar.h" HAVE_WCHAR_H)
LA_CHECK_INCLUDE_FILE("wctype.h" HAVE_WCTYPE_H)
LA_CHECK_INCLUDE_FILE("windows.h" HAVE_WINDOWS_H)
IF(ENABLE_CNG)
  LA_CHECK_INCLUDE_FILE("Bcrypt.h" HAVE_BCRYPT_H)
ELSE(ENABLE_CNG)
  UNSET(HAVE_BCRYPT_H CACHE)
ENDIF(ENABLE_CNG)
# Following files need windows.h, so we should test it after windows.h test.
LA_CHECK_INCLUDE_FILE("wincrypt.h" HAVE_WINCRYPT_H)
LA_CHECK_INCLUDE_FILE("winioctl.h" HAVE_WINIOCTL_H)

#
# Check whether use of __EXTENSIONS__ is safe.
# We need some macro such as _GNU_SOURCE to use extension functions.
#
SET(_INCLUDE_FILES)
FOREACH (it ${_HEADER})
   SET(_INCLUDE_FILES "${_INCLUDE_FILES}#include <${it}>\n")
ENDFOREACH (it)

CHECK_C_SOURCE_COMPILES(
  "#define __EXTENSIONS__ 1
   ${_INCLUDE_FILES}
   int main() { return 0;}"
 SAFE_TO_DEFINE_EXTENSIONS)

#
# Find Nettle
#
IF(ENABLE_NETTLE)
  FIND_PACKAGE(Nettle)
  IF(NETTLE_FOUND)
    SET(HAVE_LIBNETTLE 1)
    LIST(APPEND ADDITIONAL_LIBS ${NETTLE_LIBRARIES})
    INCLUDE_DIRECTORIES(${NETTLE_INCLUDE_DIR})

    LIST(APPEND CMAKE_REQUIRED_INCLUDES ${NETTLE_INCLUDE_DIR})
    LA_CHECK_INCLUDE_FILE("nettle/aes.h" HAVE_NETTLE_AES_H)
    LA_CHECK_INCLUDE_FILE("nettle/hmac.h" HAVE_NETTLE_HMAC_H)
    LA_CHECK_INCLUDE_FILE("nettle/md5.h" HAVE_NETTLE_MD5_H)
    LA_CHECK_INCLUDE_FILE("nettle/pbkdf2.h" HAVE_NETTLE_PBKDF2_H)
    LA_CHECK_INCLUDE_FILE("nettle/ripemd160.h" HAVE_NETTLE_RIPEMD160_H)
    LA_CHECK_INCLUDE_FILE("nettle/sha.h" HAVE_NETTLE_SHA_H)

  ENDIF(NETTLE_FOUND)
  MARK_AS_ADVANCED(CLEAR NETTLE_INCLUDE_DIR)
  MARK_AS_ADVANCED(CLEAR NETTLE_LIBRARIES)
ENDIF(ENABLE_NETTLE)

#
# Find OpenSSL
# (Except on Mac, where OpenSSL is deprecated.)
#
IF(ENABLE_OPENSSL AND NOT CMAKE_SYSTEM_NAME MATCHES "Darwin")
  FIND_PACKAGE(OpenSSL)
  IF(OPENSSL_FOUND)
    SET(HAVE_LIBCRYPTO 1)
    INCLUDE_DIRECTORIES(${OPENSSL_INCLUDE_DIR})
  ENDIF(OPENSSL_FOUND)
ELSE()
  SET(OPENSSL_FOUND FALSE) # Override cached value
ENDIF()

# FreeBSD libmd
IF(NOT OPENSSL_FOUND)
  CHECK_LIBRARY_EXISTS(md "MD5Init" "" LIBMD_FOUND)
  IF(LIBMD_FOUND)
    CMAKE_PUSH_CHECK_STATE()	# Save the state of the variables
    SET(CMAKE_REQUIRED_LIBRARIES "md")
    FIND_LIBRARY(LIBMD_LIBRARY NAMES md)
    LIST(APPEND ADDITIONAL_LIBS ${LIBMD_LIBRARY})
    CMAKE_POP_CHECK_STATE()	# Restore the state of the variables
  ENDIF(LIBMD_FOUND)
ENDIF(NOT OPENSSL_FOUND)

#
# How to prove that CRYPTO functions, which have several names on various
# platforms, just see if archive_digest.c can compile and link against
# required libraries.
#
MACRO(CHECK_CRYPTO ALGORITHMS IMPLEMENTATION)
    FOREACH(ALGORITHM ${ALGORITHMS})
      IF(NOT ARCHIVE_CRYPTO_${ALGORITHM})
      STRING(TOLOWER "${ALGORITHM}" lower_algorithm)
      STRING(TOUPPER "${ALGORITHM}" algorithm)
      IF ("${IMPLEMENTATION}" MATCHES "^OPENSSL$" AND NOT OPENSSL_FOUND)
        SET(ARCHIVE_CRYPTO_${ALGORITHM}_${IMPLEMENTATION} FALSE)
      ELSEIF("${IMPLEMENTATION}" MATCHES "^NETTLE$" AND NOT NETTLE_FOUND)
        SET(ARCHIVE_CRYPTO_${ALGORITHM}_${IMPLEMENTATION} FALSE)
      ENDIF("${IMPLEMENTATION}" MATCHES "^OPENSSL$" AND NOT OPENSSL_FOUND)

      IF(NOT DEFINED ARCHIVE_CRYPTO_${ALGORITHM}_${IMPLEMENTATION})
        # Probe the local implementation for whether this
	# crypto implementation is available on this platform.
	SET(TRY_CRYPTO_REQUIRED_INCLUDES
	  "-DINCLUDE_DIRECTORIES:STRING=${CMAKE_BINARY_DIR};${CMAKE_CURRENT_SOURCE_DIR}/libarchive;${CMAKE_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/CMakeTmp")
	SET(TRY_CRYPTO_REQUIRED_LIBS)
	IF ("${IMPLEMENTATION}" MATCHES "^OPENSSL$" AND OPENSSL_FOUND)
	    SET(TRY_CRYPTO_REQUIRED_INCLUDES
	      "${TRY_CRYPTO_REQUIRED_INCLUDES};${OPENSSL_INCLUDE_DIR}")
	    SET(TRY_CRYPTO_REQUIRED_LIBS
	        "-DLINK_LIBRARIES:STRING=${OPENSSL_LIBRARIES}")
	ELSEIF("${IMPLEMENTATION}" MATCHES "^NETTLE$" AND NETTLE_FOUND)
	    SET(TRY_CRYPTO_REQUIRED_INCLUDES
	      "${TRY_CRYPTO_REQUIRED_INCLUDES};${NETTLE_INCLUDE_DIR}")
	    SET(TRY_CRYPTO_REQUIRED_LIBS
	        "-DLINK_LIBRARIES:STRING=${NETTLE_LIBRARY}")
	ELSEIF("${IMPLEMENTATION}" MATCHES "^LIBMD$" AND LIBMD_FOUND)
	    SET(TRY_CRYPTO_REQUIRED_LIBS
	        "-DLINK_LIBRARIES:STRING=${LIBMD_LIBRARY}")
	ENDIF("${IMPLEMENTATION}" MATCHES "^OPENSSL$" AND OPENSSL_FOUND)

    CONFIGURE_FILE(${CMAKE_CURRENT_SOURCE_DIR}/build/cmake/config.h.in
      ${CMAKE_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/CMakeTmp/confdefs.h)
	FILE(READ "${CMAKE_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/CMakeTmp/confdefs.h"
	     CONFDEFS_H)
	FILE(READ "${CMAKE_CURRENT_SOURCE_DIR}/libarchive/archive_digest.c"
	     ARCHIVE_CRYPTO_C)

	SET(SOURCE "${CONFDEFS_H}

#define ARCHIVE_${algorithm}_COMPILE_TEST
#define ARCHIVE_CRYPTO_${algorithm}_${IMPLEMENTATION}
#define PLATFORM_CONFIG_H \"check_crypto_md.h\"

${ARCHIVE_CRYPTO_C}

int
main(int argc, char **argv)
{
  archive_${lower_algorithm}_ctx ctx;
  archive_${lower_algorithm}_init(&ctx);
  archive_${lower_algorithm}_update(&ctx, *argv, argc);
  archive_${lower_algorithm}_final(&ctx, NULL);
  return 0;
}
")

  FILE(WRITE "${CMAKE_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/CMakeTmp/check_crypto_md.h" "")
	FILE(WRITE "${CMAKE_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/CMakeTmp/check_crypto_md.c" "${SOURCE}")
	MESSAGE(STATUS "Checking support for ARCHIVE_CRYPTO_${ALGORITHM}_${IMPLEMENTATION}")

	TRY_COMPILE(ARCHIVE_CRYPTO_${ALGORITHM}_${IMPLEMENTATION}
	  ${CMAKE_BINARY_DIR}
	  ${CMAKE_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/CMakeTmp/check_crypto_md.c
	  CMAKE_FLAGS
	   "${TRY_CRYPTO_REQUIRED_LIBS}"
	   "${TRY_CRYPTO_REQUIRED_INCLUDES}"
	  OUTPUT_VARIABLE OUTPUT)

	# Inform user whether or not we found it; if not, log why we didn't.
        IF (ARCHIVE_CRYPTO_${ALGORITHM}_${IMPLEMENTATION})
          MESSAGE(STATUS "Checking support for ARCHIVE_CRYPTO_${ALGORITHM}_${IMPLEMENTATION} -- found")
		  SET(ARCHIVE_CRYPTO_${ALGORITHM} 1)
        ELSE (ARCHIVE_CRYPTO_${ALGORITHM}_${IMPLEMENTATION})
          MESSAGE(STATUS "Checking support for ARCHIVE_CRYPTO_${ALGORITHM}_${IMPLEMENTATION} -- not found")
          FILE(APPEND ${CMAKE_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/CMakeOutput.log
    	    "Checking support for ARCHIVE_CRYPTO_${ALGORITHM}_${IMPLEMENTATION} failed with the following output:\n"
    	    "${OUTPUT}\n"
    	    "Source file was:\n${SOURCE}\n")
        ENDIF (ARCHIVE_CRYPTO_${ALGORITHM}_${IMPLEMENTATION})
      ENDIF(NOT DEFINED ARCHIVE_CRYPTO_${ALGORITHM}_${IMPLEMENTATION})

      # Add appropriate libs/includes depending on whether the implementation
      # was found on this platform.
      IF (ARCHIVE_CRYPTO_${ALGORITHM}_${IMPLEMENTATION})
        IF ("${IMPLEMENTATION}" MATCHES "^OPENSSL$" AND OPENSSL_FOUND)
          INCLUDE_DIRECTORIES(${OPENSSL_INCLUDE_DIR})
	  LIST(APPEND ADDITIONAL_LIBS ${OPENSSL_LIBRARIES})
	   LIST(REMOVE_DUPLICATES ADDITIONAL_LIBS)
        ENDIF ("${IMPLEMENTATION}" MATCHES "^OPENSSL$" AND OPENSSL_FOUND)
      ENDIF (ARCHIVE_CRYPTO_${ALGORITHM}_${IMPLEMENTATION})
      ENDIF(NOT ARCHIVE_CRYPTO_${ALGORITHM})
    ENDFOREACH(ALGORITHM ${ALGORITHMS})
ENDMACRO(CHECK_CRYPTO ALGORITHMS IMPLEMENTATION)

#
# CRYPTO functions on Windows is defined at archive_windows.c, thus we do not
# need the test what the functions can be mapped to archive_{crypto name}_init,
# archive_{crypto name}_update and archive_{crypto name}_final.
# The functions on Windows use CALG_{crypto name} macro to create a crypt object
# and then we need to know what CALG_{crypto name} macros is available to show
# ARCHIVE_CRYPTO_{crypto name}_WIN macros because Windows 2000 and earlier version
# of Windows XP do not support SHA256, SHA384 and SHA512.
#
MACRO(CHECK_CRYPTO_WIN CRYPTO_LIST)
  IF(WIN32 AND NOT CYGWIN)
    FOREACH(CRYPTO ${CRYPTO_LIST})
      IF(NOT ARCHIVE_CRYPTO_${CRYPTO})
      IF(NOT DEFINED ARCHIVE_CRYPTO_${CRYPTO}_WIN)
	STRING(TOUPPER "${CRYPTO}" crypto)
	SET(ALGID "")
	IF ("${CRYPTO}" MATCHES "^MD5$")
	    SET(ALGID "CALG_MD5")
	ENDIF ("${CRYPTO}" MATCHES "^MD5$")
	IF ("${CRYPTO}" MATCHES "^SHA1$")
	    SET(ALGID "CALG_SHA1")
	ENDIF ("${CRYPTO}" MATCHES "^SHA1$")
	IF ("${CRYPTO}" MATCHES "^SHA256$")
	    SET(ALGID "CALG_SHA_256")
	ENDIF ("${CRYPTO}" MATCHES "^SHA256$")
	IF ("${CRYPTO}" MATCHES "^SHA384$")
	    SET(ALGID "CALG_SHA_384")
	ENDIF ("${CRYPTO}" MATCHES "^SHA384$")
	IF ("${CRYPTO}" MATCHES "^SHA512$")
	    SET(ALGID "CALG_SHA_512")
	ENDIF ("${CRYPTO}" MATCHES "^SHA512$")

    CONFIGURE_FILE(${CMAKE_CURRENT_SOURCE_DIR}/build/cmake/config.h.in
      ${CMAKE_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/CMakeTmp/confdefs.h)
	FILE(READ "${CMAKE_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/CMakeTmp/confdefs.h"
	     CONFDEFS_H)

	SET(SOURCE "${CONFDEFS_H}

#define ${crypto}_COMPILE_TEST
#include <windows.h>
#include <wincrypt.h>

int
main(int argc, char **argv)
{
	return ${ALGID};
}
")
	SET(SOURCE_FILE "${CMAKE_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/CMakeTmp/check_crypto_win.c")

	FILE(WRITE "${SOURCE_FILE}" "${SOURCE}")
	MESSAGE(STATUS "Checking support for ARCHIVE_CRYPTO_${CRYPTO}_WIN")

	TRY_COMPILE(ARCHIVE_CRYPTO_${CRYPTO}_WIN
	  ${CMAKE_BINARY_DIR}
	  ${SOURCE_FILE}
	  CMAKE_FLAGS "-DINCLUDE_DIRECTORIES:STRING=${CMAKE_BINARY_DIR};${CMAKE_CURRENT_SOURCE_DIR}/libarchive"
	  OUTPUT_VARIABLE OUTPUT)

	IF (ARCHIVE_CRYPTO_${CRYPTO}_WIN)
	    MESSAGE(STATUS
	        "Checking support for ARCHIVE_CRYPTO_${CRYPTO}_WIN -- found")
		SET(ARCHIVE_CRYPTO_${CRYPTO} 1)
	ELSE (ARCHIVE_CRYPTO_${CRYPTO}_WIN)
	    MESSAGE(STATUS
	         "Checking support for ARCHIVE_CRYPTO_${CRYPTO}_WIN -- not found")
    	    FILE(APPEND
	        ${CMAKE_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/CMakeOutput.log
                "Checking support for ARCHIVE_CRYPTO_${CRYPTO}_WIN failed with the following output:\n"
        	"${OUTPUT}\n"
        	"Source file was:\n${SOURCE}\n")
	ENDIF (ARCHIVE_CRYPTO_${CRYPTO}_WIN)

      ENDIF(NOT DEFINED ARCHIVE_CRYPTO_${CRYPTO}_WIN)
      ENDIF(NOT ARCHIVE_CRYPTO_${CRYPTO})
    ENDFOREACH(CRYPTO)
  ENDIF(WIN32 AND NOT CYGWIN)
ENDMACRO(CHECK_CRYPTO_WIN CRYPTO_LIST)

#
# Find iconv
# POSIX defines the second arg as const char **
# and requires it to be in libc.  But we can accept
# a non-const argument here and can support iconv()
# being in libiconv.
#
MACRO(CHECK_ICONV LIB TRY_ICONV_CONST)
  IF(NOT HAVE_ICONV)
    CMAKE_PUSH_CHECK_STATE()	# Save the state of the variables
    IF (CMAKE_C_COMPILER_ID STREQUAL "GNU" OR
        CMAKE_C_COMPILER_ID STREQUAL "Clang")
      #
      # During checking iconv proto type, we should use -Werror to avoid the
      # success of iconv detection with a warnig which success is a miss
      # detection. So this needs for all build mode(even it's a release mode).
      #
      SET(CMAKE_REQUIRED_FLAGS "${CMAKE_REQUIRED_FLAGS} -Werror")
    ENDIF ()
    IF (CMAKE_C_COMPILER_ID STREQUAL "XL")
      SET(CMAKE_REQUIRED_FLAGS "${CMAKE_REQUIRED_FLAGS} -qhalt=w -qflag=w:w")
    ENDIF ()
    IF (MSVC)
      # NOTE: /WX option is the same as gcc's -Werror option.
      SET(CMAKE_REQUIRED_FLAGS "${CMAKE_REQUIRED_FLAGS} /WX")
    ENDIF (MSVC)
    #
    CHECK_C_SOURCE_COMPILES(
      "#include <stdlib.h>
       #include <iconv.h>
       int main() {
          ${TRY_ICONV_CONST} char *ccp;
          iconv_t cd = iconv_open(\"\", \"\");
          iconv(cd, &ccp, (size_t *)0, (char **)0, (size_t *)0);
          iconv_close(cd);
          return 0;
       }"
     HAVE_ICONV_${LIB}_${TRY_ICONV_CONST})
    IF(HAVE_ICONV_${LIB}_${TRY_ICONV_CONST})
      SET(HAVE_ICONV true)
      SET(ICONV_CONST ${TRY_ICONV_CONST})
    ENDIF(HAVE_ICONV_${LIB}_${TRY_ICONV_CONST})
    CMAKE_POP_CHECK_STATE()	# Restore the state of the variables
  ENDIF(NOT HAVE_ICONV)
ENDMACRO(CHECK_ICONV TRY_ICONV_CONST)

IF(ENABLE_ICONV)
  CMAKE_PUSH_CHECK_STATE()	# Save the state of the variables
  FIND_PATH(ICONV_INCLUDE_DIR iconv.h)
  MARK_AS_ADVANCED(ICONV_INCLUDE_DIR)
  IF(ICONV_INCLUDE_DIR)
    #SET(INCLUDES ${INCLUDES} "iconv.h")
    SET(HAVE_ICONV_H 1)
    INCLUDE_DIRECTORIES(${ICONV_INCLUDE_DIR})
    SET(CMAKE_REQUIRED_INCLUDES ${ICONV_INCLUDE_DIR})
    CHECK_ICONV("libc" "const")
    CHECK_ICONV("libc" "")

    # If iconv isn't in libc and we have a libiconv, try that.
    FIND_LIBRARY(LIBICONV_PATH NAMES iconv libiconv)
    IF(NOT HAVE_ICONV AND LIBICONV_PATH)
      LIST(APPEND CMAKE_REQUIRED_LIBRARIES ${LIBICONV_PATH})
      # Test if a macro is needed for the library.
      TRY_MACRO_FOR_LIBRARY(
        "${ICONV_INCLUDE_DIR}" "${LIBICONV_PATH}"
        COMPILES
        "#include <iconv.h>\nint main() {return iconv_close((iconv_t)0);}"
        "WITHOUT_LIBICONV_STATIC;LIBICONV_STATIC")
      IF(NOT WITHOUT_LIBICONV_STATIC AND LIBICONV_STATIC)
        ADD_DEFINITIONS(-DLIBICONV_STATIC)
      ENDIF(NOT WITHOUT_LIBICONV_STATIC AND LIBICONV_STATIC)
      #
      # Set up CMAKE_REQUIRED_* for CHECK_ICONV
      #
      SET(CMAKE_REQUIRED_INCLUDES ${ICONV_INCLUDE_DIR})
      SET(CMAKE_REQUIRED_LIBRARIES ${LIBICONV_PATH})
      IF(LIBICONV_STATIC)
        # LIBICONV_STATIC is necessary for the success of CHECK_ICONV
        # on Windows.
        SET(CMAKE_REQUIRED_DEFINITIONS "-DLIBICONV_STATIC")
      ELSE(LIBICONV_STATIC)
        SET(CMAKE_REQUIRED_DEFINITIONS)
      ENDIF(LIBICONV_STATIC)
      CHECK_ICONV("libiconv" "const")
      CHECK_ICONV("libiconv" "")
      IF (HAVE_ICONV)
        LIST(APPEND ADDITIONAL_LIBS ${LIBICONV_PATH})
      ENDIF(HAVE_ICONV)
    ENDIF(NOT HAVE_ICONV AND LIBICONV_PATH)
  ENDIF(ICONV_INCLUDE_DIR)
  #
  # Find locale_charset() for libiconv.
  #
  IF(LIBICONV_PATH)
    SET(CMAKE_REQUIRED_DEFINITIONS)
    SET(CMAKE_REQUIRED_INCLUDES ${ICONV_INCLUDE_DIR})
    SET(CMAKE_REQUIRED_LIBRARIES)
    CHECK_INCLUDE_FILES("localcharset.h" HAVE_LOCALCHARSET_H)
    FIND_LIBRARY(LIBCHARSET_PATH NAMES charset libcharset)
    IF(LIBCHARSET_PATH)
      SET(CMAKE_REQUIRED_LIBRARIES ${LIBCHARSET_PATH})
      IF(WIN32 AND NOT CYGWIN)
        # Test if a macro is needed for the library.
        TRY_MACRO_FOR_LIBRARY(
          "${ICONV_INCLUDE_DIR}" "${LIBCHARSET_PATH}"
          COMPILES
          "#include <localcharset.h>\nint main() {return locale_charset()?1:0;}"
          "WITHOUT_LIBCHARSET_STATIC;LIBCHARSET_STATIC")
        IF(NOT WITHOUT_LIBCHARSET_STATIC AND LIBCHARSET_STATIC)
          ADD_DEFINITIONS(-DLIBCHARSET_STATIC)
        ENDIF(NOT WITHOUT_LIBCHARSET_STATIC AND LIBCHARSET_STATIC)
        IF(WITHOUT_LIBCHARSET_STATIC OR LIBCHARSET_STATIC)
          SET(HAVE_LOCALE_CHARSET ON CACHE INTERNAL
              "Have function locale_charset")
        ENDIF(WITHOUT_LIBCHARSET_STATIC OR LIBCHARSET_STATIC)
      ELSE(WIN32 AND NOT CYGWIN)
        CHECK_FUNCTION_EXISTS_GLIBC(locale_charset HAVE_LOCALE_CHARSET)
      ENDIF(WIN32 AND NOT CYGWIN)
      IF(HAVE_LOCALE_CHARSET)
        LIST(APPEND ADDITIONAL_LIBS ${LIBCHARSET_PATH})
      ENDIF(HAVE_LOCALE_CHARSET)
    ENDIF(LIBCHARSET_PATH)
  ENDIF(LIBICONV_PATH)
  CMAKE_POP_CHECK_STATE()	# Restore the state of the variables
ELSE(ENABLE_ICONV)
  # Make sure ICONV variables are not in CACHE after ENABLE_ICONV disabled
  # (once enabled).
  UNSET(HAVE_LOCALE_CHARSET CACHE)
  UNSET(HAVE_ICONV CACHE)
  UNSET(HAVE_ICONV_libc_ CACHE)
  UNSET(HAVE_ICONV_libc_const CACHE)
  UNSET(HAVE_ICONV_libiconv_ CACHE)
  UNSET(HAVE_ICONV_libiconv_const CACHE)
  UNSET(ICONV_INCLUDE_DIR CACHE)
  UNSET(LIBICONV_PATH CACHE)
  UNSET(LIBICONV_DLL CACHE)
  UNSET(LIBICONV_STATIC CACHE)
  UNSET(LIBCHARSET_DLL CACHE)
  UNSET(LIBCHARSET_STATIC CACHE)
ENDIF(ENABLE_ICONV)

IF(0) # CMake does not need XML support in libarchive
#
# Find Libxml2
#
IF(ENABLE_LIBXML2)
  FIND_PACKAGE(LibXml2)
ELSE()
  SET(LIBXML2_FOUND FALSE)
ENDIF()
IF(LIBXML2_FOUND)
  CMAKE_PUSH_CHECK_STATE()	# Save the state of the variables
  INCLUDE_DIRECTORIES(${LIBXML2_INCLUDE_DIR})
  LIST(APPEND ADDITIONAL_LIBS ${LIBXML2_LIBRARIES})
  SET(HAVE_LIBXML2 1)
  # libxml2's include files use iconv.h
  SET(CMAKE_REQUIRED_INCLUDES ${ICONV_INCLUDE_DIR} ${LIBXML2_INCLUDE_DIR})
  CHECK_INCLUDE_FILES("libxml/xmlreader.h" HAVE_LIBXML_XMLREADER_H)
  CHECK_INCLUDE_FILES("libxml/xmlwriter.h" HAVE_LIBXML_XMLWRITER_H)
  # Test if a macro is needed for the library.
  TRY_MACRO_FOR_LIBRARY(
    "${ICONV_INCLUDE_DIR};${LIBXML2_INCLUDE_DIR}"
    "ws2_32.lib;${ZLIB_LIBRARIES};${LIBICONV_PATH};${LIBXML2_LIBRARIES}"
    COMPILES
    "#include <stddef.h>\n#include <libxml/xmlreader.h>\nint main() {return xmlTextReaderRead((xmlTextReaderPtr)(void *)0);}"
    "WITHOUT_LIBXML_STATIC;LIBXML_STATIC")
  IF(NOT WITHOUT_LIBXML_STATIC AND LIBXML_STATIC)
    ADD_DEFINITIONS(-DLIBXML_STATIC)
  ENDIF(NOT WITHOUT_LIBXML_STATIC AND LIBXML_STATIC)
  CMAKE_POP_CHECK_STATE()	# Restore the state of the variables
ELSE(LIBXML2_FOUND)
  #
  # Find Expat
  #
  IF(ENABLE_EXPAT)
    FIND_PACKAGE(EXPAT)
  ELSE()
    SET(EXPAT_FOUND FALSE)
  ENDIF()
  IF(EXPAT_FOUND)
    CMAKE_PUSH_CHECK_STATE()	# Save the state of the variables
    INCLUDE_DIRECTORIES(${EXPAT_INCLUDE_DIR})
    LIST(APPEND ADDITIONAL_LIBS ${EXPAT_LIBRARIES})
    SET(HAVE_LIBEXPAT 1)
    LA_CHECK_INCLUDE_FILE("expat.h" HAVE_EXPAT_H)
    CMAKE_POP_CHECK_STATE()	# Restore the state of the variables
  ENDIF(EXPAT_FOUND)
ENDIF(LIBXML2_FOUND)
MARK_AS_ADVANCED(CLEAR LIBXML2_INCLUDE_DIR)
MARK_AS_ADVANCED(CLEAR LIBXML2_LIBRARIES)
ENDIF()

#
# Check functions
#
CMAKE_PUSH_CHECK_STATE()	# Save the state of the variables
IF (CMAKE_C_COMPILER_ID STREQUAL "GNU" OR
    CMAKE_C_COMPILER_ID STREQUAL "Clang")
  #
  # During checking functions, we should use -fno-builtin to avoid the
  # failure of function detection which failure is an error "conflicting
  # types for built-in function" caused by using -Werror option.
  #
  SET(CMAKE_REQUIRED_FLAGS "${CMAKE_REQUIRED_FLAGS} -fno-builtin")
ENDIF ()
CHECK_SYMBOL_EXISTS(_CrtSetReportMode "crtdbg.h" HAVE__CrtSetReportMode)
CHECK_FUNCTION_EXISTS_GLIBC(arc4random_buf HAVE_ARC4RANDOM_BUF)
CHECK_FUNCTION_EXISTS_GLIBC(chflags HAVE_CHFLAGS)
CHECK_FUNCTION_EXISTS_GLIBC(chown HAVE_CHOWN)
CHECK_FUNCTION_EXISTS_GLIBC(chroot HAVE_CHROOT)
CHECK_FUNCTION_EXISTS_GLIBC(ctime_r HAVE_CTIME_R)
CHECK_FUNCTION_EXISTS_GLIBC(dirfd HAVE_DIRFD)
CHECK_FUNCTION_EXISTS_GLIBC(fchdir HAVE_FCHDIR)
CHECK_FUNCTION_EXISTS_GLIBC(fchflags HAVE_FCHFLAGS)
CHECK_FUNCTION_EXISTS_GLIBC(fchmod HAVE_FCHMOD)
CHECK_FUNCTION_EXISTS_GLIBC(fchown HAVE_FCHOWN)
CHECK_FUNCTION_EXISTS_GLIBC(fcntl HAVE_FCNTL)
CHECK_FUNCTION_EXISTS_GLIBC(fdopendir HAVE_FDOPENDIR)
CHECK_FUNCTION_EXISTS_GLIBC(fork HAVE_FORK)
CHECK_FUNCTION_EXISTS_GLIBC(fstat HAVE_FSTAT)
CHECK_FUNCTION_EXISTS_GLIBC(fstatat HAVE_FSTATAT)
CHECK_FUNCTION_EXISTS_GLIBC(fstatfs HAVE_FSTATFS)
CHECK_FUNCTION_EXISTS_GLIBC(fstatvfs HAVE_FSTATVFS)
CHECK_FUNCTION_EXISTS_GLIBC(ftruncate HAVE_FTRUNCATE)
CHECK_FUNCTION_EXISTS_GLIBC(futimens HAVE_FUTIMENS)
CHECK_FUNCTION_EXISTS_GLIBC(futimes HAVE_FUTIMES)
CHECK_FUNCTION_EXISTS_GLIBC(futimesat HAVE_FUTIMESAT)
CHECK_FUNCTION_EXISTS_GLIBC(geteuid HAVE_GETEUID)
CHECK_FUNCTION_EXISTS_GLIBC(getgrgid_r HAVE_GETGRGID_R)
CHECK_FUNCTION_EXISTS_GLIBC(getgrnam_r HAVE_GETGRNAM_R)
CHECK_FUNCTION_EXISTS_GLIBC(getpwnam_r HAVE_GETPWNAM_R)
CHECK_FUNCTION_EXISTS_GLIBC(getpwuid_r HAVE_GETPWUID_R)
CHECK_FUNCTION_EXISTS_GLIBC(getpid HAVE_GETPID)
CHECK_FUNCTION_EXISTS_GLIBC(getvfsbyname HAVE_GETVFSBYNAME)
CHECK_FUNCTION_EXISTS_GLIBC(gmtime_r HAVE_GMTIME_R)
CHECK_FUNCTION_EXISTS_GLIBC(lchflags HAVE_LCHFLAGS)
CHECK_FUNCTION_EXISTS_GLIBC(lchmod HAVE_LCHMOD)
CHECK_FUNCTION_EXISTS_GLIBC(lchown HAVE_LCHOWN)
CHECK_FUNCTION_EXISTS_GLIBC(link HAVE_LINK)
CHECK_FUNCTION_EXISTS_GLIBC(localtime_r HAVE_LOCALTIME_R)
CHECK_FUNCTION_EXISTS_GLIBC(lstat HAVE_LSTAT)
CHECK_FUNCTION_EXISTS_GLIBC(lutimes HAVE_LUTIMES)
CHECK_FUNCTION_EXISTS_GLIBC(mbrtowc HAVE_MBRTOWC)
CHECK_FUNCTION_EXISTS_GLIBC(memmove HAVE_MEMMOVE)
CHECK_FUNCTION_EXISTS_GLIBC(mkdir HAVE_MKDIR)
CHECK_FUNCTION_EXISTS_GLIBC(mkfifo HAVE_MKFIFO)
CHECK_FUNCTION_EXISTS_GLIBC(mknod HAVE_MKNOD)
CHECK_FUNCTION_EXISTS_GLIBC(mkstemp HAVE_MKSTEMP)
CHECK_FUNCTION_EXISTS_GLIBC(nl_langinfo HAVE_NL_LANGINFO)
CHECK_FUNCTION_EXISTS_GLIBC(openat HAVE_OPENAT)
CHECK_FUNCTION_EXISTS_GLIBC(pipe HAVE_PIPE)
CHECK_FUNCTION_EXISTS_GLIBC(poll HAVE_POLL)
CHECK_FUNCTION_EXISTS_GLIBC(posix_spawnp HAVE_POSIX_SPAWNP)
CHECK_FUNCTION_EXISTS_GLIBC(readlink HAVE_READLINK)
CHECK_FUNCTION_EXISTS_GLIBC(readpassphrase HAVE_READPASSPHRASE)
CHECK_FUNCTION_EXISTS_GLIBC(select HAVE_SELECT)
CHECK_FUNCTION_EXISTS_GLIBC(setenv HAVE_SETENV)
CHECK_FUNCTION_EXISTS_GLIBC(setlocale HAVE_SETLOCALE)
CHECK_FUNCTION_EXISTS_GLIBC(sigaction HAVE_SIGACTION)
CHECK_FUNCTION_EXISTS_GLIBC(statfs HAVE_STATFS)
CHECK_FUNCTION_EXISTS_GLIBC(statvfs HAVE_STATVFS)
CHECK_FUNCTION_EXISTS_GLIBC(strchr HAVE_STRCHR)
CHECK_FUNCTION_EXISTS_GLIBC(strdup HAVE_STRDUP)
CHECK_FUNCTION_EXISTS_GLIBC(strerror HAVE_STRERROR)
CHECK_FUNCTION_EXISTS_GLIBC(strncpy_s HAVE_STRNCPY_S)
CHECK_FUNCTION_EXISTS_GLIBC(strrchr HAVE_STRRCHR)
CHECK_FUNCTION_EXISTS_GLIBC(symlink HAVE_SYMLINK)
CHECK_FUNCTION_EXISTS_GLIBC(timegm HAVE_TIMEGM)
CHECK_FUNCTION_EXISTS_GLIBC(tzset HAVE_TZSET)
CHECK_FUNCTION_EXISTS_GLIBC(unsetenv HAVE_UNSETENV)
CHECK_FUNCTION_EXISTS_GLIBC(utime HAVE_UTIME)
CHECK_FUNCTION_EXISTS_GLIBC(utimes HAVE_UTIMES)
CHECK_FUNCTION_EXISTS_GLIBC(utimensat HAVE_UTIMENSAT)
CHECK_FUNCTION_EXISTS_GLIBC(vfork HAVE_VFORK)
CHECK_FUNCTION_EXISTS_GLIBC(wcrtomb HAVE_WCRTOMB)
CHECK_FUNCTION_EXISTS_GLIBC(wcscmp HAVE_WCSCMP)
CHECK_FUNCTION_EXISTS_GLIBC(wcscpy HAVE_WCSCPY)
CHECK_FUNCTION_EXISTS_GLIBC(wcslen HAVE_WCSLEN)
CHECK_FUNCTION_EXISTS_GLIBC(wctomb HAVE_WCTOMB)
CHECK_FUNCTION_EXISTS_GLIBC(_ctime64_s HAVE__CTIME64_S)
CHECK_FUNCTION_EXISTS_GLIBC(_fseeki64 HAVE__FSEEKI64)
CHECK_FUNCTION_EXISTS_GLIBC(_get_timezone HAVE__GET_TIMEZONE)
CHECK_FUNCTION_EXISTS_GLIBC(_localtime64_s HAVE__LOCALTIME64_S)
CHECK_FUNCTION_EXISTS_GLIBC(_mkgmtime64 HAVE__MKGMTIME64)

SET(CMAKE_REQUIRED_LIBRARIES "")
CHECK_FUNCTION_EXISTS(cygwin_conv_path HAVE_CYGWIN_CONV_PATH)
CHECK_FUNCTION_EXISTS(fseeko HAVE_FSEEKO)
CHECK_FUNCTION_EXISTS(strerror_r HAVE_STRERROR_R)
CHECK_FUNCTION_EXISTS(strftime HAVE_STRFTIME)
CHECK_FUNCTION_EXISTS(vprintf HAVE_VPRINTF)
CHECK_FUNCTION_EXISTS(wmemcmp HAVE_WMEMCMP)
CHECK_FUNCTION_EXISTS(wmemcpy HAVE_WMEMCPY)
CHECK_FUNCTION_EXISTS(wmemmove HAVE_WMEMMOVE)

CMAKE_POP_CHECK_STATE()	# Restore the state of the variables

CHECK_C_SOURCE_COMPILES(
  "#include <sys/types.h>\n#include <sys/mount.h>\nint main(void) { struct vfsconf v; return sizeof(v);}"
  HAVE_STRUCT_VFSCONF)

CHECK_C_SOURCE_COMPILES(
  "#include <sys/types.h>\n#include <sys/mount.h>\nint main(void) { struct xvfsconf v; return sizeof(v);}"
  HAVE_STRUCT_XVFSCONF)

# Make sure we have the POSIX version of readdir_r, not the
# older 2-argument version.
CHECK_C_SOURCE_COMPILES(
  "#include <dirent.h>\nint main() {DIR *d = opendir(\".\"); struct dirent e,*r; return readdir_r(d,&e,&r);}"
  HAVE_READDIR_R)


# Only detect readlinkat() if we also have AT_FDCWD in unistd.h.
# NOTE: linux requires fcntl.h for AT_FDCWD.
CHECK_C_SOURCE_COMPILES(
  "#include <fcntl.h>\n#include <unistd.h>\nint main() {char buf[10]; return readlinkat(AT_FDCWD, \"\", buf, 0);}"
  HAVE_READLINKAT)


# To verify major(), we need to both include the header
# of interest and verify that the result can be linked.
# CHECK_FUNCTION_EXISTS doesn't accept a header argument,
# CHECK_SYMBOL_EXISTS doesn't test linkage.
CHECK_C_SOURCE_COMPILES(
  "#include <sys/mkdev.h>\nint main() { return major(256); }"
  MAJOR_IN_MKDEV)
CHECK_C_SOURCE_COMPILES(
  "#include <sys/sysmacros.h>\nint main() { return major(256); }"
  MAJOR_IN_SYSMACROS)

CHECK_C_SOURCE_COMPILES(
  "#include <lzma.h>\n#if LZMA_VERSION < 50020000\n#error unsupported\n#endif\nint main(void){lzma_stream_encoder_mt(0, 0); return 0;}"
  HAVE_LZMA_STREAM_ENCODER_MT)

IF(HAVE_STRERROR_R)
  SET(HAVE_DECL_STRERROR_R 1)
ENDIF(HAVE_STRERROR_R)

#
# Check defines
#
SET(headers "limits.h")
IF(HAVE_STDINT_H)
  LIST(APPEND headers "stdint.h")
ENDIF(HAVE_STDINT_H)
IF(HAVE_INTTYPES_H)
  LIST(APPEND headers "inttypes.h")
ENDIF(HAVE_INTTYPES_H)
CHECK_SYMBOL_EXISTS(EFTYPE           "errno.h"    HAVE_EFTYPE)
CHECK_SYMBOL_EXISTS(EILSEQ           "errno.h"    HAVE_EILSEQ)
CHECK_SYMBOL_EXISTS(D_MD_ORDER       "langinfo.h" HAVE_D_MD_ORDER)
CHECK_SYMBOL_EXISTS(INT32_MAX        "${headers}" HAVE_DECL_INT32_MAX)
CHECK_SYMBOL_EXISTS(INT32_MIN        "${headers}" HAVE_DECL_INT32_MIN)
CHECK_SYMBOL_EXISTS(INT64_MAX        "${headers}" HAVE_DECL_INT64_MAX)
CHECK_SYMBOL_EXISTS(INT64_MIN        "${headers}" HAVE_DECL_INT64_MIN)
CHECK_SYMBOL_EXISTS(INTMAX_MAX       "${headers}" HAVE_DECL_INTMAX_MAX)
CHECK_SYMBOL_EXISTS(INTMAX_MIN       "${headers}" HAVE_DECL_INTMAX_MIN)
CHECK_SYMBOL_EXISTS(UINT32_MAX       "${headers}" HAVE_DECL_UINT32_MAX)
CHECK_SYMBOL_EXISTS(UINT64_MAX       "${headers}" HAVE_DECL_UINT64_MAX)
CHECK_SYMBOL_EXISTS(UINTMAX_MAX      "${headers}" HAVE_DECL_UINTMAX_MAX)
CHECK_SYMBOL_EXISTS(SIZE_MAX         "${headers}" HAVE_DECL_SIZE_MAX)
CHECK_SYMBOL_EXISTS(SSIZE_MAX        "limits.h"   HAVE_DECL_SSIZE_MAX)

#
# Check struct members
#
# Check for tm_gmtoff in struct tm
CHECK_STRUCT_HAS_MEMBER("struct tm" tm_gmtoff
    "time.h" HAVE_STRUCT_TM_TM_GMTOFF)
CHECK_STRUCT_HAS_MEMBER("struct tm" __tm_gmtoff
    "time.h" HAVE_STRUCT_TM___TM_GMTOFF)

# Check for f_namemax in struct statfs
CHECK_STRUCT_HAS_MEMBER("struct statfs" f_namemax
    "sys/param.h;sys/mount.h" HAVE_STRUCT_STATFS_F_NAMEMAX)

# Check for birthtime in struct stat
CHECK_STRUCT_HAS_MEMBER("struct stat" st_birthtime
    "sys/types.h;sys/stat.h" HAVE_STRUCT_STAT_ST_BIRTHTIME)

# Check for high-resolution timestamps in struct stat
CHECK_STRUCT_HAS_MEMBER("struct stat" st_birthtimespec.tv_nsec
    "sys/types.h;sys/stat.h" HAVE_STRUCT_STAT_ST_BIRTHTIMESPEC_TV_NSEC)
CHECK_STRUCT_HAS_MEMBER("struct stat" st_mtimespec.tv_nsec
    "sys/types.h;sys/stat.h" HAVE_STRUCT_STAT_ST_MTIMESPEC_TV_NSEC)
CHECK_STRUCT_HAS_MEMBER("struct stat" st_mtim.tv_nsec
    "sys/types.h;sys/stat.h" HAVE_STRUCT_STAT_ST_MTIM_TV_NSEC)
CHECK_STRUCT_HAS_MEMBER("struct stat" st_mtime_n
    "sys/types.h;sys/stat.h" HAVE_STRUCT_STAT_ST_MTIME_N)
CHECK_STRUCT_HAS_MEMBER("struct stat" st_umtime
    "sys/types.h;sys/stat.h" HAVE_STRUCT_STAT_ST_UMTIME)
CHECK_STRUCT_HAS_MEMBER("struct stat" st_mtime_usec
    "sys/types.h;sys/stat.h" HAVE_STRUCT_STAT_ST_MTIME_USEC)
# Check for block size support in struct stat
CHECK_STRUCT_HAS_MEMBER("struct stat" st_blksize
    "sys/types.h;sys/stat.h" HAVE_STRUCT_STAT_ST_BLKSIZE)
# Check for st_flags in struct stat (BSD fflags)
CHECK_STRUCT_HAS_MEMBER("struct stat" st_flags
    "sys/types.h;sys/stat.h" HAVE_STRUCT_STAT_ST_FLAGS)

IF(HAVE_SYS_STATVFS_H)
  CHECK_STRUCT_HAS_MEMBER("struct statvfs" f_iosize
    "sys/types.h;sys/statvfs.h" HAVE_STRUCT_STATVFS_F_IOSIZE)
ENDIF()

#
#
CHECK_STRUCT_HAS_MEMBER("struct tm" tm_sec
    "sys/types.h;sys/time.h;time.h" TIME_WITH_SYS_TIME)

#
# Check for integer types
#
#
CHECK_TYPE_SIZE("short" SIZE_OF_SHORT)
CHECK_TYPE_SIZE("int" SIZE_OF_INT)
CHECK_TYPE_SIZE("long" SIZE_OF_LONG)
CHECK_TYPE_SIZE("long long"     SIZE_OF_LONG_LONG)

CHECK_TYPE_SIZE("unsigned short" SIZE_OF_UNSIGNED_SHORT)
CHECK_TYPE_SIZE("unsigned" SIZE_OF_UNSIGNED)
CHECK_TYPE_SIZE("unsigned long" SIZE_OF_UNSIGNED_LONG)
CHECK_TYPE_SIZE("unsigned long long" SIZE_OF_UNSIGNED_LONG_LONG)

CHECK_TYPE_SIZE("__int64" __INT64)
CHECK_TYPE_SIZE("unsigned __int64" UNSIGNED___INT64)

CHECK_TYPE_SIZE(int16_t INT16_T)
CHECK_TYPE_SIZE(int32_t INT32_T)
CHECK_TYPE_SIZE(int64_t INT64_T)
CHECK_TYPE_SIZE(intmax_t INTMAX_T)
CHECK_TYPE_SIZE(uint8_t UINT8_T)
CHECK_TYPE_SIZE(uint16_t UINT16_T)
CHECK_TYPE_SIZE(uint32_t UINT32_T)
CHECK_TYPE_SIZE(uint64_t UINT64_T)
CHECK_TYPE_SIZE(uintmax_t UINTMAX_T)

CHECK_TYPE_SIZE(dev_t       DEV_T)
IF(NOT HAVE_DEV_T)
  IF(MSVC)
    SET(dev_t "unsigned int")
  ENDIF(MSVC)
ENDIF(NOT HAVE_DEV_T)
#
CHECK_TYPE_SIZE(gid_t       GID_T)
IF(NOT HAVE_GID_T)
  IF(WIN32)
    SET(gid_t "short")
  ELSE(WIN32)
    SET(gid_t "unsigned int")
  ENDIF(WIN32)
ENDIF(NOT HAVE_GID_T)
#
CHECK_TYPE_SIZE(id_t        ID_T)
IF(NOT HAVE_ID_T)
  IF(WIN32)
    SET(id_t "short")
  ELSE(WIN32)
    SET(id_t "unsigned int")
  ENDIF(WIN32)
ENDIF(NOT HAVE_ID_T)
#
CHECK_TYPE_SIZE(mode_t      MODE_T)
IF(NOT HAVE_MODE_T)
  IF(WIN32)
    SET(mode_t "unsigned short")
  ELSE(WIN32)
    SET(mode_t "int")
  ENDIF(WIN32)
ENDIF(NOT HAVE_MODE_T)
#
CHECK_TYPE_SIZE(off_t       OFF_T)
IF(NOT HAVE_OFF_T)
  SET(off_t "__int64")
ENDIF(NOT HAVE_OFF_T)
#
CHECK_TYPE_SIZE(size_t      SIZE_T)
IF(NOT HAVE_SIZE_T)
  IF("${CMAKE_SIZEOF_VOID_P}" EQUAL 8)
    SET(size_t "uint64_t")
  ELSE("${CMAKE_SIZEOF_VOID_P}" EQUAL 8)
    SET(size_t   "uint32_t")
  ENDIF("${CMAKE_SIZEOF_VOID_P}" EQUAL 8)
ENDIF(NOT HAVE_SIZE_T)
#
CHECK_TYPE_SIZE(ssize_t     SSIZE_T)
IF(NOT HAVE_SSIZE_T)
  IF("${CMAKE_SIZEOF_VOID_P}" EQUAL 8)
    SET(ssize_t "int64_t")
  ELSE("${CMAKE_SIZEOF_VOID_P}" EQUAL 8)
    SET(ssize_t "long")
  ENDIF("${CMAKE_SIZEOF_VOID_P}" EQUAL 8)
ENDIF(NOT HAVE_SSIZE_T)
#
CHECK_TYPE_SIZE(uid_t       UID_T)
IF(NOT HAVE_UID_T)
  IF(WIN32)
    SET(uid_t "short")
  ELSE(WIN32)
    SET(uid_t "unsigned int")
  ENDIF(WIN32)
ENDIF(NOT HAVE_UID_T)
#
CHECK_TYPE_SIZE(pid_t       PID_T)
IF(NOT HAVE_PID_T)
  IF(WIN32)
    SET(pid_t "int")
  ELSE(WIN32)
    MESSAGE(FATAL_ERROR "pid_t doesn't exist on this platform?")
  ENDIF(WIN32)
ENDIF(NOT HAVE_PID_T)
#
CHECK_TYPE_SIZE(intptr_t   INTPTR_T)
IF(NOT HAVE_INTPTR_T)
  IF("${CMAKE_SIZEOF_VOID_P}" EQUAL 8)
    SET(intptr_t "int64_t")
  ELSE()
    SET(intptr_t "int32_t")
  ENDIF()
ENDIF(NOT HAVE_INTPTR_T)
#
CHECK_TYPE_SIZE(uintptr_t   UINTPTR_T)
IF(NOT HAVE_UINTPTR_T)
  IF("${CMAKE_SIZEOF_VOID_P}" EQUAL 8)
    SET(uintptr_t "uint64_t")
  ELSE()
    SET(uintptr_t "uint32_t")
  ENDIF()
ENDIF(NOT HAVE_UINTPTR_T)
#
CHECK_TYPE_SIZE(wchar_t     SIZEOF_WCHAR_T)
IF(HAVE_SIZEOF_WCHAR_T)
  SET(HAVE_WCHAR_T 1)
ENDIF(HAVE_SIZEOF_WCHAR_T)
#
# Check if _FILE_OFFSET_BITS macro needed for large files
#
CHECK_FILE_OFFSET_BITS()

#
# Check for Extended Attribute libraries, headers, and functions
#
IF(ENABLE_XATTR)
  LA_CHECK_INCLUDE_FILE(attr/xattr.h     HAVE_ATTR_XATTR_H)
  LA_CHECK_INCLUDE_FILE(sys/xattr.h      HAVE_SYS_XATTR_H)
  LA_CHECK_INCLUDE_FILE(sys/extattr.h      HAVE_SYS_EXTATTR_H)
  CHECK_LIBRARY_EXISTS(attr "setxattr" "" HAVE_LIBATTR)
  IF(HAVE_LIBATTR)
    SET(CMAKE_REQUIRED_LIBRARIES "attr")
  ENDIF(HAVE_LIBATTR)
  CHECK_SYMBOL_EXISTS(EXTATTR_NAMESPACE_USER "sys/types.h;sys/extattr.h" HAVE_DECL_EXTATTR_NAMESPACE_USER)
  CHECK_FUNCTION_EXISTS_GLIBC(extattr_get_file HAVE_EXTATTR_GET_FILE)
  CHECK_FUNCTION_EXISTS_GLIBC(extattr_list_file HAVE_EXTATTR_LIST_FILE)
  CHECK_FUNCTION_EXISTS_GLIBC(extattr_set_fd HAVE_EXTATTR_SET_FD)
  CHECK_FUNCTION_EXISTS_GLIBC(extattr_set_file HAVE_EXTATTR_SET_FILE)
  CHECK_FUNCTION_EXISTS_GLIBC(fgetxattr HAVE_FGETXATTR)
  CHECK_FUNCTION_EXISTS_GLIBC(flistxattr HAVE_FLISTXATTR)
  CHECK_FUNCTION_EXISTS_GLIBC(fsetxattr HAVE_FSETXATTR)
  CHECK_FUNCTION_EXISTS_GLIBC(getxattr HAVE_GETXATTR)
  CHECK_FUNCTION_EXISTS_GLIBC(lgetxattr HAVE_LGETXATTR)
  CHECK_FUNCTION_EXISTS_GLIBC(listxattr HAVE_LISTXATTR)
  CHECK_FUNCTION_EXISTS_GLIBC(llistxattr HAVE_LLISTXATTR)
  CHECK_FUNCTION_EXISTS_GLIBC(lsetxattr HAVE_LSETXATTR)
  CHECK_FUNCTION_EXISTS_GLIBC(fgetea HAVE_FGETEA)
  CHECK_FUNCTION_EXISTS_GLIBC(flistea HAVE_FLISTEA)
  CHECK_FUNCTION_EXISTS_GLIBC(fsetea HAVE_FSETEA)
  CHECK_FUNCTION_EXISTS_GLIBC(getea HAVE_GETEA)
  CHECK_FUNCTION_EXISTS_GLIBC(lgetea HAVE_LGETEA)
  CHECK_FUNCTION_EXISTS_GLIBC(listea HAVE_LISTEA)
  CHECK_FUNCTION_EXISTS_GLIBC(llistea HAVE_LLISTEA)
  CHECK_FUNCTION_EXISTS_GLIBC(lsetea HAVE_LSETEA)
ELSE(ENABLE_XATTR)
  SET(HAVE_ATTR_LIB FALSE)
  SET(HAVE_ATTR_XATTR_H FALSE)
  SET(HAVE_DECL_EXTATTR_NAMESPACE_USER FALSE)
  SET(HAVE_EXTATTR_GET_FILE FALSE)
  SET(HAVE_EXTATTR_LIST_FILE FALSE)
  SET(HAVE_EXTATTR_SET_FD FALSE)
  SET(HAVE_EXTATTR_SET_FILE FALSE)
  SET(HAVE_FGETEA FALSE)
  SET(HAVE_FGETXATTR FALSE)
  SET(HAVE_FLISTEA FALSE)
  SET(HAVE_FLISTXATTR FALSE)
  SET(HAVE_FSETEA FALSE)
  SET(HAVE_FSETXATTR FALSE)
  SET(HAVE_GETEA FALSE)
  SET(HAVE_GETXATTR FALSE)
  SET(HAVE_LGETEA FALSE)
  SET(HAVE_LGETXATTR FALSE)
  SET(HAVE_LISTEA FALSE)
  SET(HAVE_LISTXATTR FALSE)
  SET(HAVE_LLISTEA FALSE)
  SET(HAVE_LLISTXATTR FALSE)
  SET(HAVE_LSETEA FALSE)
  SET(HAVE_LSETXATTR FALSE)
  SET(HAVE_SYS_EXTATTR_H FALSE)
  SET(HAVE_SYS_XATTR_H FALSE)
ENDIF(ENABLE_XATTR)

#
# Check for ACL libraries, headers, and functions
#
# The ACL support in libarchive is written against the POSIX1e draft,
# which was never officially approved and varies quite a bit across
# platforms.  Worse, some systems have completely non-POSIX acl functions,
# which makes the following checks rather more complex than I would like.
#
IF(ENABLE_ACL)
  CHECK_LIBRARY_EXISTS(acl "acl_get_file" "" HAVE_LIBACL)
  IF(HAVE_LIBACL)
    SET(CMAKE_REQUIRED_LIBRARIES "acl")
    FIND_LIBRARY(ACL_LIBRARY NAMES acl)
    LIST(APPEND ADDITIONAL_LIBS ${ACL_LIBRARY})
  ENDIF(HAVE_LIBACL)
  #
  CHECK_FUNCTION_EXISTS_GLIBC(acl_create_entry HAVE_ACL_CREATE_ENTRY)
  CHECK_FUNCTION_EXISTS_GLIBC(acl_init HAVE_ACL_INIT)
  CHECK_FUNCTION_EXISTS_GLIBC(acl_set_fd HAVE_ACL_SET_FD)
  CHECK_FUNCTION_EXISTS_GLIBC(acl_set_fd_np HAVE_ACL_SET_FD_NP)
  CHECK_FUNCTION_EXISTS_GLIBC(acl_set_file HAVE_ACL_SET_FILE)
  CHECK_TYPE_EXISTS(acl_permset_t "${INCLUDES}"    HAVE_ACL_PERMSET_T)

  # The "acl_get_perm()" function was omitted from the POSIX draft.
  # (It's a pretty obvious oversight; otherwise, there's no way to
  # test for specific permissions in a permset.)  Linux uses the obvious
  # name, FreeBSD adds _np to mark it as "non-Posix extension."
  # Test for both as a double-check that we really have POSIX-style ACL support.
  CHECK_FUNCTION_EXISTS(acl_get_fd_np HAVE_ACL_GET_FD_NP)
  CHECK_FUNCTION_EXISTS(acl_get_perm HAVE_ACL_GET_PERM)
  CHECK_FUNCTION_EXISTS(acl_get_perm_np HAVE_ACL_GET_PERM_NP)
  CHECK_FUNCTION_EXISTS(acl_get_link HAVE_ACL_GET_LINK)
  CHECK_FUNCTION_EXISTS(acl_get_link_np HAVE_ACL_GET_LINK_NP)
  CHECK_FUNCTION_EXISTS(acl_is_trivial_np HAVE_ACL_IS_TRIVIAL_NP)
  CHECK_FUNCTION_EXISTS(acl_set_link_np HAVE_ACL_SET_LINK_NP)
  CHECK_SYMBOL_EXISTS(ACL_TYPE_NFS4 "${INCLUDES}" HAVE_ACL_TYPE_NFS4)

  # MacOS has an acl.h that isn't POSIX.  It can be detected by
  # checking for ACL_USER
  CHECK_SYMBOL_EXISTS(ACL_USER "${INCLUDES}" HAVE_ACL_USER)
  CHECK_C_SOURCE_COMPILES("#include <sys/types.h>
#include <sys/acl.h>
int main(void) { return ACL_TYPE_EXTENDED; }" HAVE_ACL_TYPE_EXTENDED)

  # Solaris and derivates ACLs
  CHECK_LIBRARY_EXISTS(sec "acl_get" "" HAVE_LIBSEC)
  IF(HAVE_LIBSEC)
    SET(CMAKE_REQUIRED_LIBRARIES "sec")
    FIND_LIBRARY(SEC_LIBRARY NAMES sec)
    LIST(APPEND ADDITIONAL_LIBS ${SEC_LIBRARY})
  ENDIF(HAVE_LIBSEC)
  #
  CHECK_TYPE_EXISTS(aclent_t "${INCLUDES}" HAVE_ACLENT_T)
  CHECK_TYPE_EXISTS(ace_t "${INCLUDES}" HAVE_ACE_T)
  CHECK_FUNCTION_EXISTS(acl_get HAVE_FACL_GET)
  CHECK_FUNCTION_EXISTS(facl_get HAVE_FACL_GET)
  CHECK_FUNCTION_EXISTS(acl_set HAVE_FACL_SET)
  CHECK_FUNCTION_EXISTS(facl_set HAVE_FACL_SET)
ELSE(ENABLE_ACL)
  # If someone runs cmake, then disables ACL support, we need
  # to forcibly override the cached values for these.
  SET(HAVE_ACL_CREATE_ENTRY FALSE)
  SET(HAVE_ACL_GET_LINK FALSE)
  SET(HAVE_ACL_GET_LINK_NP FALSE)
  SET(HAVE_ACL_GET_PERM FALSE)
  SET(HAVE_ACL_GET_PERM_NP FALSE)
  SET(HAVE_ACL_INIT FALSE)
  SET(HAVE_ACL_LIB FALSE)
  SET(HAVE_ACL_PERMSET_T FALSE)
  SET(HAVE_ACL_SET_FD FALSE)
  SET(HAVE_ACL_SET_FD_NP FALSE)
  SET(HAVE_ACL_SET_FILE FALSE)
  SET(HAVE_ACL_TYPE_NFS4 FALSE)
  SET(HAVE_ACL_USER FALSE)
  SET(HAVE_ACL_TYPE_EXTENDED FALSE)
  SET(HAVE_ACL_GET FALSE)
  SET(HAVE_ACLENT_T FALSE)
  SET(HAVE_ACE_T FALSE)
  SET(HAVE_FACL_GET FALSE)
  SET(HAVE_ACL_SET FALSE)
  SET(HAVE_FACL_SET FALSE)
ENDIF(ENABLE_ACL)

#
# Check MD5/RMD160/SHA support
# NOTE: Crypto checks must be run last before generating config.h
#
CHECK_CRYPTO("MD5;RMD160;SHA1;SHA256;SHA384;SHA512" LIBC)
CHECK_CRYPTO("SHA256;SHA384;SHA512" LIBC2)
CHECK_CRYPTO("SHA256;SHA384;SHA512" LIBC3)
CHECK_CRYPTO("MD5;SHA1;SHA256;SHA384;SHA512" LIBSYSTEM)
CHECK_CRYPTO("MD5;RMD160;SHA1;SHA256;SHA384;SHA512" NETTLE)
CHECK_CRYPTO("MD5;RMD160;SHA1;SHA256;SHA384;SHA512" OPENSSL)

# Libmd has to be probed after OpenSSL.
CHECK_CRYPTO("MD5;RMD160;SHA1;SHA256;SHA512" LIBMD)

CHECK_CRYPTO_WIN("MD5;SHA1;SHA256;SHA384;SHA512")

# Generate "config.h" from "build/cmake/config.h.in"
CONFIGURE_FILE(${CMAKE_CURRENT_SOURCE_DIR}/build/cmake/config.h.in
	${CMAKE_CURRENT_BINARY_DIR}/config.h)
INCLUDE_DIRECTORIES(BEFORE ${CMAKE_CURRENT_BINARY_DIR})
ADD_DEFINITIONS(-DHAVE_CONFIG_H)

#
# Register installation of PDF documents.
#
IF(WIN32 AND NOT CYGWIN)
  #
  # On Windows platform, It's better that we install PDF documents
  # on one's computer.
  # These PDF documents are available in the release package.
  #
  IF(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/doc/pdf)
    INSTALL(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/doc/pdf
            DESTINATION share/man
            FILES_MATCHING PATTERN "*.pdf"
    )
  ENDIF(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/doc/pdf)
ENDIF(WIN32 AND NOT CYGWIN)
#
#
#
INCLUDE_DIRECTORIES(BEFORE ${CMAKE_CURRENT_SOURCE_DIR}/libarchive)
#
IF(MSVC)
  ADD_DEFINITIONS(-D_CRT_SECURE_NO_DEPRECATE)
ENDIF(MSVC)

# We need CoreServices on Mac OS.
IF(APPLE)
  LIST(APPEND ADDITIONAL_LIBS "-framework CoreServices")
ENDIF(APPLE)

add_subdirectory(libarchive)

install(FILES COPYING DESTINATION ${CMAKE_DOC_DIR}/cmlibarchive)
