include(ExternalData)

set(output "${CMAKE_BINARY_DIR}/MissingData.txt")
ExternalData_Expand_Arguments(Data args DATA{MissingData.txt,Data.txt})
if("x${args}" STREQUAL "x${output}")
  message(STATUS "Missing data reference correctly transformed!")
else()
  message(FATAL_ERROR "Missing data reference transformed to:\n  ${args}\n"
    "but we expected:\n  ${output}")
endif()
