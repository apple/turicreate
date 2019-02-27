cmake_minimum_required(VERSION 3.11.2)
project(RerunRccConfigChange)
include("../AutogenTest.cmake")

# Tests rcc rebuilding after a configuration change

# Dummy executable to generate a clean target
add_executable(dummy dummy.cpp)

# When a .qrc or a file listed in a .qrc file changes,
# the target must be rebuilt
set(timeformat "%Y%j%H%M%S")
set(rccDepSD "${CMAKE_CURRENT_SOURCE_DIR}/RccConfigChange")
set(rccDepBD "${CMAKE_CURRENT_BINARY_DIR}/RccConfigChange")

# Initial build
try_compile(RCC_DEPENDS
  "${rccDepBD}"
  "${rccDepSD}"
  RccConfigChange
  CMAKE_FLAGS "-DQT_TEST_VERSION=${QT_TEST_VERSION}"
              "-DCMAKE_AUTOGEN_VERBOSE=${CMAKE_AUTOGEN_VERBOSE}"
              "-DQT_QMAKE_EXECUTABLE:FILEPATH=${QT_QMAKE_EXECUTABLE}"
  OUTPUT_VARIABLE output
)
if (NOT RCC_DEPENDS)
  message(SEND_ERROR "Initial build of rccConfigChange failed. Output: ${output}")
endif()

# - Rebuild Release
message("Rebuilding rccConfigChange in Release configuration")
execute_process(COMMAND "${CMAKE_COMMAND}" --build . --config Release WORKING_DIRECTORY "${rccDepBD}" RESULT_VARIABLE result)
if (result)
  message(SEND_ERROR "Release build of rccConfigChange failed.")
endif()

# - Rebuild Debug
message("Rebuilding rccConfigChange in Debug configuration")
execute_process(COMMAND "${CMAKE_COMMAND}" --build . --config Debug WORKING_DIRECTORY "${rccDepBD}" RESULT_VARIABLE result)
if (result)
  message(SEND_ERROR "Debug build of rccConfigChange failed.")
endif()
