# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

#-----------------------------------------------------------------------------
# set some special flags for different compilers
#
if(WIN32 AND CMAKE_C_COMPILER_ID STREQUAL "Intel")
  set(_INTEL_WINDOWS 1)
endif()

# Disable deprecation warnings for standard C functions.
# really only needed for newer versions of VS, but should
# not hurt other versions, and this will work into the
# future
if(MSVC OR _INTEL_WINDOWS)
  add_definitions(-D_CRT_SECURE_NO_DEPRECATE -D_CRT_NONSTDC_NO_DEPRECATE)
else()
endif()

if(MSVC)
  set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -stack:10000000")
endif()

#silence duplicate symbol warnings on AIX
if(CMAKE_SYSTEM_NAME MATCHES "AIX")
  if(NOT CMAKE_COMPILER_IS_GNUCXX)
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -bhalt:5 ")
  endif()
endif()

if(CMAKE_SYSTEM_NAME MATCHES "IRIX")
  if(NOT CMAKE_COMPILER_IS_GNUCXX)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wl,-woff84 -no_auto_include")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,-woff15")
  endif()
endif()

if(CMAKE_SYSTEM MATCHES "OSF1-V")
  if(NOT CMAKE_COMPILER_IS_GNUCXX)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -timplicit_local -no_implicit_include ")
  endif()
endif()

# Workaround for short jump tables on PA-RISC
if(CMAKE_SYSTEM_PROCESSOR MATCHES "^parisc")
  if(CMAKE_COMPILER_IS_GNUCC)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -mlong-calls")
  endif()
  if(CMAKE_COMPILER_IS_GNUCXX)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mlong-calls")
  endif()
endif()

if (CMAKE_CXX_COMPILER_ID STREQUAL SunPro AND
    NOT DEFINED CMAKE_CXX${CMAKE_CXX_STANDARD}_STANDARD_COMPILE_OPTION)
  if (NOT CMAKE_CXX_COMPILER_VERSION VERSION_LESS 5.13)
    if (NOT CMAKE_CXX_STANDARD OR CMAKE_CXX_STANDARD EQUAL 98)
      set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++03")
    elseif(CMAKE_VERSION VERSION_LESS 3.8.20170502)
      # CMake knows how to add this flag for compilation as C++11,
      # but has not been taught that SunPro needs it for linking too.
      # Add it in a place that will be used for both.
      set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
    endif()
  else()
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -library=stlport4")
  endif()
endif()

foreach(lang C CXX)
  # Suppress warnings from PGI compiler.
  if (CMAKE_${lang}_COMPILER_ID STREQUAL "PGI")
    set(CMAKE_${lang}_FLAGS "${CMAKE_${lang}_FLAGS} -w")
  endif()
endforeach()

# use the ansi CXX compile flag for building cmake
if (CMAKE_ANSI_CXXFLAGS)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${CMAKE_ANSI_CXXFLAGS}")
endif ()

if (CMAKE_ANSI_CFLAGS)
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${CMAKE_ANSI_CFLAGS}")
endif ()

# Allow per-translation-unit parallel builds when using MSVC
if(CMAKE_GENERATOR MATCHES "Visual Studio" AND
   (CMAKE_C_COMPILER_ID MATCHES "MSVC|Intel" OR
   CMAKE_CXX_COMPILER_ID MATCHES "MSVC|Intel"))

  set(CMake_MSVC_PARALLEL ON CACHE STRING "\
Enables /MP flag for parallel builds using MSVC. Specify an \
integer value to control the number of threads used (Only \
works on some older versions of Visual Studio). Setting to \
ON lets the toolchain decide how many threads to use. Set to \
OFF to disable /MP completely." )

  if(CMake_MSVC_PARALLEL)
    if(CMake_MSVC_PARALLEL GREATER 0)
      string(APPEND CMAKE_C_FLAGS " /MP${CMake_MSVC_PARALLEL}")
      string(APPEND CMAKE_CXX_FLAGS " /MP${CMake_MSVC_PARALLEL}")
    else()
      string(APPEND CMAKE_C_FLAGS " /MP")
      string(APPEND CMAKE_CXX_FLAGS " /MP")
    endif()
  endif()
endif()
