# Keep formatting here consistent with bootstrap script expectations.
if(BEOS)
  set(CMAKE_BIN_DIR_DEFAULT "bin") # HAIKU
  set(CMAKE_DATA_DIR_DEFAULT "share/cmake-${CMake_VERSION_MAJOR}.${CMake_VERSION_MINOR}") # HAIKU
  set(CMAKE_MAN_DIR_DEFAULT "documentation/man") # HAIKU
  set(CMAKE_DOC_DIR_DEFAULT "documentation/doc/cmake-${CMake_VERSION_MAJOR}.${CMake_VERSION_MINOR}") # HAIKU
  set(CMAKE_XDGDATA_DIR_DEFAULT "share") # HAIKU
elseif(CYGWIN)
  set(CMAKE_BIN_DIR_DEFAULT "bin") # CYGWIN
  set(CMAKE_DATA_DIR_DEFAULT "share/cmake-${CMake_VERSION}") # CYGWIN
  set(CMAKE_DOC_DIR_DEFAULT "share/doc/cmake-${CMake_VERSION}") # CYGWIN
  set(CMAKE_MAN_DIR_DEFAULT "share/man") # CYGWIN
  set(CMAKE_XDGDATA_DIR_DEFAULT "share") # CYGWIN
else()
  set(CMAKE_BIN_DIR_DEFAULT "bin") # OTHER
  set(CMAKE_DATA_DIR_DEFAULT "share/cmake-${CMake_VERSION_MAJOR}.${CMake_VERSION_MINOR}") # OTHER
  set(CMAKE_DOC_DIR_DEFAULT "doc/cmake-${CMake_VERSION_MAJOR}.${CMake_VERSION_MINOR}") # OTHER
  set(CMAKE_MAN_DIR_DEFAULT "man") # OTHER
  set(CMAKE_XDGDATA_DIR_DEFAULT "share") # OTHER
endif()

set(CMAKE_BIN_DIR_DESC "bin")
set(CMAKE_DATA_DIR_DESC "data")
set(CMAKE_DOC_DIR_DESC "docs")
set(CMAKE_MAN_DIR_DESC "man pages")
set(CMAKE_XDGDATA_DIR_DESC "XDG specific files")

set(CMake_INSTALL_INFIX "" CACHE STRING "")
set_property(CACHE CMake_INSTALL_INFIX PROPERTY HELPSTRING
  "Intermediate installation path (empty by default)"
  )
mark_as_advanced(CMake_INSTALL_INFIX)

foreach(v
    CMAKE_BIN_DIR
    CMAKE_DATA_DIR
    CMAKE_DOC_DIR
    CMAKE_MAN_DIR
    CMAKE_XDGDATA_DIR
    )
  # Populate the cache with empty values so we know when the user sets them.
  set(${v} "" CACHE STRING "")
  set_property(CACHE ${v} PROPERTY HELPSTRING
    "Location under install prefix for ${${v}_DESC} (default \"${${v}_DEFAULT}\")"
    )
  set_property(CACHE ${v} PROPERTY ADVANCED 1)

  # Use the default when the user did not set this variable.
  if(NOT ${v})
    set(${v} "${CMake_INSTALL_INFIX}${${v}_DEFAULT}")
  endif()
  # Remove leading slash to treat as relative to install prefix.
  string(REGEX REPLACE "^/" "" ${v} "${${v}}")
endforeach()
