
# Copyright 2008-2016 Conrad Sanderson (http://conradsanderson.id.au)
# Copyright 2008-2016 National ICT Australia (NICTA)
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
# ------------------------------------------------------------------------



cmake_minimum_required(VERSION 2.8.12 FATAL_ERROR)

project(armadillo CXX C)
include(CheckIncludeFileCXX)
include(CheckLibraryExists)

## Set ARMA_USE_WRAPPER to false if you're getting linking errors when compiling your programs,
## or if you prefer to directly link with BLAS and/or LAPACK.
## You will then need to link your programs with -lblas -llapack instead of -larmadillo
## If you're using OpenBLAS, link your programs with -lopenblas -llapack instead of -larmadillo

set(ARMA_USE_WRAPPER true)


# the settings below will be automatically configured by the rest of this script

set(ARMA_USE_LAPACK           false)
set(ARMA_USE_BLAS             false)
set(ARMA_USE_ATLAS            false)
set(ARMA_USE_HDF5_ALT         false)
set(ARMA_USE_ARPACK           false)
set(ARMA_USE_EXTERN_CXX11_RNG false)
set(ARMA_USE_SUPERLU          false)  # Caveat: only SuperLU version 5.x can be used!

## NOTE: OpenBLAS appears to have its own LAPACK functions,
## NOTE: but on some systems the installed version of OpenBLAS
## NOTE: has been modified not to include LAPACK functions.
## NOTE: As the presence of LAPACK functions in OpenBLAS can't be guaranteed,
## NOTE: this installer script requires LAPACK to be present on the system
## NOTE: in order to enable the use of LAPACK functions by Armadillo.
## NOTE: This installer will link with OpenBLAS first,
## NOTE: so if a full version of OpenBLAS is actually present on the system,
## NOTE: the linker should make use of LAPACK functions from OpenBLAS
## NOTE: instead of standard LAPACK.


## extract version from sources

set(ARMA_VERSION_FILE_NAME "${PROJECT_SOURCE_DIR}/include/armadillo_bits/arma_version.hpp")

if(NOT EXISTS ${ARMA_VERSION_FILE_NAME})
  message(FATAL_ERROR "Can't read ${ARMA_VERSION_FILE_NAME}")
endif()

file(READ ${ARMA_VERSION_FILE_NAME} ARMA_VERSION_FILE_CONTENTS)
string(REGEX REPLACE ".*#define ARMA_VERSION_MAJOR ([0-9]+).*" "\\1" ARMA_VERSION_MAJOR "${ARMA_VERSION_FILE_CONTENTS}")
string(REGEX REPLACE ".*#define ARMA_VERSION_MINOR ([0-9]+).*" "\\1" ARMA_VERSION_MINOR "${ARMA_VERSION_FILE_CONTENTS}")
string(REGEX REPLACE ".*#define ARMA_VERSION_PATCH ([0-9]+).*" "\\1" ARMA_VERSION_PATCH "${ARMA_VERSION_FILE_CONTENTS}")

message(STATUS "Configuring Armadillo ${ARMA_VERSION_MAJOR}.${ARMA_VERSION_MINOR}.${ARMA_VERSION_PATCH}")


# As Red Hat Enterprise Linux (and related systems such as Fedora)
# does not search /usr/local/lib by default, we need to place the
# library in either /usr/lib or /usr/lib64

if(NOT APPLE)
  if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
    message(STATUS "*** CMAKE_INSTALL_PREFIX was initalised by cmake to the default value of ${CMAKE_INSTALL_PREFIX}")
    message(STATUS "*** CMAKE_INSTALL_PREFIX changed to /usr")
    set(CMAKE_INSTALL_PREFIX "/usr" CACHE PATH "Standard install prefix" FORCE)
  endif()
endif()


# Adhere to GNU filesystem layout conventions
include(GNUInstallDirs)

# deprecated options
set(INSTALL_LIB_DIR     "" CACHE STRING "deprecated; use CMAKE_INSTALL_LIBDIR instead")
set(INSTALL_INCLUDE_DIR "" CACHE STRING "deprecated; use CMAKE_INSTALL_INCLUDEDIR instead")
set(INSTALL_DATA_DIR    "" CACHE STRING "deprecated; use CMAKE_INSTALL_DATADIR instead")
set(INSTALL_BIN_DIR     "" CACHE STRING "deprecated; use CMAKE_INSTALL_BINDIR instead")

