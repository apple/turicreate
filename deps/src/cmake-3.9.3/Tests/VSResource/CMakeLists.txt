cmake_minimum_required(VERSION 2.8.4)
project(VSResource)

string(REPLACE "/INCREMENTAL:YES" ""
  CMAKE_EXE_LINKER_FLAGS_DEBUG
  "${CMAKE_EXE_LINKER_FLAGS_DEBUG}")

message(STATUS "CMAKE_RC_COMPILER='${CMAKE_RC_COMPILER}'")

# Because of the following avoidance techniques required for windres and VS6,
# we recommend using a configured header file, and defining preprocessor
# symbols via #define code and including that header in the rc file. Using
# add_definitions is fine for simple definitions (with no spaces and no
# quoting), but requires avoidance or work-arounds beyond that...

if(CMAKE_RC_COMPILER MATCHES windres)
  # windres rc compiler does not properly define quoted /D values as strings
  message(STATUS "CMAKE_RC_COMPILER MATCHES windres")
  add_definitions(/DCMAKE_RCDEFINE=test.txt)
  add_definitions(/DCMAKE_RCDEFINE_NO_QUOTED_STRINGS)
  if(CMAKE_CURRENT_BINARY_DIR MATCHES " ")
    # windres cannot handle spaces in include dir
    set(CMAKE_RC_NO_INCLUDE 1)
  endif()
elseif(MSVC60)
  # VS6 rc compiler does not deal well with spaces in a "/D" value, but it can
  # handle the quoting
  message(STATUS "MSVC60")
  add_definitions(/DCMAKE_RCDEFINE="test.txt")
else()
  # expected case -- rc compiler is "capable enough"
  message(STATUS
    "rc compiler handles quoted strings with spaces in values via /D")
  set(TEXTFILE_FROM_SOURCE_DIR "textfile, spaces in name, from binary dir")
  configure_file(${CMAKE_CURRENT_SOURCE_DIR}/test.txt
    "${CMAKE_CURRENT_BINARY_DIR}/test with spaces.txt" @ONLY)
  add_definitions(/DCMAKE_RCDEFINE="test with spaces.txt")
endif()

if(CMAKE_RC_NO_INCLUDE)
  add_definitions(/DCMAKE_RC_NO_INCLUDE)
else()
  configure_file(${CMAKE_CURRENT_SOURCE_DIR}/include.rc.in
    "${CMAKE_CURRENT_BINARY_DIR}/include.rc" @ONLY)
  include_directories(${CMAKE_CURRENT_BINARY_DIR})
endif()

add_library(ResourceLib STATIC lib.cpp lib.rc)

add_executable(VSResource main.cpp test.rc)
target_link_libraries(VSResource ResourceLib)

if(MSVC AND NOT MSVC_VERSION VERSION_LESS 1600)
  set_property(SOURCE test.rc PROPERTY COMPILE_FLAGS /nologo)
endif()

set_property(TARGET VSResource
  PROPERTY VS_GLOBAL_CMakeTestVsGlobalVariable "test val")
