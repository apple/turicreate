cmake_minimum_required(VERSION 3.10)
project(RerunRccDepends)
include("../AutogenTest.cmake")

# Tests rcc rebuilding when a resource file changes

# Dummy executable to generate a clean target
add_executable(dummy dummy.cpp)

# When a .qrc or a file listed in a .qrc file changes,
# the target must be rebuilt
set(timeformat "%Y%j%H%M%S")
set(rccDepSD "${CMAKE_CURRENT_SOURCE_DIR}/RccDepends")
set(rccDepBD "${CMAKE_CURRENT_BINARY_DIR}/RccDepends")

# Initial build
configure_file(${rccDepSD}/resPlainA.qrc.in ${rccDepBD}/resPlain.qrc COPYONLY)
configure_file(${rccDepSD}/resGenA.qrc.in ${rccDepBD}/resGen.qrc.in COPYONLY)
try_compile(RCC_DEPENDS
  "${rccDepBD}"
  "${rccDepSD}"
  RccDepends
  CMAKE_FLAGS "-DQT_TEST_VERSION=${QT_TEST_VERSION}"
              "-DCMAKE_AUTOGEN_VERBOSE=${CMAKE_AUTOGEN_VERBOSE}"
              "-DQT_QMAKE_EXECUTABLE:FILEPATH=${QT_QMAKE_EXECUTABLE}"
  OUTPUT_VARIABLE output
)
if (NOT RCC_DEPENDS)
  message(SEND_ERROR "Initial build of rccDepends failed. Output: ${output}")
endif()

# Get name of the output binaries
file(STRINGS "${rccDepBD}/targetPlain.txt" targetListPlain ENCODING UTF-8)
file(STRINGS "${rccDepBD}/targetGen.txt" targetListGen ENCODING UTF-8)
list(GET targetListPlain 0 rccDepBinPlain)
list(GET targetListGen 0 rccDepBinGen)
message("Target that uses a plain .qrc file is:\n  ${rccDepBinPlain}")
message("Target that uses a GENERATED .qrc file is:\n  ${rccDepBinGen}")


message("Changing a resource files listed in the .qrc file")
# - Acquire binary timestamps before the build
file(TIMESTAMP "${rccDepBinPlain}" rdPlainBefore "${timeformat}")
file(TIMESTAMP "${rccDepBinGen}" rdGenBefore "${timeformat}")
# - Ensure that the timestamp will change
# - Change a resource files listed in the .qrc file
# - Rebuild
execute_process(COMMAND "${CMAKE_COMMAND}" -E sleep 1)
file(TOUCH "${rccDepBD}/resPlain/input.txt" "${rccDepBD}/resGen/input.txt")
execute_process(COMMAND "${CMAKE_COMMAND}" --build . WORKING_DIRECTORY "${rccDepBD}" RESULT_VARIABLE result)
if (result)
  message(SEND_ERROR "Second build of rccDepends failed.")
endif()
# - Acquire binary timestamps after the build
file(TIMESTAMP "${rccDepBinPlain}" rdPlainAfter "${timeformat}")
file(TIMESTAMP "${rccDepBinGen}" rdGenAfter "${timeformat}")
# - Test if timestamps changed
if (NOT rdPlainAfter GREATER rdPlainBefore)
  message(SEND_ERROR "Plain .qrc binary ${rccDepBinPlain}) should have changed!")
endif()
if (NOT rdGenAfter GREATER rdGenBefore)
  message(SEND_ERROR "GENERATED .qrc binary ${rccDepBinGen} should have changed!")
endif()