if(INSTALL_LIB_DIR)
  message(STATUS "*** INSTALL_LIB_DIR is deprecated; use CMAKE_INSTALL_LIBDIR instead")
  set(CMAKE_INSTALL_LIBDIR "${INSTALL_LIB_DIR}")
  GNUInstallDirs_get_absolute_install_dir(CMAKE_INSTALL_FULL_LIBDIR CMAKE_INSTALL_LIBDIR)
endif()

if(INSTALL_INCLUDE_DIR)
  message(STATUS "*** INSTALL_INCLUDE_DIR is deprecated; use CMAKE_INSTALL_INCLUDEDIR instead")
  set(CMAKE_INSTALL_INCLUDEDIR "${INSTALL_INCLUDE_DIR}")
  GNUInstallDirs_get_absolute_install_dir(CMAKE_INSTALL_FULL_INCLUDEDIR CMAKE_INSTALL_INCLUDEDIR)
endif()

if(INSTALL_DATA_DIR)
  message(STATUS "*** INSTALL_DATA_DIR is deprecated; use CMAKE_INSTALL_DATADIR instead")
  set(CMAKE_INSTALL_DATADIR "${INSTALL_DATA_DIR}")
endif()

if(INSTALL_BIN_DIR)
  message(STATUS "*** INSTALL_BIN_DIR is deprecated; use CMAKE_INSTALL_BINDIR instead")
  set(CMAKE_INSTALL_BINDIR "${INSTALL_BIN_DIR}")
endif()


if(MSVC)
  option(BUILD_SHARED_LIBS "build shared library" OFF)
else()
  option(BUILD_SHARED_LIBS "build shared library" ON)
endif()

option(DETECT_HDF5 "Detect HDF5 and include HDF5 support, if found" OFF)


if(WIN32)
  message(STATUS "")
  message(STATUS "*** WARNING: automatic installation is experimental for this platform.")
  message(STATUS "*** WARNING: if anything breaks, you get to keep all the pieces.")
  message(STATUS "*** WARNING: manual installation is described in README.txt")
  message(STATUS "")
endif()

if(MSVC AND BUILD_SHARED_LIBS)
  message(STATUS "")
  message(STATUS "*** WARNING: building shared library with MSVC is not supported." )
  message(STATUS "*** WARNING: if anything breaks, you get to keep all the pieces.")
  message(STATUS "")
endif()


message(STATUS "CMAKE_SYSTEM_NAME          = ${CMAKE_SYSTEM_NAME}"         )
message(STATUS "CMAKE_CXX_COMPILER_ID      = ${CMAKE_CXX_COMPILER_ID}"     )
message(STATUS "CMAKE_CXX_COMPILER_VERSION = ${CMAKE_CXX_COMPILER_VERSION}")
message(STATUS "CMAKE_COMPILER_IS_GNUCXX   = ${CMAKE_COMPILER_IS_GNUCXX}"  )
message(STATUS "BUILD_SHARED_LIBS          = ${BUILD_SHARED_LIBS}"         )
message(STATUS "DETECT_HDF5                = ${DETECT_HDF5}"               )


##
## Find LAPACK and BLAS libraries, or their optimised versions
##

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/cmake_aux/Modules/")

if(APPLE)

  set(ARMA_OS macos)

  set(ARMA_USE_LAPACK true)
  set(ARMA_USE_BLAS   true)

  set(ARMA_LIBS ${ARMA_LIBS} "-framework Accelerate")  # or "-framework accelerate" ?
  message(STATUS "MacOS X detected. Added '-framework Accelerate' to compiler flags")

  if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -stdlib=libc++")
    message(STATUS "Clang compiler on MacOS X detected. Added '-stdlib=libc++' to compiler flags")
  endif()

