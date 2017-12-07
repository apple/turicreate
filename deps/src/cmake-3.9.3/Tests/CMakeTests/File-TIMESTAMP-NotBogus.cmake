set(STAMP_FILENAME "${CMAKE_CURRENT_BINARY_DIR}/FileTimestamp-Stamp")
set(STAMP_FORMAT "%Y-%m-%d")

unset(ENV{SOURCE_DATE_EPOCH})
string(TIMESTAMP timestamp1 "${STAMP_FORMAT}")

file(WRITE "${STAMP_FILENAME}" "foo")
file(TIMESTAMP "${STAMP_FILENAME}" timestamp2 "${STAMP_FORMAT}")

string(TIMESTAMP timestamp3 "${STAMP_FORMAT}")

message(STATUS "timestamp1 [${timestamp1}]")
message(STATUS "timestamp2 [${timestamp2}]")
message(STATUS "timestamp3 [${timestamp3}]")

if(timestamp1 STREQUAL timestamp3)
  if(NOT timestamp1 STREQUAL timestamp2)
    message(FATAL_ERROR
      "timestamp mismatch [${timestamp1}] != [${timestamp2}]")
  else()
    message("all timestamps match")
  endif()
else()
  message(WARNING "this test may race when run at midnight")
endif()
