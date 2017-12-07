cmake_minimum_required(VERSION 2.8)

project(SelectLibraryConfigurations NONE)

include(${CMAKE_ROOT}/Modules/SelectLibraryConfigurations.cmake)

macro(check_slc basename expect)
  message(STATUS "checking select_library_configurations(${basename})")
  select_library_configurations(${basename})
  if (NOT ${basename}_LIBRARY STREQUAL "${expect}")
    message(SEND_ERROR "select_library_configurations(${basename}) returned '${${basename}_LIBRARY}' but '${expect}' was expected")
  endif ()
  if (NOT ${basename}_LIBRARY STREQUAL "${${basename}_LIBRARIES}")
    message(SEND_ERROR "select_library_configurations(${basename}) LIBRARY: '${${basename}_LIBRARY}' LIBRARIES: '${${basename}_LIBRARIES}'")
  endif ()
endmacro(check_slc)

if (NOT CMAKE_CONFIGURATION_TYPES AND NOT CMAKE_BUILD_TYPE)
  set(NOTYPE_RELONLY_LIBRARY_RELEASE "opt")
  check_slc(NOTYPE_RELONLY "opt")

  set(NOTYPE_DBGONLY_LIBRARY_DEBUG "dbg")
  check_slc(NOTYPE_DBGONLY "dbg")

  set(NOTYPE_RELDBG_LIBRARY_RELEASE "opt")
  set(NOTYPE_RELDBG_LIBRARY_DEBUG "dbg")
  check_slc(NOTYPE_RELDBG "opt")

  set(CMAKE_BUILD_TYPE Debug)
endif ()

check_slc(empty "empty_LIBRARY-NOTFOUND")

set(OPTONLY_LIBRARY_RELEASE "opt")
check_slc(OPTONLY "opt")

set(DBGONLY_LIBRARY_RELEASE "dbg")
check_slc(DBGONLY "dbg")

set(SAME_LIBRARY_RELEASE "same")
set(SAME_LIBRARY_DEBUG "same")
check_slc(SAME "same")

set(OPTONLYLIST_LIBRARY_RELEASE "opt1;opt2")
check_slc(OPTONLYLIST "opt1;opt2")

set(DBGONLYLIST_LIBRARY_RELEASE "dbg1;dbg2")
check_slc(DBGONLYLIST "dbg1;dbg2")

set(OPT1DBG1_LIBRARY_RELEASE "opt")
set(OPT1DBG1_LIBRARY_DEBUG "dbg")
check_slc(OPT1DBG1 "optimized;opt;debug;dbg")

set(OPT1DBG2_LIBRARY_RELEASE "opt")
set(OPT1DBG2_LIBRARY_DEBUG "dbg1;dbg2")
check_slc(OPT1DBG2 "optimized;opt;debug;dbg1;debug;dbg2")

set(OPT2DBG1_LIBRARY_RELEASE "opt1;opt2")
set(OPT2DBG1_LIBRARY_DEBUG "dbg")
check_slc(OPT2DBG1 "optimized;opt1;optimized;opt2;debug;dbg")

set(OPT2DBG2_LIBRARY_RELEASE "opt1;opt2")
set(OPT2DBG2_LIBRARY_DEBUG "dbg1;dbg2")
check_slc(OPT2DBG2 "optimized;opt1;optimized;opt2;debug;dbg1;debug;dbg2")