else()

  set(ARMA_OS unix)

  include(ARMA_FindMKL)
  include(ARMA_FindACMLMP)
  include(ARMA_FindACML)
  include(ARMA_FindOpenBLAS)
  include(ARMA_FindATLAS)
  include(ARMA_FindBLAS)
  include(ARMA_FindLAPACK)

  message(STATUS "     MKL_FOUND = ${MKL_FOUND}"     )
  message(STATUS "  ACMLMP_FOUND = ${ACMLMP_FOUND}"  )
  message(STATUS "    ACML_FOUND = ${ACML_FOUND}"    )
  message(STATUS "OpenBLAS_FOUND = ${OpenBLAS_FOUND}")
  message(STATUS "   ATLAS_FOUND = ${ATLAS_FOUND}"   )
  message(STATUS "    BLAS_FOUND = ${BLAS_FOUND}"    )
  message(STATUS "  LAPACK_FOUND = ${LAPACK_FOUND}"  )

  if(MKL_FOUND OR ACMLMP_FOUND OR ACML_FOUND)

    set(ARMA_USE_LAPACK true)
    set(ARMA_USE_BLAS   true)

    message(STATUS "")
    message(STATUS "*** If the MKL or ACML libraries are installed in non-standard locations such as")
    message(STATUS "*** /opt/intel/mkl, /opt/intel/composerxe/, /usr/local/intel/mkl")
    message(STATUS "*** make sure the run-time linker can find them.")
    message(STATUS "*** On Linux systems this can be done by editing /etc/ld.so.conf")
    message(STATUS "*** or modifying the LD_LIBRARY_PATH environment variable.")
    message(STATUS "")
    message(STATUS "*** On systems with SELinux enabled (eg. Fedora, RHEL),")
    message(STATUS "*** you may need to change the SELinux type of all MKL/ACML libraries")
    message(STATUS "*** to fix permission problems that may occur during run-time.")
    message(STATUS "*** See README.txt for more information")
    message(STATUS "")

    if(MKL_FOUND)
      set(ARMA_LIBS ${ARMA_LIBS} ${MKL_LIBRARIES})

      if(ACMLMP_FOUND OR ACML_FOUND)
        message(STATUS "*** Intel MKL as well as AMD ACML libraries were found.")
        message(STATUS "*** Using only the MKL library to avoid linking conflicts.")
        message(STATUS "*** If you wish to use ACML instead, please link manually with")
        message(STATUS "*** acml or acml_mp instead of the armadillo wrapper library.")
        message(STATUS "*** Alternatively, remove MKL from your system and rerun")
        message(STATUS "*** Armadillo's configuration using ./configure")
      endif()

    else()

      if(ACMLMP_FOUND)
        set(ARMA_LIBS ${ARMA_LIBS} ${ACMLMP_LIBRARIES})

        message(STATUS "*** Both single-core and multi-core ACML libraries were found.")
        message(STATUS "*** Using only the multi-core library to avoid linking conflicts.")
      else()
        if(ACML_FOUND)
          set(ARMA_LIBS ${ARMA_LIBS} ${ACML_LIBRARIES})
        endif()
      endif()

    endif()

  else()

    if(OpenBLAS_FOUND AND ATLAS_FOUND)
      message(STATUS "")
      message(STATUS "*** WARNING: found both OpenBLAS and ATLAS; ATLAS will not be used")
    endif()

    if(OpenBLAS_FOUND AND BLAS_FOUND)
      message(STATUS "")
      message(STATUS "*** WARNING: found both OpenBLAS and BLAS; BLAS will not be used")
    endif()

    if(OpenBLAS_FOUND)

      set(ARMA_USE_BLAS true)
      set(ARMA_LIBS ${ARMA_LIBS} ${OpenBLAS_LIBRARIES})

      message(STATUS "")
      message(STATUS "*** If the OpenBLAS library is installed in")
      message(STATUS "*** /usr/local/lib or /usr/local/lib64")
      message(STATUS "*** make sure the run-time linker can find it.")
      message(STATUS "*** On Linux systems this can be done by editing /etc/ld.so.conf")
      message(STATUS "*** or modifying the LD_LIBRARY_PATH environment variable.")
      message(STATUS "")

    else()

      if(ATLAS_FOUND)
        set(ARMA_USE_ATLAS true)
        set(ARMA_ATLAS_INCLUDE_DIR ${ATLAS_INCLUDE_DIR})
        set(ARMA_LIBS ${ARMA_LIBS} ${ATLAS_LIBRARIES})

        message(STATUS "ATLAS_INCLUDE_DIR = ${ATLAS_INCLUDE_DIR}")
      endif()

      if(BLAS_FOUND)
        set(ARMA_USE_BLAS true)
        set(ARMA_LIBS ${ARMA_LIBS} ${BLAS_LIBRARIES})
      endif()

    endif()

    if(LAPACK_FOUND)
      set(ARMA_USE_LAPACK true)

      set(ARMA_LIBS ${ARMA_LIBS} ${LAPACK_LIBRARIES} ${BLAS_LIBRARIES})
    endif()

  endif()

