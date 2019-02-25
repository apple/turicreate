include(ExternalData)
set(ExternalData_URL_TEMPLATES
  "file:///${CMAKE_CURRENT_SOURCE_DIR}/%(algo)/%(hash)"
  )
set(input SubDirectory1/Data.txt)
set(output ${CMAKE_CURRENT_BINARY_DIR}/SubDirectory1/Data.txt)
ExternalData_Expand_Arguments(Data args DATA{${input}})
if("x${args}" STREQUAL "x${output}")
  message(STATUS "Data reference correctly transformed!")
else()
  message(FATAL_ERROR "Data reference transformed to:\n  ${args}\n"
    "but we expected:\n  ${output}")
endif()
