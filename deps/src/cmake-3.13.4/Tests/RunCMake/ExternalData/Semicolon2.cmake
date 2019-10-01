include(ExternalData)
set(ExternalData_URL_TEMPLATES
  "file:///${CMAKE_CURRENT_SOURCE_DIR}/%(algo)/%(hash)"
  )
set(input Data.txt)
set(output ${CMAKE_CURRENT_BINARY_DIR}/Data.txt)
ExternalData_Expand_Arguments(Data args "DATA{${input}};a\\;b;c;d;DATA{${input}}")
set(expect "${output};a\\;b;c;d;${output}")
if("x${args}" STREQUAL "x${expect}")
  message(STATUS "Data arguments correctly transformed!")
else()
  message(FATAL_ERROR "Data arguments transformed to:\n  ${args}\n"
    "but we expected:\n  ${expect}")
endif()