endif()


find_package(PkgConfig)


# set(DETECT_HDF5 false)

## uncomment the above line to disable the detection of the HDF5 library;
## you can also disable HDF5 detection directly on the command line:
## cmake -D DETECT_HDF5=false .

if(DETECT_HDF5)
  find_package(HDF5 QUIET COMPONENTS C)

  if(NOT HDF5_FOUND)
    # On Debian systems, the HDF5 package has been split into multiple packages
    # so that it is co-installable.  But this may mean that the include files
    # are hidden somewhere very odd that the FindHDF5.cmake script will not
    # find.  Thus, we'll also quickly check pkgconfig to see if there is
    # information on what to use there.
    if (PKG_CONFIG_FOUND)
      pkg_check_modules(HDF5 hdf5)
      # But using pkgconfig is a little weird because HDF5_LIBRARIES won't be
      # filled with exact library paths, like the other scripts.  So instead
      # what we get is HDF5_LIBRARY_DIRS which is the equivalent of what we'd
      # pass to -L.  So we have to add those...
      if (HDF5_FOUND)
        link_directories("${HDF5_LIBRARY_DIRS}")
      endif()
    endif()
  endif()

  message(STATUS "HDF5_FOUND = ${HDF5_FOUND}")
  if(HDF5_FOUND)
    set(ARMA_USE_HDF5_ALT true)
    set(CMAKE_REQUIRED_INCLUDES ${CMAKE_REQUIRED_INCLUDES} ${HDF5_INCLUDE_DIRS})
    set(ARMA_LIBS ${ARMA_LIBS} ${HDF5_LIBRARIES})
    # Since we called HDF5 with no arguments, the script will find only the C
    # bindings.  So HDF5_INCLUDE_DIRS may now contain one or two elements; if it
    # contains two, the first is what the user passed as HDF5_INCLUDE_DIR and we
    # should use that as ARMA_HDF5_INCLUDE_DIR.  Otherwise, the one entry in
    # HDF5_INCLUDE_DIRS is the correct include directory.  So, in either case we
    # can use the first element in the list.  Issue a status message, too, just
    # for good measure.
    list(GET HDF5_INCLUDE_DIRS 0 ARMA_HDF5_INCLUDE_DIR)
    message(STATUS "ARMA_HDF5_INCLUDE_DIR = ${ARMA_HDF5_INCLUDE_DIR}")
    message(STATUS "")
    message(STATUS "*** If use of HDF5 is causing problems,")
    message(STATUS "*** rerun cmake with HDF5 detection disabled:")
    message(STATUS "*** cmake -D DETECT_HDF5=false .")
    message(STATUS "")
  endif()
endif()

include(ARMA_FindARPACK)
message(STATUS "ARPACK_FOUND = ${ARPACK_FOUND}")

if(ARPACK_FOUND)
  set(ARMA_USE_ARPACK true)
  set(ARMA_LIBS ${ARMA_LIBS} ${ARPACK_LIBRARY})
endif()

include(ARMA_FindSuperLU5)
message(STATUS "SuperLU_FOUND = ${SuperLU_FOUND}")

if(SuperLU_FOUND)
  message(STATUS "SuperLU_INCLUDE_DIR = ${SuperLU_INCLUDE_DIR}")
  set(ARMA_USE_SUPERLU true)
  set(ARMA_LIBS ${ARMA_LIBS} ${SuperLU_LIBRARY})
  set(CMAKE_REQUIRED_INCLUDES ${CMAKE_REQUIRED_INCLUDES} ${SuperLU_INCLUDE_DIR})
  set(ARMA_SUPERLU_INCLUDE_DIR ${SuperLU_INCLUDE_DIR})
