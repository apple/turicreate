cmake_minimum_required(VERSION 2.8)
project(FindPathTest NONE)

set(CMAKE_FIND_DEBUG_MODE 1)

macro(test_find_path expected)
  unset(HDR CACHE)
  find_path(HDR ${ARGN}
    NO_CMAKE_ENVIRONMENT_PATH
    NO_SYSTEM_ENVIRONMENT_PATH
    )
  if(HDR)
    # Convert to relative path for comparison to expected location.
    file(RELATIVE_PATH REL_HDR "${CMAKE_CURRENT_SOURCE_DIR}" "${HDR}")

    # Check and report failure.
    if(NOT "${REL_HDR}" STREQUAL "${expected}")
      message(SEND_ERROR "Header ${expected} found as [${REL_HDR}]")
    elseif(CMAKE_FIND_DEBUG_MODE)
      message(STATUS "Header ${expected} found as [${REL_HDR}]")
    endif()
  else()
    message(SEND_ERROR "Header ${expected} NOT FOUND")
  endif()
endmacro()

set(CMAKE_SYSTEM_PREFIX_PATH ${CMAKE_CURRENT_SOURCE_DIR})
set(CMAKE_LIBRARY_ARCHITECTURE arch)

test_find_path(include NAMES test1.h)
test_find_path(include/arch NAMES test1arch.h)
