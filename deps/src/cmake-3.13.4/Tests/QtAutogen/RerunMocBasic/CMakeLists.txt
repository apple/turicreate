cmake_minimum_required(VERSION 3.10)
project(RerunMocBasic)
include("../AutogenTest.cmake")

# Dummy executable to generate a clean target
add_executable(dummy dummy.cpp)

set(timeformat "%Y%j%H%M%S")
set(mocBasicSrcDir "${CMAKE_CURRENT_SOURCE_DIR}/MocBasic")
set(mocBasicBinDir "${CMAKE_CURRENT_BINARY_DIR}/MocBasic")

# Initial build
configure_file("${mocBasicSrcDir}/test1a.h.in" "${mocBasicBinDir}/test1.h" COPYONLY)
try_compile(MOC_RERUN
  "${mocBasicBinDir}"
  "${mocBasicSrcDir}"
  MocBasic
  CMAKE_FLAGS "-DQT_TEST_VERSION=${QT_TEST_VERSION}"
              "-DCMAKE_AUTOGEN_VERBOSE=${CMAKE_AUTOGEN_VERBOSE}"
              "-DQT_QMAKE_EXECUTABLE:FILEPATH=${QT_QMAKE_EXECUTABLE}"
  OUTPUT_VARIABLE output
)
if (NOT MOC_RERUN)
  message(SEND_ERROR "Initial build of mocBasic failed. Output: ${output}")
endif()
# Get name of the output binary
file(STRINGS "${mocBasicBinDir}/mocBasic.txt" mocBasicList ENCODING UTF-8)
list(GET mocBasicList 0 mocBasicBin)

message("Changing the header content for a MOC rerun")
# - Acquire binary timestamps before the build
file(TIMESTAMP "${mocBasicBin}" timeBefore "${timeformat}")
# - Ensure that the timestamp will change
# - Change header file content and rebuild
# - Rebuild
execute_process(COMMAND "${CMAKE_COMMAND}" -E sleep 1)
configure_file("${mocBasicSrcDir}/test1b.h.in" "${mocBasicBinDir}/test1.h" COPYONLY)
execute_process(COMMAND "${CMAKE_COMMAND}" --build . WORKING_DIRECTORY "${mocBasicBinDir}" RESULT_VARIABLE result )
if (result)
  message(SEND_ERROR "Second build of mocBasic failed.")
endif()
# - Acquire binary timestamps after the build
file(TIMESTAMP "${mocBasicBin}" timeAfter "${timeformat}")
# - Test if timestamps changed
if (NOT timeAfter GREATER timeBefore)
  message(SEND_ERROR "File (${mocBasicBin}) should have changed!")
endif()


message("Changing nothing for a MOC rerun")
# - Acquire binary timestamps before the build
file(TIMESTAMP "${mocBasicBin}" timeBefore "${timeformat}")
# - Ensure that the timestamp would change
# - Change nothing
# - Rebuild
execute_process(COMMAND "${CMAKE_COMMAND}" -E sleep 1)
execute_process(COMMAND "${CMAKE_COMMAND}" --build . WORKING_DIRECTORY "${mocBasicBinDir}" RESULT_VARIABLE result )
if (result)
  message(SEND_ERROR "Third build of mocBasic failed.")
endif()
# - Acquire binary timestamps after the build
file(TIMESTAMP "${mocBasicBin}" timeAfter "${timeformat}")
# - Test if timestamps changed
if (timeAfter GREATER timeBefore)
  message(SEND_ERROR "File (${mocBasicBin}) should not have changed!")
endif()