endif()

message(STATUS "")
message(STATUS "*** Armadillo wrapper library will use the following libraries:")
message(STATUS "*** ARMA_LIBS = ${ARMA_LIBS}")
message(STATUS "")


if(DEFINED CMAKE_CXX_COMPILER_ID AND DEFINED CMAKE_CXX_COMPILER_VERSION)
  if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" AND NOT ${CMAKE_CXX_COMPILER_VERSION} VERSION_LESS 4.8.3)
    set(ARMA_USE_EXTERN_CXX11_RNG true)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
    message(STATUS "Detected gcc 4.8.3 or later. Added '-std=c++11' to compiler flags")
  endif()
endif()


message(STATUS "Copying ${PROJECT_SOURCE_DIR}/include/ to ${PROJECT_BINARY_DIR}/tmp/include/")
file(COPY ${PROJECT_SOURCE_DIR}/include/ DESTINATION ${PROJECT_BINARY_DIR}/tmp/include/)

message(STATUS "Generating ${PROJECT_BINARY_DIR}/tmp/include/config.hpp")
configure_file(${PROJECT_BINARY_DIR}/tmp/include/armadillo_bits/config.hpp.cmake ${PROJECT_BINARY_DIR}/tmp/include/armadillo_bits/config.hpp)


include_directories(${PROJECT_BINARY_DIR}/tmp/include/ ${CMAKE_REQUIRED_INCLUDES})


## work around a silly limitation in Mac OS X
if(APPLE)
  if(${ARMA_VERSION_MINOR} GREATER 99)
    math(EXPR ARMA_VERSION_MINOR_ALT "${ARMA_VERSION_MINOR} / 10")
  else()
    set(ARMA_VERSION_MINOR_ALT ${ARMA_VERSION_MINOR})
  endif()
else()
  set(ARMA_VERSION_MINOR_ALT ${ARMA_VERSION_MINOR})
endif()


