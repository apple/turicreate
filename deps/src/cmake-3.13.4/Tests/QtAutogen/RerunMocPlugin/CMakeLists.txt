cmake_minimum_required(VERSION 3.10)
project(RerunMocPlugin)
include("../AutogenTest.cmake")

# Tests Q_PLUGIN_METADATA and CMAKE_AUTOMOC_DEPEND_FILTERS
# json file change detection

# Dummy executable to generate a clean target
add_executable(dummy dummy.cpp)

# Utility variables
set(timeformat "%Y%j%H%M%S")
set(mocPlugSrcDir "${CMAKE_CURRENT_SOURCE_DIR}/MocPlugin")
set(mocPlugBinDir "${CMAKE_CURRENT_BINARY_DIR}/MocPlugin")

# Initial build
try_compile(MOC_PLUGIN
  "${mocPlugBinDir}"
  "${mocPlugSrcDir}"
  MocPlugin
  CMAKE_FLAGS "-DQT_TEST_VERSION=${QT_TEST_VERSION}"
              "-DCMAKE_AUTOGEN_VERBOSE=${CMAKE_AUTOGEN_VERBOSE}"
              "-DQT_QMAKE_EXECUTABLE:FILEPATH=${QT_QMAKE_EXECUTABLE}"
  OUTPUT_VARIABLE output
)
if (NOT MOC_PLUGIN)
  message(SEND_ERROR "Initial build of mocPlugin failed. Output: ${output}")
endif()

find_library(plAFile "PlugA" PATHS "${mocPlugBinDir}/Debug" "${mocPlugBinDir}" NO_DEFAULT_PATH)
find_library(plBFile "PlugB" PATHS "${mocPlugBinDir}/Debug" "${mocPlugBinDir}" NO_DEFAULT_PATH)
find_library(plCFile "PlugC" PATHS "${mocPlugBinDir}/Debug" "${mocPlugBinDir}" NO_DEFAULT_PATH)
find_library(plDFile "PlugD" PATHS "${mocPlugBinDir}/Debug" "${mocPlugBinDir}" NO_DEFAULT_PATH)
find_library(plEFile "PlugE" PATHS "${mocPlugBinDir}/Debug" "${mocPlugBinDir}" NO_DEFAULT_PATH)

# - Ensure that the timestamp will change.
# - Change the json files referenced by Q_PLUGIN_METADATA
# - Rebuild
file(TIMESTAMP "${plAFile}" plABefore "${timeformat}")
file(TIMESTAMP "${plBFile}" plBBefore "${timeformat}")
file(TIMESTAMP "${plCFile}" plCBefore "${timeformat}")
file(TIMESTAMP "${plDFile}" plDBefore "${timeformat}")
file(TIMESTAMP "${plEFile}" plEBefore "${timeformat}")

execute_process(COMMAND "${CMAKE_COMMAND}" -E sleep 1)
configure_file("${mocPlugSrcDir}/jsonIn/StyleD.json" "${mocPlugBinDir}/jsonFiles/StyleC.json")
configure_file("${mocPlugSrcDir}/jsonIn/StyleE.json" "${mocPlugBinDir}/jsonFiles/sub/StyleD.json")
configure_file("${mocPlugSrcDir}/jsonIn/StyleC.json" "${mocPlugBinDir}/jsonFiles/StyleE.json")
execute_process(COMMAND "${CMAKE_COMMAND}" --build . WORKING_DIRECTORY "${mocPlugBinDir}")

file(TIMESTAMP "${plAFile}" plAAfter "${timeformat}")
file(TIMESTAMP "${plBFile}" plBAfter "${timeformat}")
file(TIMESTAMP "${plCFile}" plCAfter "${timeformat}")
file(TIMESTAMP "${plDFile}" plDAfter "${timeformat}")
file(TIMESTAMP "${plEFile}" plEAfter "${timeformat}")

if (plAAfter GREATER plABefore)
  message(SEND_ERROR "file (${plAFile}) should not have changed!")
endif()
if (plBAfter GREATER plBBefore)
  message(SEND_ERROR "file (${plBFile}) should not have changed!")
endif()
if (NOT plCAfter GREATER plCBefore)
  message(SEND_ERROR "file (${plCFile}) should have changed!")
endif()
if (NOT plDAfter GREATER plDBefore)
  message(SEND_ERROR "file (${plDFile}) should have changed!")
endif()
if (NOT plEAfter GREATER plEBefore)
  # There's a bug in Ninja on Windows
  # https://gitlab.kitware.com/cmake/cmake/issues/16776
  if(NOT ("${CMAKE_GENERATOR}" MATCHES "Ninja"))
    message(SEND_ERROR "file (${plEFile}) should have changed!")
  endif()
endif()

# - Ensure that the timestamp will change.
# - Change the json files referenced by A_CUSTOM_MACRO
# - Rebuild
file(TIMESTAMP "${plCFile}" plCBefore "${timeformat}")
file(TIMESTAMP "${plDFile}" plDBefore "${timeformat}")
file(TIMESTAMP "${plEFile}" plEBefore "${timeformat}")

execute_process(COMMAND "${CMAKE_COMMAND}" -E sleep 1)
configure_file("${mocPlugSrcDir}/jsonIn/StyleE.json" "${mocPlugBinDir}/jsonFiles/StyleC_Custom.json")
configure_file("${mocPlugSrcDir}/jsonIn/StyleC.json" "${mocPlugBinDir}/jsonFiles/sub/StyleD_Custom.json")
configure_file("${mocPlugSrcDir}/jsonIn/StyleD.json" "${mocPlugBinDir}/jsonFiles/StyleE_Custom.json")
execute_process(COMMAND "${CMAKE_COMMAND}" --build . WORKING_DIRECTORY "${mocPlugBinDir}")

file(TIMESTAMP "${plCFile}" plCAfter "${timeformat}")
file(TIMESTAMP "${plDFile}" plDAfter "${timeformat}")
file(TIMESTAMP "${plEFile}" plEAfter "${timeformat}")

if (NOT plCAfter GREATER plCBefore)
  message(SEND_ERROR "file (${plCFile}) should have changed!")
endif()
if (NOT plDAfter GREATER plDBefore)
  message(SEND_ERROR "file (${plDFile}) should have changed!")
endif()
if (NOT plEAfter GREATER plEBefore)
  # There's a bug in Ninja on Windows
  # https://gitlab.kitware.com/cmake/cmake/issues/16776
  if(NOT ("${CMAKE_GENERATOR}" MATCHES "Ninja"))
    message(SEND_ERROR "file (${plEFile}) should have changed!")
  endif()
endif()
