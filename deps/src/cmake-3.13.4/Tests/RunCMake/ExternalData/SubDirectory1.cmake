include(ExternalData)
set(ExternalData_URL_TEMPLATES
  "file:///${CMAKE_CURRENT_SOURCE_DIR}/%(algo)/%(hash)"
  )
add_subdirectory(SubDirectory1)
