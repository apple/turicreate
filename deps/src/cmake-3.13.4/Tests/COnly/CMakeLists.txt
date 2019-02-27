# a simple C only test case
cmake_minimum_required (VERSION 2.6)
project (COnly C)

set(CMAKE_DEBUG_POSTFIX "_test_debug_postfix")
add_library(testc1 STATIC libc1.c)
add_library(testc2 SHARED libc2.c)
add_executable (COnly conly.c foo.c foo.h)
target_link_libraries(COnly testc1 testc2)
if(MSVC_VERSION)
  set_target_properties(COnly PROPERTIES
    LINK_FLAGS " /NODEFAULTLIB:\"libcdg.lib\" /NODEFAULTLIB:\"libcmtg.lib\" /NODEFAULTLIB:\"foomsvcrt.lib\" /NODEFAULTLIB:\"libbar.lib\" /NODEFAULTLIB:\"libfooba.lib\"")
endif()
string(ASCII 35 32 67 77 97 107 101 ASCII_STRING)
message(STATUS "String: ${ASCII_STRING}")
get_source_file_property(LANG conly.c LANGUAGE)
if("${LANG}" STREQUAL "C")
  message("Language is C")
else()
  message(FATAL_ERROR "Bad language for file conly.c")
endif()

add_library(testCModule MODULE testCModule.c)
