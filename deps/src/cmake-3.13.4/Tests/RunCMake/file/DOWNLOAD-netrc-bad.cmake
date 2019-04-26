if(NOT "${CMAKE_CURRENT_SOURCE_DIR}" MATCHES "^/")
  set(slash /)
endif()
file(DOWNLOAD "" "" NETRC)
file(DOWNLOAD "" "" NETRC_FILE)
set(CMAKE_NETRC FALSE)
file(DOWNLOAD
  "file://${slash}${CMAKE_CURRENT_SOURCE_DIR}/DOWNLOAD-netrc-bad.txt"
  "${CMAKE_CURRENT_BINARY_DIR}/netrc-bad.txt"
  NETRC INVALID
  )
file(DOWNLOAD
  "file://${slash}${CMAKE_CURRENT_SOURCE_DIR}/DOWNLOAD-netrc-bad.txt"
  "${CMAKE_CURRENT_BINARY_DIR}/netrc-bad.txt"
  )
