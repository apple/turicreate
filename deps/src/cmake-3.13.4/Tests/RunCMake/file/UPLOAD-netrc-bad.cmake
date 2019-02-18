if(NOT "${CMAKE_CURRENT_SOURCE_DIR}" MATCHES "^/")
  set(slash /)
endif()
file(UPLOAD "" "" NETRC)
file(UPLOAD "" "" NETRC_FILE)
set(CMAKE_NETRC FALSE)
file(UPLOAD
  "${CMAKE_CURRENT_SOURCE_DIR}/UPLOAD-netrc-bad.txt"
  "file://${slash}${CMAKE_CURRENT_BINARY_DIR}/netrc-bad.txt"
  NETRC INVALID
  )
file(UPLOAD
  "${CMAKE_CURRENT_SOURCE_DIR}/UPLOAD-netrc-bad.txt"
  "file://${slash}${CMAKE_CURRENT_BINARY_DIR}/netrc-bad.txt"
  )
