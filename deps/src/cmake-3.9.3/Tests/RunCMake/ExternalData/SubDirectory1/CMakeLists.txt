set(input ../Data.txt)
set(output ${CMAKE_BINARY_DIR}/Data.txt)
ExternalData_Expand_Arguments(Data args DATA{${input}})
if("x${args}" STREQUAL "x${output}")
  message(STATUS "Data reference correctly transformed in parent dir 1!")
else()
  message(FATAL_ERROR "Data reference transformed to:\n  ${args}\n"
    "but we expected:\n  ${output}")
endif()

set(input ${CMAKE_CURRENT_SOURCE_DIR}/../Data.txt)
set(output ${CMAKE_BINARY_DIR}/Data.txt)
ExternalData_Expand_Arguments(Data args DATA{${input}})
if("x${args}" STREQUAL "x${output}")
  message(STATUS "Data reference correctly transformed in parent dir 2!")
else()
  message(FATAL_ERROR "Data reference transformed to:\n  ${args}\n"
    "but we expected:\n  ${output}")
endif()

set(input Data.txt)
set(output ${CMAKE_CURRENT_BINARY_DIR}/Data.txt)
ExternalData_Expand_Arguments(Data args DATA{${input}})
if("x${args}" STREQUAL "x${output}")
  message(STATUS "Data reference correctly transformed in current dir!")
else()
  message(FATAL_ERROR "Data reference transformed to:\n  ${args}\n"
    "but we expected:\n  ${output}")
endif()