## necessary when linking with Intel MKL on Linux systems
if(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
  set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -Wl,--no-as-needed")
endif()


if(CMAKE_COMPILER_IS_GNUCXX)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O2")
endif()

message(STATUS "CMAKE_CXX_FLAGS           = ${CMAKE_CXX_FLAGS}"          )
message(STATUS "CMAKE_SHARED_LINKER_FLAGS = ${CMAKE_SHARED_LINKER_FLAGS}")
message(STATUS "CMAKE_REQUIRED_INCLUDES   = ${CMAKE_REQUIRED_INCLUDES}"  )


add_library( armadillo STATIC ${PROJECT_SOURCE_DIR}/src/wrapper.cpp)
target_link_libraries( armadillo PUBLIC ${ARMA_LIBS} )

set_target_properties(armadillo PROPERTIES VERSION ${ARMA_VERSION_MAJOR}.${ARMA_VERSION_MINOR_ALT}.${ARMA_VERSION_PATCH} SOVERSION ${ARMA_VERSION_MAJOR})


################################################################################
# INSTALL CONFIGURATION


message(STATUS ""                                                      )
message(STATUS "CMAKE_INSTALL_PREFIX     = ${CMAKE_INSTALL_PREFIX}"    )
message(STATUS "CMAKE_INSTALL_LIBDIR     = ${CMAKE_INSTALL_LIBDIR}"    )
message(STATUS "CMAKE_INSTALL_INCLUDEDIR = ${CMAKE_INSTALL_INCLUDEDIR}")
message(STATUS "CMAKE_INSTALL_DATADIR    = ${CMAKE_INSTALL_DATADIR}"   )
message(STATUS "CMAKE_INSTALL_BINDIR     = ${CMAKE_INSTALL_BINDIR}"    )


# Note that the trailing / character in "include/" is critical

install(DIRECTORY ${PROJECT_BINARY_DIR}/tmp/include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
PATTERN ".svn" EXCLUDE
PATTERN "*.cmake" EXCLUDE
PATTERN "*~" EXCLUDE
PATTERN "*orig" EXCLUDE
)

install(TARGETS armadillo EXPORT ArmadilloLibraryDepends
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

# Export the package for use from the build-tree
# (this registers the build-tree with a global CMake-registry)
export(PACKAGE armadillo)

## LOCAL FILES
# Create ArmadilloConfig.cmake file for the use from the build tree
set(ARMADILLO_INCLUDE_DIRS "${PROJECT_BINARY_DIR}/tmp/include")
set(ARMADILLO_LIB_DIR      "${PROJECT_BINARY_DIR}")
set(ARMADILLO_CMAKE_DIR    "${PROJECT_BINARY_DIR}")


message(STATUS "Generating '${PROJECT_BINARY_DIR}/ArmadilloConfig.cmake'")
# copy/change config and configVersion file (modify only the @xyz@ variables)
configure_file(${PROJECT_SOURCE_DIR}/cmake_aux/InstallFiles/ArmadilloConfig.cmake.in
  "${PROJECT_BINARY_DIR}/ArmadilloConfig.cmake" @ONLY)

message(STATUS "Generating '${PROJECT_BINARY_DIR}/ArmadilloConfigVersion.cmake'")
configure_file(${PROJECT_SOURCE_DIR}/cmake_aux/InstallFiles/ArmadilloConfigVersion.cmake.in
  "${PROJECT_BINARY_DIR}/ArmadilloConfigVersion.cmake" @ONLY)

# Install the export set for use with the install-tree
install(EXPORT ArmadilloLibraryDepends DESTINATION
  "${CMAKE_INSTALL_DATADIR}/Armadillo/CMake"
  COMPONENT dev)


## GLOBAL INSTALL FILES
# Create ArmadilloConfig.cmake file for the use from the install tree
# and install it
set(ARMADILLO_INCLUDE_DIRS "${CMAKE_INSTALL_FULL_INCLUDEDIR}")
set(ARMADILLO_LIB_DIR      "${CMAKE_INSTALL_FULL_LIBDIR}")
set(ARMADILLO_CMAKE_DIR    "${CMAKE_INSTALL_FULL_DATADIR}/Armadillo/CMake")


message(STATUS "Generating '${PROJECT_BINARY_DIR}/InstallFiles/ArmadilloConfig.cmake'")
# copy/change config and configVersion file (modify only the @xyz@ variables)
configure_file(${PROJECT_SOURCE_DIR}/cmake_aux/InstallFiles/ArmadilloConfig.cmake.in
  "${PROJECT_BINARY_DIR}/InstallFiles/ArmadilloConfig.cmake" @ONLY)

message(STATUS "Generating '${PROJECT_BINARY_DIR}/InstallFiles/ArmadilloConfigVersion.cmake'")
configure_file(${PROJECT_SOURCE_DIR}/cmake_aux/InstallFiles/ArmadilloConfigVersion.cmake.in
  "${PROJECT_BINARY_DIR}/InstallFiles/ArmadilloConfigVersion.cmake" @ONLY)

# Install files to be found by cmake users with find_package()
install(FILES
  "${PROJECT_BINARY_DIR}/InstallFiles/ArmadilloConfig.cmake"
  "${PROJECT_BINARY_DIR}/InstallFiles/ArmadilloConfigVersion.cmake"
  DESTINATION "${ARMADILLO_CMAKE_DIR}" COMPONENT dev)


# pkgconfig related

message(STATUS "Copying ${PROJECT_SOURCE_DIR}/misc/ to ${PROJECT_BINARY_DIR}/tmp/misc/")
file(COPY ${PROJECT_SOURCE_DIR}/misc/ DESTINATION ${PROJECT_BINARY_DIR}/tmp/misc/)

message(STATUS "Generating '${PROJECT_BINARY_DIR}/tmp/misc/armadillo.pc'")
configure_file(${PROJECT_BINARY_DIR}/tmp/misc/armadillo.pc.in "${PROJECT_BINARY_DIR}/tmp/misc/armadillo.pc" @ONLY)

install(FILES "${PROJECT_BINARY_DIR}/tmp/misc/armadillo.pc" DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig)
