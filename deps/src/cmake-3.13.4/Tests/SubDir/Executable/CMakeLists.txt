add_executable(test test.cxx)

if (NOT DEFINED_AFTER_SUBDIRS_COMMAND)
  message(FATAL_ERROR "DEFINED_AFTER_SUBDIRS_COMMAND should be defined.")
endif()

string(FIND "${CMAKE_CURRENT_BINARY_DIR}" "SubDir/Executable" location)
string(LENGTH "${CMAKE_CURRENT_BINARY_DIR}" dirLength)
math(EXPR suffixLength "${dirLength} - ${location}")

if (NOT suffixLength EQUAL 17)
  message(FATAL_ERROR "CMAKE_CURRENT_BINARY_DIR does not end with \"SubDir/Executable\"")
endif()
