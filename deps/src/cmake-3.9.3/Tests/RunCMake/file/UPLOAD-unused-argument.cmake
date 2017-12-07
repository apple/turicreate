if(NOT "${CMAKE_CURRENT_BINARY_DIR}" MATCHES "^/")
  set(slash /)
endif()
file(UPLOAD
  "${CMAKE_CURRENT_SOURCE_DIR}/UPLOAD-unused-argument.txt"
  "file://${slash}${CMAKE_CURRENT_BINARY_DIR}/unused-argument.txt"
  JUNK
  )