message("Changing the .qrc file")
# - Acquire binary timestamps before the build
file(TIMESTAMP "${rccDepBinPlain}" rdPlainBefore "${timeformat}")
file(TIMESTAMP "${rccDepBinGen}" rdGenBefore "${timeformat}")
# - Ensure that the timestamp will change
# - Change the .qrc file
# - Rebuild
execute_process(COMMAND "${CMAKE_COMMAND}" -E sleep 1)
configure_file(${rccDepSD}/resPlainB.qrc.in ${rccDepBD}/resPlain.qrc COPYONLY)
configure_file(${rccDepSD}/resGenB.qrc.in ${rccDepBD}/resGen.qrc.in COPYONLY)
execute_process(COMMAND "${CMAKE_COMMAND}" --build . WORKING_DIRECTORY "${rccDepBD}" RESULT_VARIABLE result)
if (result)
  message(SEND_ERROR "Third build of rccDepends failed.")
endif()
# - Acquire binary timestamps after the build
file(TIMESTAMP "${rccDepBinPlain}" rdPlainAfter "${timeformat}")
file(TIMESTAMP "${rccDepBinGen}" rdGenAfter "${timeformat}")
# - Test if timestamps changed
if (NOT rdPlainAfter GREATER rdPlainBefore)
  message(SEND_ERROR "Plain .qrc binary ${rccDepBinPlain}) should have changed!")
endif()
if (NOT rdGenAfter GREATER rdGenBefore)
  message(SEND_ERROR "GENERATED .qrc binary ${rccDepBinGen} should have changed!")
endif()


message("Changing a newly added resource files listed in the .qrc file")
# - Acquire binary timestamps before the build
file(TIMESTAMP "${rccDepBinPlain}" rdPlainBefore "${timeformat}")
file(TIMESTAMP "${rccDepBinGen}" rdGenBefore "${timeformat}")
# - Ensure that the timestamp will change
# - Change a newly added resource files listed in the .qrc file
# - Rebuild
execute_process(COMMAND "${CMAKE_COMMAND}" -E sleep 1)
file(TOUCH "${rccDepBD}/resPlain/inputAdded.txt" "${rccDepBD}/resGen/inputAdded.txt")
execute_process(COMMAND "${CMAKE_COMMAND}" --build . WORKING_DIRECTORY "${rccDepBD}" RESULT_VARIABLE result)
if (result)
  message(SEND_ERROR "Fourth build of rccDepends failed.")
endif()
# - Acquire binary timestamps after the build
file(TIMESTAMP "${rccDepBinPlain}" rdPlainAfter "${timeformat}")
file(TIMESTAMP "${rccDepBinGen}" rdGenAfter "${timeformat}")
# - Test if timestamps changed
if (NOT rdPlainAfter GREATER rdPlainBefore)
  message(SEND_ERROR "Plain .qrc binary ${rccDepBinPlain}) should have changed!")
endif()
if (NOT rdGenAfter GREATER rdGenBefore)
  message(SEND_ERROR "GENERATED .qrc binary ${rccDepBinGen} should have changed!")
endif()


message("Changing nothing in the .qrc file")
# - Acquire binary timestamps before the build
file(TIMESTAMP "${rccDepBinPlain}" rdPlainBefore "${timeformat}")
file(TIMESTAMP "${rccDepBinGen}" rdGenBefore "${timeformat}")
# - Ensure that the timestamp will change
# - Change nothing
# - Rebuild
execute_process(COMMAND "${CMAKE_COMMAND}" -E sleep 1)
execute_process(COMMAND "${CMAKE_COMMAND}" --build . WORKING_DIRECTORY "${rccDepBD}" RESULT_VARIABLE result)
if (result)
  message(SEND_ERROR "Fifth build of rccDepends failed.")
endif()
# - Acquire binary timestamps after the build
file(TIMESTAMP "${rccDepBinPlain}" rdPlainAfter "${timeformat}")
file(TIMESTAMP "${rccDepBinGen}" rdGenAfter "${timeformat}")
# - Test if timestamps changed
if (rdPlainAfter GREATER rdPlainBefore)
  message(SEND_ERROR "Plain .qrc binary ${rccDepBinPlain}) should NOT have changed!")
endif()
if (rdGenAfter GREATER rdGenBefore)
  message(SEND_ERROR "GENERATED .qrc binary ${rccDepBinGen} should NOT have changed!")
endif()
