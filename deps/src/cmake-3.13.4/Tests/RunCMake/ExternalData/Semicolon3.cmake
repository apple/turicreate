include(ExternalData)
set(ExternalData_URL_TEMPLATES
  "file:///${CMAKE_CURRENT_SOURCE_DIR}/%(algo)/%(hash)"
  )
set(input "DATA{a;b}")
ExternalData_Expand_Arguments(Data args "${input}")
if("x${args}" STREQUAL "x${input}")
  message(STATUS "Data arguments correctly not transformed!")
else()
  message(FATAL_ERROR "Data arguments transformed to:\n  ${args}\n"
    "but we expected:\n  ${input}")
endif()
