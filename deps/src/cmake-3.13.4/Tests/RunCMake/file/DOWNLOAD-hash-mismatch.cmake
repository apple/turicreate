if(NOT "${CMAKE_CURRENT_SOURCE_DIR}" MATCHES "^/")
  set(slash /)
endif()
file(DOWNLOAD
  "file://${slash}${CMAKE_CURRENT_SOURCE_DIR}/DOWNLOAD-hash-mismatch.txt"
  ${CMAKE_CURRENT_BINARY_DIR}/hash-mismatch.txt
  EXPECTED_HASH SHA1=0123456789abcdef0123456789abcdef01234567
  STATUS status
  )
message("status='${status}'")
