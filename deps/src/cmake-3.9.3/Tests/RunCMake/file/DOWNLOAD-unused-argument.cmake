if(NOT "${CMAKE_CURRENT_SOURCE_DIR}" MATCHES "^/")
  set(slash /)
endif()
file(DOWNLOAD
  "file://${slash}${CMAKE_CURRENT_SOURCE_DIR}/DOWNLOAD-unused-argument.txt"
  "${CMAKE_CURRENT_BINARY_DIR}/unused-argument.txt"
  JUNK
  )
