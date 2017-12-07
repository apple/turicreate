# Test add_compiler_export_flags without deprecation warning.
set(CMAKE_WARN_DEPRECATED OFF)

project(GenerateExportHeader)

include(CheckCXXCompilerFlag)

set( CMAKE_INCLUDE_CURRENT_DIR ON )

macro(TEST_FAIL value msg)
  if (${value})
    message (SEND_ERROR "Test fail:" "${msg}\n" ${Out} )
  endif ()
endmacro()

macro(TEST_PASS value msg)
  if (NOT ${value})
    message (SEND_ERROR "Test fail:" "${msg}\n" ${Out} )
  endif ()
endmacro()

check_cxx_compiler_flag(-Werror HAS_WERROR_FLAG)

if(HAS_WERROR_FLAG)
  set(ERROR_FLAG "-Werror")
else()
  # MSVC
  # And intel on windows?
  # http://software.intel.com/en-us/articles/how-to-handle-warnings-message-in-compiler/?wapkw=%28compiler+warning+message%29
  check_cxx_compiler_flag("/WX" HAS_WX_FLAG)
  if(HAS_WX_FLAG)
    set(ERROR_FLAG "/WX")
  else()
    # Sun CC
    # http://www.acsu.buffalo.edu/~charngda/sunstudio.html
    check_cxx_compiler_flag("-errwarn=%all" HAS_ERRWARN_ALL)
    if (HAS_ERRWARN_ALL)
      set(ERROR_FLAG "-errwarn=%all")
    else()
    endif()
  endif()
endif()

include(GenerateExportHeader)

set(CMAKE_CXX_STANDARD 98)

# Those versions of the HP compiler that need a flag to get proper C++98
# template support also need a flag to use the newer C++ library.
if (CMAKE_CXX_COMPILER_ID STREQUAL HP AND
    CMAKE_CXX98_STANDARD_COMPILE_OPTION STREQUAL "+hpxstd98")
  string(APPEND CMAKE_CXX_FLAGS " -AA")
endif ()

# Clang/C2 in C++98 mode cannot properly handle some of MSVC headers
if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang" AND
    CMAKE_CXX_SIMULATE_ID STREQUAL "MSVC")
  set(CMAKE_CXX_STANDARD 11)
endif()

add_subdirectory(lib_shared_and_static)

add_compiler_export_flags()

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_CURRENT_BINARY_DIR})

message(STATUS "COMPILER_HAS_DEPRECATED: " ${COMPILER_HAS_DEPRECATED})
message(STATUS "COMPILER_HAS_HIDDEN_VISIBILITY: " ${COMPILER_HAS_HIDDEN_VISIBILITY})
message(STATUS "WIN32: " ${WIN32})
message(STATUS "HAS_WERROR_FLAG: " ${HAS_WERROR_FLAG})

set(link_libraries)
macro(macro_add_test_library name)
  add_subdirectory(${name})
  include_directories(${name}
    ${CMAKE_CURRENT_BINARY_DIR}/${name} # For the export header.
  )
  list(APPEND link_libraries ${name})
endmacro()

macro_add_test_library(libshared)
macro_add_test_library(libstatic)

add_subdirectory(nodeprecated)
if(NOT BORLAND)
  add_subdirectory(c_identifier)
endif()

if (CMAKE_COMPILER_IS_GNUCXX OR (${CMAKE_CXX_COMPILER_ID} MATCHES Clang))
  # No need to clutter the test output with warnings.
  string(APPEND CMAKE_CXX_FLAGS " -Wno-deprecated-declarations")
endif()

if(MSVC AND COMPILER_HAS_DEPRECATED)
  add_definitions(/wd4996)
endif()

add_executable(GenerateExportHeader exportheader_test.cpp)

target_link_libraries(GenerateExportHeader ${link_libraries})
if (WIN32 OR CYGWIN)
  if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang" AND
    CMAKE_CXX_SIMULATE_ID STREQUAL "MSVC")
    set(_platform Win32-Clang)
  elseif(MSVC AND COMPILER_HAS_DEPRECATED)
    set(_platform Win32)
  elseif((MINGW OR CYGWIN) AND COMPILER_HAS_DEPRECATED)
    set(_platform MinGW)
  else()
    set(_platform WinEmpty)
  endif()
elseif(COMPILER_HAS_HIDDEN_VISIBILITY)
  set(_platform UNIX)
elseif(COMPILER_HAS_DEPRECATED)
  set(_platform UNIX_DeprecatedOnly)
else()
  set(_platform Empty)
endif()
message(STATUS "Testing reference: ${_platform}")
target_compile_definitions(GenerateExportHeader
  PRIVATE
    "SRC_DIR=\"${CMAKE_CURRENT_SOURCE_DIR}/reference/${_platform}\""
    "BIN_DIR=\"${CMAKE_CURRENT_BINARY_DIR}\""
)

include(${CMAKE_CURRENT_LIST_DIR}/GEH-failures.cmake)
