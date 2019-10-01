# this one must silently fail
include(I_am_not_here OPTIONAL)

# this one must be found and the result must be put into _includedFile
include(CMake RESULT_VARIABLE _includedFile)

set(fileOne "${_includedFile}")
set(fileTwo "${CMAKE_ROOT}/Modules/CMake.cmake")
if(WIN32)
  string(TOLOWER "${fileOne}" fileOne)
  string(TOLOWER "${fileTwo}" fileTwo)
endif()

if(NOT "${fileOne}"   STREQUAL "${fileTwo}")
   message(FATAL_ERROR "Wrong CMake.cmake was included: \"${fileOne}\" expected \"${fileTwo}\"")
endif()

# this one must return NOTFOUND in _includedFile
include(I_do_not_exist OPTIONAL RESULT_VARIABLE _includedFile)

if(_includedFile)
   message(FATAL_ERROR "File \"I_do_not_exist\" was included, although it shouldn't exist,\nIncluded file is \"${_includedFile}\"")
endif()

# and this one must succeed too
include(CMake OPTIONAL RESULT_VARIABLE _includedFile)
set(fileOne "${_includedFile}")
set(fileTwo "${CMAKE_ROOT}/Modules/CMake.cmake")
if(WIN32)
  string(TOLOWER "${fileOne}" fileOne)
  string(TOLOWER "${fileTwo}" fileTwo)
endif()

if(NOT "${fileOne}"   STREQUAL "${fileTwo}")
   message(FATAL_ERROR "Wrong CMake.cmake was included: \"${fileOne}\" expected \"${fileTwo}\"")
endif()

# Check that CMAKE_CURRENT_LIST_DIR is working:
# Needs to be a file in the build tree, which is correct cmake script
# but doesn't do a lot, if possible only set() commands:
include(${CMAKE_CURRENT_LIST_DIR}/../../CTestCustom.cmake)
